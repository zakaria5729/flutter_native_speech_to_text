import 'dart:async';
import 'dart:js_interop';
import 'dart:js_interop_unsafe';

import 'package:flutter/services.dart';
import 'package:flutter_web_plugins/flutter_web_plugins.dart';

class FlutterNativeSpeechToTextWeb {
  static const String _defaultLanguage = 'bn-BD';

  _SpeechRecognition? _recognition;
  Completer<Map<String, dynamic>?>? _pending;
  Timer? _stopTimer;
  void Function()? _forceFinish;

  static void registerWith(Registrar registrar) {
    final channel = MethodChannel(
      'com.zakaria5729/flutter_native_speech_to_text',
      const StandardMethodCodec(),
      registrar,
    );
    channel
        .setMethodCallHandler(FlutterNativeSpeechToTextWeb().handleMethodCall);
  }

  Future<dynamic> handleMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'isAvailable':
        return _constructor() != null;
      case 'listen':
        final args = (call.arguments as Map?)?.cast<String, dynamic>() ??
            const <String, dynamic>{};
        return _listen(args);
      case 'stop':
        final recognition = _recognition;
        if (recognition != null) {
          recognition.stop();
          _stopTimer?.cancel();
          _stopTimer =
              Timer(const Duration(seconds: 3), () => _forceFinish?.call());
        }
        return null;
      case 'cancel':
        _cancel();
        return null;
      default:
        throw PlatformException(
          code: 'UNIMPLEMENTED',
          message: "Method '${call.method}' is not implemented on web",
        );
    }
  }

  static JSFunction? _constructor() {
    for (final name in const ['SpeechRecognition', 'webkitSpeechRecognition']) {
      if (globalContext.has(name)) {
        final value = globalContext.getProperty<JSAny?>(name.toJS);
        if (value.isA<JSFunction>()) return value as JSFunction;
      }
    }
    return null;
  }

  Future<Map<String, dynamic>?> _listen(Map<String, dynamic> args) {
    if (args['isEnable'] == false) {
      throw PlatformException(
        code: 'DISABLED',
        message: 'Speech to text recognition unavailable',
      );
    }
    if (_pending != null) {
      throw PlatformException(
        code: 'ALREADY_LISTENING',
        message: 'A recognition session is already in progress',
      );
    }
    final constructor = _constructor();
    if (constructor == null) {
      throw PlatformException(
        code: 'NO_RECOGNIZER',
        message: 'This browser does not support the Web Speech API. '
            'Use a Chromium-based browser or Safari.',
      );
    }

    final recognition = constructor.callAsConstructor<_SpeechRecognition>();
    final completer = Completer<Map<String, dynamic>?>();
    _recognition = recognition;
    _pending = completer;

    Map<String, dynamic>? latest;
    PlatformException? failure;

    void finish() {
      _stopTimer?.cancel();
      _stopTimer = null;
      _forceFinish = null;
      if (identical(_pending, completer)) {
        _pending = null;
        _recognition = null;
      }
      if (completer.isCompleted) return;
      final error = failure;
      if (error != null) {
        completer.completeError(error);
      } else {
        completer.complete(latest);
      }
    }

    _forceFinish = () {
      try {
        recognition.abort();
      } catch (_) {}
      finish();
    };

    recognition
      ..lang = (args['language'] ??
          args['languagePreference'] ??
          _defaultLanguage) as String
      ..continuous = false
      ..interimResults = args['partialResults'] == true
      ..maxAlternatives = (args['maxResults'] as num?)?.toInt() ?? 1
      ..onresult = ((_SpeechRecognitionEvent event) {
        latest = _toResultMap(event.results);
      }).toJS
      ..onerror = ((_SpeechRecognitionErrorEvent event) {
        failure = _mapError(event.error, event.message);
        _forceFinish?.call();
      }).toJS
      ..onend = finish.toJS;

    try {
      recognition.start();
    } catch (e) {
      finish();
      throw PlatformException(
        code: 'ERROR',
        message: 'Failed to start speech recognition: $e',
      );
    }
    return completer.future;
  }

  void _cancel() {
    final completer = _pending;
    final recognition = _recognition;
    _stopTimer?.cancel();
    _stopTimer = null;
    _forceFinish = null;
    _pending = null;
    _recognition = null;
    if (completer != null && !completer.isCompleted) completer.complete(null);
    recognition?.abort();
  }

  static PlatformException? _mapError(String error, String? message) {
    switch (error) {
      case 'no-speech':
      case 'aborted':
        return null;
      case 'not-allowed':
        return PlatformException(
          code: 'PERMISSION_DENIED',
          message: 'Microphone permission was denied for this site',
        );
      case 'service-not-allowed':
        return PlatformException(
          code: 'SERVICE_NOT_ALLOWED',
          message: 'The browser blocked its speech recognition service. '
              'Enable Dictation in system settings, install the requested '
              'language there, and allow speech recognition for this site.',
        );
      case 'audio-capture':
        return PlatformException(
          code: 'NO_MICROPHONE',
          message: 'No microphone was found or it could not be used',
        );
      case 'network':
        return PlatformException(
          code: 'NETWORK',
          message: 'A network error occurred during speech recognition',
        );
      case 'language-not-supported':
        return PlatformException(
          code: 'LANGUAGE_NOT_SUPPORTED',
          message: 'The requested language is not supported',
        );
      default:
        return PlatformException(
          code: 'ERROR',
          message: (message != null && message.isNotEmpty) ? message : error,
        );
    }
  }

  static Map<String, dynamic>? _toResultMap(_SpeechRecognitionResultList list) {
    final finals = <_SpeechRecognitionResult>[];
    final all = <_SpeechRecognitionResult>[];
    for (var i = 0; i < list.length; i++) {
      final result = list.item(i);
      all.add(result);
      if (result.isFinal) finals.add(result);
    }
    final isFinal = finals.isNotEmpty;
    final used = isFinal ? finals : all;
    if (used.isEmpty) return null;

    final text = used.map((r) => r.item(0).transcript).join(' ').trim();
    List<String> alternatives;
    List<double> confidences;
    if (used.length == 1) {
      final result = used.first;
      alternatives = [
        for (var i = 0; i < result.length; i++) result.item(i).transcript,
      ];
      confidences = [
        for (var i = 0; i < result.length; i++) result.item(i).confidence,
      ];
    } else {
      alternatives = [text];
      confidences = [
        used.map((r) => r.item(0).confidence).reduce((a, b) => a + b) /
            used.length,
      ];
    }
    return {
      'recognizedText': text,
      'alternativeResults': alternatives,
      'confidenceScores': confidences,
      'isFinal': isFinal,
    };
  }
}

extension type _SpeechRecognition._(JSObject _) implements JSObject {
  external set lang(String value);
  external set continuous(bool value);
  external set interimResults(bool value);
  external set maxAlternatives(int value);
  external set onresult(JSFunction? value);
  external set onerror(JSFunction? value);
  external set onend(JSFunction? value);
  external void start();
  external void stop();
  external void abort();
}

extension type _SpeechRecognitionEvent._(JSObject _) implements JSObject {
  external _SpeechRecognitionResultList get results;
}

extension type _SpeechRecognitionErrorEvent._(JSObject _) implements JSObject {
  external String get error;
  external String? get message;
}

extension type _SpeechRecognitionResultList._(JSObject _) implements JSObject {
  external int get length;
  external _SpeechRecognitionResult item(int index);
}

extension type _SpeechRecognitionResult._(JSObject _) implements JSObject {
  external int get length;
  external bool get isFinal;
  external _SpeechRecognitionAlternative item(int index);
}

extension type _SpeechRecognitionAlternative._(JSObject _) implements JSObject {
  external String get transcript;
  external double get confidence;
}
