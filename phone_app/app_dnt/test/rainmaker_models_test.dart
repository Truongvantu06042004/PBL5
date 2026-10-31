import 'package:flutter_test/flutter_test.dart';
import 'package:rainmaker/rainmaker_models.dart';

void main() {
  test('parses the Light node emitted by this ESP RainMaker firmware', () {
    final node = RainmakerNode.fromJson(<String, dynamic>{
      'id': 'LSF8MQvm82TohaJSX77qUs',
      'config': <String, dynamic>{
        'name': 'ESP32-C3',
        'devices': <dynamic>[
          <String, dynamic>{
            'name': 'Light',
            'type': 'esp.device.lightbulb',
            'params': <dynamic>[
              <String, dynamic>{
                'name': 'Power',
                'type': 'esp.param.power',
                'data_type': 'bool',
                'properties': <dynamic>['read', 'write'],
              },
              <String, dynamic>{
                'name': 'Brightness',
                'data_type': 'int',
                'bounds': <String, dynamic>{'min': 0, 'max': 100},
                'properties': <dynamic>['read', 'write'],
              },
            ],
          },
        ],
      },
      'params': <String, dynamic>{
        'Light': <String, dynamic>{'Power': true, 'Brightness': 65},
      },
    });

    expect(node.id, 'LSF8MQvm82TohaJSX77qUs');
    expect(node.devices.single.name, 'Light');
    expect(node.devices.single.params.first.isPower, isTrue);
    expect(node.devices.single.params.last.value, 65);
  });
}
