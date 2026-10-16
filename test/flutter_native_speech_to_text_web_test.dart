@TestOn('browser')
library;

import 'package:flutter/services.dart';
import 'package:flutter_native_speech_to_text/flutter_native_speech_to_text_web.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  final web = FlutterNativeSpeechToTextWeb();

  test('isAvailable returns a bool without throwing', () async {
    expect(await web.handleMethodCall(const MethodCall('isAvailable')),
        isA<bool>());
  });

  test('listen with isEnable false throws DISABLED', () {
    expect(
      web.handleMethodCall(const MethodCall('listen', {'isEnable': false})),
      throwsA(
          isA<PlatformException>().having((e) => e.code, 'code', 'DISABLED')),
    );
  });

  test('stop and cancel are safe when idle', () async {
    expect(await web.handleMethodCall(const MethodCall('stop')), isNull);
    expect(await web.handleMethodCall(const MethodCall('cancel')), isNull);
  });

  test('unknown methods throw UNIMPLEMENTED', () {
    expect(
      web.handleMethodCall(const MethodCall('nope')),
      throwsA(isA<PlatformException>()
          .having((e) => e.code, 'code', 'UNIMPLEMENTED')),
    );
  });
}
