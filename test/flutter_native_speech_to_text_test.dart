import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_native_speech_to_text/flutter_native_speech_to_text.dart';

void main() {
  group('FlutterNativeSpeechToText', () {
    test('SpeechResult.fromMap creates correct object', () {
      final map = {
        'recognizedText': 'Hello world',
        'alternativeResults': ['Hello world', 'Hello word'],
        'confidenceScores': [0.9, 0.7],
        'isFinal': true,
      };

      final result = SpeechResult.fromMap(map);

      expect(result.recognizedText, 'Hello world');
      expect(result.alternativeResults, ['Hello world', 'Hello word']);
      expect(result.confidenceScores, [0.9, 0.7]);
      expect(result.isFinal, true);
    });

    test('SpeechResult.fromMap handles null values', () {
      final map = <String, dynamic>{};

      final result = SpeechResult.fromMap(map);

      expect(result.recognizedText, isNull);
      expect(result.alternativeResults, isNull);
      expect(result.confidenceScores, isNull);
      expect(result.isFinal, true);
    });

    test('SpeechToTextException contains message and code', () {
      const exception = SpeechToTextException('Test error', code: 'TEST_CODE');

      expect(exception.message, 'Test error');
      expect(exception.code, 'TEST_CODE');
      expect(exception.toString(),
          'SpeechToTextException: Test error (code: TEST_CODE)');
    });

    test('SpeechToTextException works without code', () {
      const exception = SpeechToTextException('Test error');

      expect(exception.message, 'Test error');
      expect(exception.code, isNull);
      expect(exception.toString(), 'SpeechToTextException: Test error');
    });
  });
}
