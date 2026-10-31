import 'dart:math';
import 'dart:typed_data';

import 'package:esp_provisioning/esp_provisioning.dart';
import 'package:flutter/foundation.dart';
import 'package:permission_handler/permission_handler.dart';

import 'app_config.dart';
import 'rainmaker_api.dart';
import 'rainmaker_models.dart';

class ProvisioningException implements Exception {
  ProvisioningException(this.message);
  final String message;

  @override
  String toString() => message;
}

class ProvisioningService {
  ProvisioningService(this._api)
    : _provisioner = EspProvisioning(AppConfig.bleServiceUuid);

  final RainmakerApi _api;
  final EspProvisioning _provisioner;

  bool get isSupported =>
      !kIsWeb &&
      (defaultTargetPlatform == TargetPlatform.android ||
          defaultTargetPlatform == TargetPlatform.iOS);

  Future<List<EspBleDevice>> scan() async {
    _ensureSupported();
    await _requestPermissions();
    return _provisioner.scanForDevices(AppConfig.bleNamePrefix);
  }

  Future<void> stopScan() async {
    if (isSupported) await _provisioner.stopScan();
  }

  Future<List<EspWifiAccessPoint>> connectAndGetNetworks({
    required String deviceName,
    required String proofOfPossession,
  }) async {
    _ensureSupported();
    await _provisioner.connect(deviceName, proofOfPossession);
    return _provisioner.getAccessPoints(deviceName);
  }

  /// Associates the ESP with the signed-in RainMaker user, sends Wi-Fi
  /// credentials, then creates the server mapping. This is the same sequence
  /// used by the original Android application.
  Future<void> provision({
    required RainmakerSession session,
    required String deviceName,
    required String ssid,
    required String password,
  }) async {
    _ensureSupported();
    if (ssid.trim().isEmpty) {
      throw ProvisioningException('Hãy nhập tên mạng Wi-Fi (SSID).');
    }
    final secretKey = _newSecretKey();
    try {
      final response = await _provisioner.sendDataToEndpoint(
        deviceName,
        AppConfig.cloudUserAssociationEndpoint,
        _encodeUserMapping(session.userId, secretKey),
      );
      final nodeId = _decodeNodeId(response);
      if (nodeId == null || nodeId.isEmpty) {
        throw ProvisioningException(
          'ESP không chấp nhận liên kết tài khoản RainMaker.',
        );
      }

      await _provisioner.setAccessPoint(deviceName, ssid.trim(), password);
      await _api.addNode(
        session: session,
        nodeId: nodeId,
        secretKey: secretKey,
      );
    } catch (error) {
      if (error is ProvisioningException || error is RainmakerException) {
        rethrow;
      }
      throw ProvisioningException('Provisioning không thành công: $error');
    } finally {
      // ESP normally disconnects itself after it receives Wi-Fi credentials.
      // Cleanup must not turn an otherwise successful setup into a failure.
      try {
        await _provisioner.disconnect(deviceName);
      } catch (_) {}
    }
  }

  Future<void> _requestPermissions() async {
    if (defaultTargetPlatform == TargetPlatform.android) {
      final statuses = await <Permission>[
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ].request();
      if (statuses.values.any(
        (status) => !status.isGranted && !status.isLimited,
      )) {
        throw ProvisioningException(
          'Cần cấp quyền Bluetooth và Vị trí để tìm ESP qua BLE.',
        );
      }
    } else if (defaultTargetPlatform == TargetPlatform.iOS) {
      final status = await Permission.bluetooth.request();
      if (!status.isGranted && !status.isLimited) {
        throw ProvisioningException(
          'Cần cấp quyền Bluetooth để tìm ESP qua BLE.',
        );
      }
    }
  }

  void _ensureSupported() {
    if (!isSupported) {
      throw ProvisioningException(
        'Thêm thiết bị BLE chỉ thực hiện trên Android hoặc iPhone/iPad.',
      );
    }
  }
}

/// Encodes the small protobuf message defined in esp_rmaker_user_mapping.proto.
/// Using this compact encoder avoids carrying Java-generated protobuf code into
/// the Flutter app while maintaining the firmware endpoint protocol.
Uint8List _encodeUserMapping(String userId, String secretKey) {
  final cmd = BytesBuilder()
    ..addByte(0x0a) // CmdSetUserMapping.UserID (field 1, string)
    ..add(_lengthDelimited(userId.codeUnits))
    ..addByte(0x12) // CmdSetUserMapping.SecretKey (field 2, string)
    ..add(_lengthDelimited(secretKey.codeUnits));
  final payload = BytesBuilder()
    ..addByte(0x52) // RMakerConfigPayload.cmd_set_user_mapping (field 10)
    ..add(_lengthDelimited(cmd.toBytes()));
  return payload.toBytes();
}

Uint8List _lengthDelimited(List<int> content) {
  final output = BytesBuilder()..add(_varint(content.length));
  output.add(content);
  return output.toBytes();
}

List<int> _varint(int value) {
  final bytes = <int>[];
  var current = value;
  do {
    var byte = current & 0x7f;
    current >>= 7;
    if (current != 0) byte |= 0x80;
    bytes.add(byte);
  } while (current != 0);
  return bytes;
}

String? _decodeNodeId(Uint8List response) {
  // Outer field 11 = resp_set_user_mapping, inner field 2 = NodeId.
  final responseMessage = _readLengthDelimitedField(response, 11);
  if (responseMessage == null) return null;
  final nodeIdBytes = _readLengthDelimitedField(responseMessage, 2);
  return nodeIdBytes == null ? null : String.fromCharCodes(nodeIdBytes);
}

Uint8List? _readLengthDelimitedField(Uint8List message, int wantedField) {
  var index = 0;
  while (index < message.length) {
    final key = _readVarint(message, index);
    if (key == null) return null;
    index = key.next;
    final field = key.value >> 3;
    final wireType = key.value & 0x07;
    if (wireType == 2) {
      final length = _readVarint(message, index);
      if (length == null) return null;
      index = length.next;
      final end = index + length.value;
      if (end > message.length) return null;
      final value = Uint8List.sublistView(message, index, end);
      if (field == wantedField) return value;
      index = end;
    } else if (wireType == 0) {
      final value = _readVarint(message, index);
      if (value == null) return null;
      index = value.next;
    } else {
      return null;
    }
  }
  return null;
}

({int value, int next})? _readVarint(Uint8List data, int start) {
  var result = 0;
  var shift = 0;
  var index = start;
  while (index < data.length && shift < 35) {
    final byte = data[index++];
    result |= (byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) return (value: result, next: index);
    shift += 7;
  }
  return null;
}

String _newSecretKey() {
  final random = Random.secure();
  final values = List<int>.generate(16, (_) => random.nextInt(256));
  final hex = values
      .map((value) => value.toRadixString(16).padLeft(2, '0'))
      .join();
  return '${hex.substring(0, 8)}-${hex.substring(8, 12)}-${hex.substring(12, 16)}-${hex.substring(16, 20)}-${hex.substring(20)}';
}
