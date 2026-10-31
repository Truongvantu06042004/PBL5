import 'package:esp_provisioning/esp_provisioning.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import 'app_config.dart';
import 'provisioning_service.dart';
import 'rainmaker_api.dart';
import 'rainmaker_models.dart';

class TvtRainMakerApp extends StatefulWidget {
  const TvtRainMakerApp({super.key});

  @override
  State<TvtRainMakerApp> createState() => _TvtRainMakerAppState();
}

class _TvtRainMakerAppState extends State<TvtRainMakerApp> {
  final _api = RainmakerApi();
  RainmakerSession? _session;
  var _loading = true;

  @override
  void initState() {
    super.initState();
    _restore();
  }

  Future<void> _restore() async {
    final session = await _api.restoreSession();
    if (mounted) {
      setState(() {
        _session = session;
        _loading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: AppConfig.appName,
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xffe53935)),
        useMaterial3: true,
      ),
      home: _loading
          ? const _SplashPage()
          : _session == null
          ? LoginPage(
              api: _api,
              onLoggedIn: (session) => setState(() => _session = session),
            )
          : HomePage(
              api: _api,
              session: _session!,
              onSignedOut: () => setState(() => _session = null),
            ),
    );
  }
}

class _SplashPage extends StatelessWidget {
  const _SplashPage();

  @override
  Widget build(BuildContext context) {
    return const Scaffold(body: Center(child: CircularProgressIndicator()));
  }
}

class LoginPage extends StatefulWidget {
  const LoginPage({required this.api, required this.onLoggedIn, super.key});

  final RainmakerApi api;
  final ValueChanged<RainmakerSession> onLoggedIn;

  @override
  State<LoginPage> createState() => _LoginPageState();
}

class _LoginPageState extends State<LoginPage> {
  final _formKey = GlobalKey<FormState>();
  final _email = TextEditingController();
  final _password = TextEditingController();
  var _busy = false;

  bool get _mobile =>
      !kIsWeb &&
      (defaultTargetPlatform == TargetPlatform.android ||
          defaultTargetPlatform == TargetPlatform.iOS);

  @override
  void dispose() {
    _email.dispose();
    _password.dispose();
    super.dispose();
  }

  Future<void> _login() async {
    if (!_formKey.currentState!.validate()) return;
    await _run(() => widget.api.loginWithPassword(_email.text, _password.text));
  }

  Future<void> _oauth(String provider) async {
    await _run(() => widget.api.loginWithProvider(provider));
  }

  Future<void> _run(Future<RainmakerSession> Function() action) async {
    setState(() => _busy = true);
    try {
      final session = await action();
      if (mounted) widget.onLoggedIn(session);
    } catch (error) {
      if (mounted) _showError(context, error);
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 420),
              child: Form(
                key: _formKey,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Image.asset(
                      'assets/images/tvt_rainmaker_logo.png',
                      height: 112,
                    ),
                    const SizedBox(height: 16),
                    Text(
                      AppConfig.appName,
                      textAlign: TextAlign.center,
                      style: Theme.of(context).textTheme.headlineMedium,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Điều khiển thiết bị ESP RainMaker',
                      textAlign: TextAlign.center,
                      style: Theme.of(context).textTheme.bodyLarge,
                    ),
                    const SizedBox(height: 32),
                    TextFormField(
                      controller: _email,
                      keyboardType: TextInputType.emailAddress,
                      decoration: const InputDecoration(
                        labelText: 'Email',
                        border: OutlineInputBorder(),
                      ),
                      validator: (value) =>
                          value == null || !value.contains('@')
                          ? 'Nhập email hợp lệ.'
                          : null,
                    ),
                    const SizedBox(height: 12),
                    TextFormField(
                      controller: _password,
                      obscureText: true,
                      decoration: const InputDecoration(
                        labelText: 'Mật khẩu',
                        border: OutlineInputBorder(),
                      ),
                      validator: (value) => value == null || value.isEmpty
                          ? 'Nhập mật khẩu.'
                          : null,
                      onFieldSubmitted: (_) => _login(),
                    ),
                    const SizedBox(height: 16),
                    FilledButton(
                      onPressed: _busy ? null : _login,
                      child: _busy
                          ? const SizedBox.square(
                              dimension: 20,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : const Text('Đăng nhập'),
                    ),
                    if (_mobile) ...[
                      const SizedBox(height: 16),
                      const Row(
                        children: [
                          Expanded(child: Divider()),
                          Padding(
                            padding: EdgeInsets.symmetric(horizontal: 8),
                            child: Text('hoặc'),
                          ),
                          Expanded(child: Divider()),
                        ],
                      ),
                      const SizedBox(height: 12),
                      OutlinedButton.icon(
                        onPressed: _busy ? null : () => _oauth('Google'),
                        icon: const Icon(Icons.g_mobiledata),
                        label: const Text('Đăng nhập bằng Google'),
                      ),
                      const SizedBox(height: 8),
                      OutlinedButton.icon(
                        onPressed: _busy ? null : () => _oauth('GitHub'),
                        icon: const Icon(Icons.code),
                        label: const Text('Đăng nhập bằng GitHub'),
                      ),
                    ],
                    if (!_mobile)
                      const Padding(
                        padding: EdgeInsets.only(top: 18),
                        child: Text(
                          'Trên web/máy tính, hãy dùng email và mật khẩu. Google/GitHub dùng callback ứng dụng chỉ bật trên Android/iOS.',
                          textAlign: TextAlign.center,
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({
    required this.api,
    required this.session,
    required this.onSignedOut,
    super.key,
  });

  final RainmakerApi api;
  final RainmakerSession session;
  final VoidCallback onSignedOut;

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  late final ProvisioningService _provisioning = ProvisioningService(
    widget.api,
  );
  List<RainmakerNode> _nodes = const [];
  Object? _error;
  var _loading = true;

  @override
  void initState() {
    super.initState();
    _loadNodes();
  }

  Future<void> _loadNodes() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final nodes = await widget.api.getNodes(widget.session);
      if (mounted) setState(() => _nodes = nodes);
    } catch (error) {
      if (mounted) setState(() => _error = error);
    } finally {
      if (mounted) setState(() => _loading = false);
    }
  }

  Future<void> _logout() async {
    await widget.api.clearSession();
    widget.onSignedOut();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(AppConfig.appName),
        actions: [
          IconButton(
            onPressed: _loading ? null : _loadNodes,
            icon: const Icon(Icons.refresh),
            tooltip: 'Làm mới',
          ),
          PopupMenuButton<String>(
            onSelected: (value) {
              if (value == 'logout') _logout();
            },
            itemBuilder: (_) => <PopupMenuEntry<String>>[
              PopupMenuItem<String>(
                enabled: false,
                child: Text(
                  widget.session.email.isEmpty
                      ? widget.session.userId
                      : widget.session.email,
                ),
              ),
              const PopupMenuDivider(),
              const PopupMenuItem<String>(
                value: 'logout',
                child: Text('Đăng xuất'),
              ),
            ],
          ),
        ],
      ),
      body: _buildBody(),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () async {
          final result = await Navigator.of(context).push<bool>(
            MaterialPageRoute(
              builder: (_) => ProvisionDevicePage(
                service: _provisioning,
                session: widget.session,
              ),
            ),
          );
          if (result == true) _loadNodes();
        },
        icon: const Icon(Icons.add),
        label: const Text('Thêm thiết bị'),
      ),
    );
  }

  Widget _buildBody() {
    if (_loading) return const Center(child: CircularProgressIndicator());
    if (_error != null) return _RetryView(error: _error!, onRetry: _loadNodes);
    if (_nodes.isEmpty) {
      return const Center(
        child: Padding(
          padding: EdgeInsets.all(32),
          child: Text(
            'Chưa có thiết bị. Nhấn “Thêm thiết bị” để provisioning ESP32.',
          ),
        ),
      );
    }
    return RefreshIndicator(
      onRefresh: _loadNodes,
      child: ListView.separated(
        padding: const EdgeInsets.fromLTRB(16, 16, 16, 92),
        itemCount: _nodes.length,
        separatorBuilder: (_, _) => const SizedBox(height: 10),
        itemBuilder: (context, index) {
          final node = _nodes[index];
          return Card(
            child: ListTile(
              leading: Icon(
                node.online ? Icons.cloud_done : Icons.cloud_off,
                color: node.online ? Colors.green : Colors.grey,
              ),
              title: Text(node.name),
              subtitle: Text('${node.devices.length} thiết bị • ${node.id}'),
              trailing: const Icon(Icons.chevron_right),
              onTap: () => Navigator.of(context).push(
                MaterialPageRoute(
                  builder: (_) => NodeDetailPage(
                    api: widget.api,
                    session: widget.session,
                    node: node,
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}

class NodeDetailPage extends StatefulWidget {
  const NodeDetailPage({
    required this.api,
    required this.session,
    required this.node,
    super.key,
  });

  final RainmakerApi api;
  final RainmakerSession session;
  final RainmakerNode node;

  @override
  State<NodeDetailPage> createState() => _NodeDetailPageState();
}

class _NodeDetailPageState extends State<NodeDetailPage> {
  final Set<String> _updating = <String>{};

  Future<void> _setValue(
    RainmakerDevice device,
    RainmakerParam param,
    dynamic value,
  ) async {
    final key = '${device.name}/${param.name}';
    setState(() => _updating.add(key));
    try {
      await widget.api.updateParameter(
        session: widget.session,
        nodeId: widget.node.id,
        deviceName: device.name,
        parameterName: param.name,
        value: value,
      );
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text('${param.name} đã cập nhật.')));
      }
    } catch (error) {
      if (mounted) _showError(context, error);
    } finally {
      if (mounted) setState(() => _updating.remove(key));
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(widget.node.name)),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text(
            'Node ID: ${widget.node.id}',
            style: Theme.of(context).textTheme.bodySmall,
          ),
          const SizedBox(height: 12),
          if (widget.node.devices.isEmpty) const _EmptyConfigCard(),
          for (final device in widget.node.devices)
            _DevicePanel(device: device, buildParam: _buildParam),
        ],
      ),
    );
  }

  Widget _buildParam(RainmakerDevice device, RainmakerParam param) {
    final key = '${device.name}/${param.name}';
    final updating = _updating.contains(key);
    if (param.isPower) {
      final value = param.value is bool
          ? param.value as bool
          : param.value.toString().toLowerCase() == 'true';
      return SwitchListTile(
        contentPadding: EdgeInsets.zero,
        title: Text(param.name),
        value: value,
        onChanged: !param.isWritable || updating
            ? null
            : (newValue) => _setValue(device, param, newValue),
      );
    }
    if (param.isNumeric) {
      final current =
          (param.value is num ? (param.value as num).toDouble() : param.min)
              .clamp(param.min, param.max);
      return Opacity(
        opacity: param.isWritable ? 1 : .6,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('${param.name}: ${current.round()}'),
            Slider(
              value: current,
              min: param.min,
              max: param.max <= param.min ? param.min + 1 : param.max,
              divisions: (param.max - param.min).abs() <= 360
                  ? (param.max - param.min).round()
                  : null,
              onChanged: !param.isWritable || updating ? null : (_) {},
              onChangeEnd: !param.isWritable || updating
                  ? null
                  : (value) => _setValue(device, param, value.round()),
            ),
          ],
        ),
      );
    }
    return ListTile(
      title: Text(param.name),
      subtitle: Text('${param.value ?? '-'}'),
    );
  }
}

class _DevicePanel extends StatelessWidget {
  const _DevicePanel({required this.device, required this.buildParam});

  final RainmakerDevice device;
  final Widget Function(RainmakerDevice, RainmakerParam) buildParam;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(device.name, style: Theme.of(context).textTheme.titleLarge),
            Text(device.type, style: Theme.of(context).textTheme.bodySmall),
            const Divider(),
            for (final param in device.params) buildParam(device, param),
          ],
        ),
      ),
    );
  }
}

class _EmptyConfigCard extends StatelessWidget {
  const _EmptyConfigCard();

  @override
  Widget build(BuildContext context) => const Card(
    child: Padding(
      padding: EdgeInsets.all(18),
      child: Text(
        'Cloud chưa gửi cấu hình thiết bị. Hãy chờ ESP kết nối Wi-Fi rồi kéo xuống làm mới.',
      ),
    ),
  );
}

class ProvisionDevicePage extends StatefulWidget {
  const ProvisionDevicePage({
    required this.service,
    required this.session,
    super.key,
  });

  final ProvisioningService service;
  final RainmakerSession session;

  @override
  State<ProvisionDevicePage> createState() => _ProvisionDevicePageState();
}

class _ProvisionDevicePageState extends State<ProvisionDevicePage> {
  final _pop = TextEditingController();
  final _ssid = TextEditingController();
  final _password = TextEditingController();
  List<EspBleDevice> _devices = const [];
  List<EspWifiAccessPoint> _networks = const [];
  EspBleDevice? _selected;
  String _status =
      'Nhấn Quét để tìm thiết bị có tên bắt đầu bằng ${AppConfig.bleNamePrefix}.';
  var _busy = false;

  @override
  void dispose() {
    _pop.dispose();
    _ssid.dispose();
    _password.dispose();
    super.dispose();
  }

  Future<void> _scan() async {
    setState(() {
      _busy = true;
      _status = 'Đang quét BLE…';
    });
    try {
      final devices = await widget.service.scan();
      if (mounted) {
        setState(() {
          _devices = devices;
          _status = devices.isEmpty
              ? 'Không tìm thấy ${AppConfig.bleNamePrefix}. Kiểm tra ESP đang ở chế độ provisioning.'
              : 'Chọn ESP cần thiết lập.';
        });
      }
    } catch (error) {
      if (mounted) _showError(context, error);
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _connect(EspBleDevice device) async {
    if (_pop.text.trim().isEmpty) {
      _showError(
        context,
        ProvisioningException(
          'Nhập POP hiển thị trên Serial Monitor hoặc mã QR.',
        ),
      );
      return;
    }
    setState(() {
      _busy = true;
      _status = 'Đang kết nối ${device.name}…';
    });
    try {
      final networks = await widget.service.connectAndGetNetworks(
        deviceName: device.name,
        proofOfPossession: _pop.text.trim(),
      );
      if (mounted) {
        setState(() {
          _selected = device;
          _networks = networks;
          _status = 'Đã kết nối. Chọn hoặc nhập Wi-Fi cho ESP.';
        });
      }
    } catch (error) {
      if (mounted) _showError(context, error);
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _provision() async {
    final device = _selected;
    if (device == null) return;
    setState(() {
      _busy = true;
      _status = 'Đang liên kết tài khoản, gửi Wi-Fi và chờ ESP lên Cloud…';
    });
    try {
      await widget.service.provision(
        session: widget.session,
        deviceName: device.name,
        ssid: _ssid.text,
        password: _password.text,
      );
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text(
            'Đã gửi provisioning. ESP sẽ hiện trong danh sách sau khi kết nối Cloud.',
          ),
        ),
      );
      Navigator.of(context).pop(true);
    } catch (error) {
      if (mounted) _showError(context, error);
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final supported = widget.service.isSupported;
    return Scaffold(
      appBar: AppBar(title: const Text('Thêm ESP32')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          if (!supported)
            const Card(
              child: Padding(
                padding: EdgeInsets.all(16),
                child: Text(
                  'Provisioning Bluetooth yêu cầu Android hoặc iPhone/iPad. Web và máy tính vẫn điều khiển được những ESP đã thêm vào Cloud.',
                ),
              ),
            ),
          Text(_status),
          const SizedBox(height: 16),
          TextField(
            controller: _pop,
            enabled: supported && !_busy,
            decoration: const InputDecoration(
              labelText: 'POP',
              helperText: 'Lấy từ QR/Serial Monitor: POP: ...',
              border: OutlineInputBorder(),
            ),
          ),
          const SizedBox(height: 12),
          FilledButton.icon(
            onPressed: !supported || _busy ? null : _scan,
            icon: const Icon(Icons.bluetooth_searching),
            label: const Text('Quét ESP qua BLE'),
          ),
          if (_devices.isNotEmpty) ...[
            const SizedBox(height: 16),
            Text(
              'Thiết bị tìm thấy',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            for (final device in _devices)
              Card(
                child: ListTile(
                  leading: const Icon(Icons.memory),
                  title: Text(device.name),
                  subtitle: Text('RSSI: ${device.rssi} dBm'),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: _busy ? null : () => _connect(device),
                ),
              ),
          ],
          if (_selected != null) ...[
            const SizedBox(height: 18),
            Text(
              'Wi-Fi cho ${_selected!.name}',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (_networks.isNotEmpty)
              Wrap(
                spacing: 8,
                children: _networks
                    .take(12)
                    .map(
                      (network) => ActionChip(
                        label: Text(network.ssid),
                        onPressed: _busy
                            ? null
                            : () => setState(() => _ssid.text = network.ssid),
                      ),
                    )
                    .toList(),
              ),
            const SizedBox(height: 12),
            TextField(
              controller: _ssid,
              enabled: !_busy,
              decoration: const InputDecoration(
                labelText: 'Tên Wi-Fi (SSID)',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _password,
              enabled: !_busy,
              obscureText: true,
              decoration: const InputDecoration(
                labelText: 'Mật khẩu Wi-Fi',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 16),
            FilledButton.icon(
              onPressed: _busy ? null : _provision,
              icon: const Icon(Icons.wifi),
              label: const Text('Provisioning thiết bị'),
            ),
          ],
          if (_busy)
            const Padding(
              padding: EdgeInsets.only(top: 24),
              child: Center(child: CircularProgressIndicator()),
            ),
        ],
      ),
    );
  }
}

class _RetryView extends StatelessWidget {
  const _RetryView({required this.error, required this.onRetry});
  final Object error;
  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) => Center(
    child: Padding(
      padding: const EdgeInsets.all(28),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.cloud_off, size: 52),
          const SizedBox(height: 12),
          Text(
            'Không tải được thiết bị',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          const SizedBox(height: 8),
          Text(error.toString(), textAlign: TextAlign.center),
          const SizedBox(height: 18),
          FilledButton(onPressed: onRetry, child: const Text('Thử lại')),
        ],
      ),
    ),
  );
}

void _showError(BuildContext context, Object error) {
  ScaffoldMessenger.of(context).showSnackBar(
    SnackBar(
      content: Text(error.toString()),
      behavior: SnackBarBehavior.floating,
    ),
  );
}
