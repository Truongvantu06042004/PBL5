import 'dart:convert';

class RainmakerSession {
  const RainmakerSession({
    required this.idToken,
    required this.accessToken,
    required this.refreshToken,
    required this.userId,
    required this.email,
  });

  final String idToken;
  final String accessToken;
  final String refreshToken;
  final String userId;
  final String email;

  Map<String, String> toStorage() => <String, String>{
    'idToken': idToken,
    'accessToken': accessToken,
    'refreshToken': refreshToken,
    'userId': userId,
    'email': email,
  };

  static RainmakerSession? fromStorage(Map<String, String> values) {
    if (values.values.any((value) => value.isEmpty)) return null;
    return RainmakerSession(
      idToken: values['idToken']!,
      accessToken: values['accessToken']!,
      refreshToken: values['refreshToken']!,
      userId: values['userId']!,
      email: values['email']!,
    );
  }
}

class RainmakerNode {
  RainmakerNode({
    required this.id,
    required this.name,
    required this.online,
    required this.devices,
  });

  final String id;
  final String name;
  final bool online;
  final List<RainmakerDevice> devices;

  factory RainmakerNode.fromJson(Map<String, dynamic> json) {
    final config = _asMap(json['config']);
    final params = _asMap(json['params']);
    final deviceConfigs = _asList(config['devices']);
    final devices = deviceConfigs
        .map((device) => RainmakerDevice.fromConfig(device, params))
        .toList(growable: false);
    final nodeId = (json['id'] ?? json['node_id'] ?? '').toString();
    final nodeName = (config['name'] ?? json['name'] ?? nodeId).toString();
    return RainmakerNode(
      id: nodeId,
      name: nodeName.isEmpty ? 'ESP device' : nodeName,
      online: json['online'] != false && config.isNotEmpty,
      devices: devices,
    );
  }
}

class RainmakerDevice {
  RainmakerDevice({
    required this.name,
    required this.type,
    required this.params,
  });

  final String name;
  final String type;
  final List<RainmakerParam> params;

  factory RainmakerDevice.fromConfig(
    Map<String, dynamic> config,
    Map<String, dynamic> nodeParams,
  ) {
    final name = (config['name'] ?? 'Device').toString();
    final values = _asMap(nodeParams[name]);
    return RainmakerDevice(
      name: name,
      type: (config['type'] ?? 'device').toString(),
      params: _asList(config['params'])
          .map((param) => RainmakerParam.fromConfig(param, values))
          .toList(growable: false),
    );
  }
}

class RainmakerParam {
  RainmakerParam({
    required this.name,
    required this.type,
    required this.dataType,
    required this.value,
    required this.min,
    required this.max,
    required this.properties,
  });

  final String name;
  final String type;
  final String dataType;
  final dynamic value;
  final double min;
  final double max;
  final List<String> properties;

  bool get isWritable => properties.contains('write');
  bool get isPower =>
      type == 'esp.param.power' || name.toLowerCase() == 'power';
  bool get isNumeric =>
      value is num || dataType == 'int' || dataType == 'float';
  bool get isHue => name.toLowerCase() == 'hue';
  bool get isSaturation => name.toLowerCase() == 'saturation';
  bool get isBrightness => name.toLowerCase() == 'brightness';

  factory RainmakerParam.fromConfig(
    Map<String, dynamic> config,
    Map<String, dynamic> values,
  ) {
    final name = (config['name'] ?? 'Parameter').toString();
    final bounds = _asMap(config['bounds']);
    final properties = _asList(config['properties'])
        .map((item) => item.toString())
        .toList();
    return RainmakerParam(
      name: name,
      type: (config['type'] ?? '').toString(),
      dataType: (config['data_type'] ?? '').toString(),
      value: values[name] ?? config['value'],
      min: _number(bounds['min'], 0),
      max: _number(bounds['max'], name.toLowerCase() == 'hue' ? 360 : 100),
      properties: properties,
    );
  }
}

Map<String, dynamic> _asMap(dynamic value) {
  if (value is Map<String, dynamic>) return value;
  if (value is Map) {
    return value.map((key, item) => MapEntry(key.toString(), item));
  }
  if (value is String) {
    try {
      return _asMap(jsonDecode(value));
    } on FormatException {
      return <String, dynamic>{};
    }
  }
  return <String, dynamic>{};
}

List<Map<String, dynamic>> _asList(dynamic value) {
  if (value is! List) return const <Map<String, dynamic>>[];
  return value
      .map(_asMap)
      .where((item) => item.isNotEmpty)
      .toList(growable: false);
}

double _number(dynamic value, double fallback) {
  if (value is num) return value.toDouble();
  return double.tryParse(value?.toString() ?? '') ?? fallback;
}
