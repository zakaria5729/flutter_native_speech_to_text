/// A Flutter plugin for native speech-to-text using Android's RecognizerIntent ACTION_RECOGNIZE_SPEECH.
library flutter_native_speech_to_text;

import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

/// The main class for interacting with the native speech-to-text functionality.
class FlutterNativeSpeechToText {
  static const MethodChannel _channel =
      MethodChannel('com.zakaria5729/flutter_native_speech_to_text');

  /// Checks if speech recognition is available on the device.
  static Future<bool> isAvailable() async {
    try {
      final bool? result = await _channel.invokeMethod<bool>('isAvailable');
      return result ?? false;
    } on PlatformException catch (e) {
      debugPrint('Error checking availability: ${e.message}');
      return false;
    }
  }

  /// Starts listening for speech input using Android's native RecognizerIntent.
  ///
  /// Returns the recognized [SpeechResult] on success, or throws [SpeechToTextException] on failure.
  ///
  /// Parameters:
  /// - [languagePreference]: Preferred language for recognition (e.g., "en-US", "bn-BD").
  /// - [partialResults]: Whether to return partial results.
  /// - [confidenceScores]: Whether to return confidence scores.
  /// - [originatorPackage]: Package name of the calling app.
  /// - [speechInputMinimumLengthMillis]: Minimum length of speech input in milliseconds.
  /// - [speechInputCompleteSilenceLengthMillis]: Silence duration to consider speech complete.
  /// - [speechInputPossiblyCompleteSilenceLengthMillis]: Silence duration to consider speech possibly complete.
  /// - [getAudioFormat]: Optional audio format MIME type (e.g. "audio/AMR") when requesting audio.
  /// - [getAudio]: Whether to return audio data.
  /// - [languageModel]: Language model to use, "free_form" or "web_search".
  ///   Defaults to "free_form" (`RecognizerIntent.LANGUAGE_MODEL_FREE_FORM`).
  /// - [language]: Language code for recognition. Defaults to "bn-BD" (`RecognizerIntent.EXTRA_LANGUAGE`) when omitted.
  /// - [prompt]: Prompt to show to the user.
  /// - [maxResults]: Maximum number of results to return.
  /// - [isEnable]: If false, returns error "Speech to text recognition unavailable".
  static Future<SpeechResult> listen({
    String? languagePreference,
    bool? partialResults,
    bool? confidenceScores,
    String? originatorPackage,
    int? speechInputMinimumLengthMillis,
    int? speechInputCompleteSilenceLengthMillis,
    int? speechInputPossiblyCompleteSilenceLengthMillis,
    String? getAudioFormat,
    bool? getAudio,
    String? languageModel,
    String? language,
    String? prompt,
    int? maxResults,
    bool isEnable = true,
  }) async {
    if (!isEnable) {
      throw const SpeechToTextException(
          'Speech to text recognition unavailable');
    }

    try {
      final Map<String, dynamic> args = {
        if (languagePreference != null)
          'languagePreference': languagePreference,
        if (partialResults != null) 'partialResults': partialResults,
        if (confidenceScores != null) 'confidenceScores': confidenceScores,
        if (originatorPackage != null) 'originatorPackage': originatorPackage,
        if (speechInputMinimumLengthMillis != null)
          'speechInputMinimumLengthMillis': speechInputMinimumLengthMillis,
        if (speechInputCompleteSilenceLengthMillis != null)
          'speechInputCompleteSilenceLengthMillis':
              speechInputCompleteSilenceLengthMillis,
        if (speechInputPossiblyCompleteSilenceLengthMillis != null)
          'speechInputPossiblyCompleteSilenceLengthMillis':
              speechInputPossiblyCompleteSilenceLengthMillis,
        if (getAudioFormat != null) 'getAudioFormat': getAudioFormat,
        if (getAudio != null) 'getAudio': getAudio,
        if (languageModel != null) 'languageModel': languageModel,
        if (language != null) 'language': language,
        if (prompt != null) 'prompt': prompt,
        if (maxResults != null) 'maxResults': maxResults,
        'isEnable': isEnable,
      };

      final Map<String, dynamic>? result =
          await _channel.invokeMapMethod<String, dynamic>('listen', args);
      if (result == null) {
        return const SpeechResult(isFinal: true);
      }
      return SpeechResult.fromMap(result);
    } on PlatformException catch (e) {
      throw SpeechToTextException(e.message ?? 'Unknown error', code: e.code);
    }
  }

  /// Stops the current speech recognition session.
  static Future<void> stop() async {
    try {
      await _channel.invokeMethod('stop');
    } on PlatformException catch (e) {
      throw SpeechToTextException(e.message ?? 'Failed to stop', code: e.code);
    }
  }

  /// Cancels the current speech recognition session.
  static Future<void> cancel() async {
    try {
      await _channel.invokeMethod('cancel');
    } on PlatformException catch (e) {
      throw SpeechToTextException(e.message ?? 'Failed to cancel',
          code: e.code);
    }
  }
}

/// Backward compatibility alias for [FlutterNativeSpeechToText].
typedef FlutterSpeechToText = FlutterNativeSpeechToText;

/// Represents the result of a speech recognition operation.
class SpeechResult {
  final String? recognizedText;
  final List<String>? alternativeResults;
  final List<double>? confidenceScores;
  final bool isFinal;
  final Map<String, dynamic>? audioData;

  const SpeechResult({
    this.recognizedText,
    this.alternativeResults,
    this.confidenceScores,
    this.isFinal = true,
    this.audioData,
  });

  factory SpeechResult.fromMap(Map<String, dynamic> map) {
    return SpeechResult(
      recognizedText: map['recognizedText'] as String?,
      alternativeResults:
          (map['alternativeResults'] as List<dynamic>?)?.cast<String>(),
      confidenceScores: (map['confidenceScores'] as List<dynamic>?)
          ?.map((e) => (e as num).toDouble())
          .toList(),
      isFinal: map['isFinal'] as bool? ?? true,
      audioData:
          (map['audioData'] as Map<dynamic, dynamic>?)?.cast<String, dynamic>(),
    );
  }

  @override
  String toString() {
    return 'SpeechResult(recognizedText: $recognizedText, alternativeResults: $alternativeResults, confidenceScores: $confidenceScores, isFinal: $isFinal)';
  }
}

/// Exception class for speech-to-text errors.
class SpeechToTextException implements Exception {
  final String message;
  final String? code;

  const SpeechToTextException(this.message, {this.code});

  @override
  String toString() =>
      'SpeechToTextException: $message${code != null ? ' (code: $code)' : ''}';
}
