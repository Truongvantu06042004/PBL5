import 'dart:convert';

import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:flutter_web_auth_2/flutter_web_auth_2.dart';
import 'package:http/http.dart' as http;

import 'app_config.dart';
import 'rainmaker_models.dart';

class RainmakerException implements Exception {
  RainmakerException(this.message);
  final String message;

  @override
  String toString() => message;
}

class RainmakerApi {
  RainmakerApi({http.Client? client, FlutterSecureStorage? storage})
    : _client = client ?? http.Client(),
      _storage = storage ?? const FlutterSecureStorage();

  final http.Client _client;
  final FlutterSecureStorage _storage;

  Future<RainmakerSession?> restoreSession() async {
    final values = <String, String>{};
    for (final key in _storageKeys) {
      values[key] = await _storage.read(key: key) ?? '';
    }
    return RainmakerSession.fromStorage(values);
  }

  Future<void> clearSession() async {
    for (final key in _storageKeys) {
      await _storage.delete(key: key);
    }
  }

  Future<RainmakerSession> loginWithPassword(
    String email,
    String password,
  ) async {
    final response = await _client.post(
      _api('/v1/login'),
      headers: _jsonHeaders,
      body: jsonEncode(<String, String>{
        'user_name': email.trim(),
        'password': password,
      }),
    );
    return _saveTokenResponse(response, email.trim());
  }

  Future<RainmakerSession> loginWithProvider(String provider) async {
    final authorize = Uri.parse('${AppConfig.authBaseUrl}/authorize').replace(
      queryParameters: <String, String>{
        'identity_provider': provider,
        'redirect_uri': AppConfig.redirectUri,
        'response_type': 'CODE',
        'client_id': AppConfig.oauthClientId,
      },
    );
    final callback = await FlutterWebAuth2.authenticate(
      url: authorize.toString(),
      callbackUrlScheme: 'rainmaker',
    );
    final code = Uri.parse(callback).queryParameters['code'];
    if (code == null || code.isEmpty) {
      throw RainmakerException('Không nhận được mã đăng nhập từ $provider.');
    }

    final response = await _client.post(
      Uri.parse('${AppConfig.authBaseUrl}/token'),
      headers: const <String, String>{
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: <String, String>{
        'grant_type': 'authorization_code',
        'client_id': AppConfig.oauthClientId,
        'code': code,
        'redirect_uri': AppConfig.redirectUri,
      },
    );
    return _saveTokenResponse(response, '');
  }

  Future<List<RainmakerNode>> getNodes(RainmakerSession session) async {
    final response = await _client.get(
      _api('/v1/user/nodes', <String, String>{'node_details': 'true'}),
      headers: _authHeaders(session),
    );
    final body = _decodeSuccess(response);
    final rawNodes = body['node_details'] ?? body['nodes'] ?? const <dynamic>[];
    if (rawNodes is! List) return const <RainmakerNode>[];
    return rawNodes
        .whereType<Map>()
        .map(
          (node) => RainmakerNode.fromJson(
            node.map((key, value) => MapEntry(key.toString(), value)),
          ),
        )
        .where((node) => node.id.isNotEmpty)
        .toList(growable: false);
  }

  Future<void> updateParameter({
    required RainmakerSession session,
    required String nodeId,
    required String deviceName,
    required String parameterName,
    required dynamic value,
  }) async {
    final response = await _client.put(
      _api('/v1/user/nodes/params', <String, String>{'node_id': nodeId}),
      headers: _authHeaders(session),
      body: jsonEncode(<String, dynamic>{
        deviceName: <String, dynamic>{parameterName: value},
      }),
    );
    _decodeSuccess(response);
  }

  Future<void> addNode({
    required RainmakerSession session,
    required String nodeId,
    required String secretKey,
  }) async {
    final response = await _client.put(
      _api('/v1/user/nodes/mapping'),
      headers: _authHeaders(session),
      body: jsonEncode(<String, String>{
        'node_id': nodeId,
        'secret_key': secretKey,
        'operation': 'add',
      }),
    );
    _decodeSuccess(response);
  }

  Future<void> _store(RainmakerSession session) async {
    for (final entry in session.toStorage().entries) {
      await _storage.write(key: entry.key, value: entry.value);
    }
  }

  Future<RainmakerSession> _saveTokenResponse(
    http.Response response,
    String fallbackEmail,
  ) async {
    final body = _decodeSuccess(response);
    final idToken = body['idtoken']?.toString() ?? '';
    final accessToken = body['accesstoken']?.toString() ?? '';
    final refreshToken = body['refreshtoken']?.toString() ?? '';
    if (idToken.isEmpty || accessToken.isEmpty || refreshToken.isEmpty) {
      throw RainmakerException('Máy chủ không trả về đủ thông tin đăng nhập.');
    }
    final claims = _decodeJwtPayload(idToken);
    final session = RainmakerSession(
      idToken: idToken,
      accessToken: accessToken,
      refreshToken: refreshToken,
      userId: claims['custom:user_id']?.toString() ?? '',
      email: claims['email']?.toString() ?? fallbackEmail,
    );
    if (session.userId.isEmpty) {
      throw RainmakerException('Không thể đọc user ID từ token RainMaker.');
    }
    await _store(session);
    return session;
  }

  Map<String, dynamic> _decodeSuccess(http.Response response) {
    dynamic body;
    try {
      body = response.body.isEmpty
          ? <String, dynamic>{}
          : jsonDecode(response.body);
    } on FormatException {
      throw RainmakerException(
        'Máy chủ trả về dữ liệu không hợp lệ (${response.statusCode}).',
      );
    }
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final message = body is Map
          ? (body['message'] ?? body['error'] ?? response.reasonPhrase)
          : response.reasonPhrase;
      throw RainmakerException(
        message?.toString() ?? 'Yêu cầu thất bại (${response.statusCode}).',
      );
    }
    if (body is Map<String, dynamic>) return body;
    if (body is Map) {
      return body.map((key, value) => MapEntry(key.toString(), value));
    }
    return <String, dynamic>{};
  }

  Uri _api(String path, [Map<String, String>? query]) =>
      Uri.parse('${AppConfig.apiBaseUrl}$path').replace(queryParameters: query);

  Map<String, String> _authHeaders(RainmakerSession session) =>
      <String, String>{..._jsonHeaders, 'Authorization': session.accessToken};

  static const _jsonHeaders = <String, String>{
    'Content-Type': 'application/json',
  };
  static const _storageKeys = <String>[
    'idToken',
    'accessToken',
    'refreshToken',
    'userId',
    'email',
  ];
}

Map<String, dynamic> _decodeJwtPayload(String token) {
  final parts = token.split('.');
  if (parts.length < 2) return <String, dynamic>{};
  try {
    final payload = utf8.decode(
      base64Url.decode(base64Url.normalize(parts[1])),
    );
    final decoded = jsonDecode(payload);
    return decoded is Map<String, dynamic> ? decoded : <String, dynamic>{};
  } on FormatException {
    return <String, dynamic>{};
  }
}
