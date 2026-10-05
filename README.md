# flutter_native_speech_to_text

[![pub package](https://img.shields.io/pub/v/flutter_native_speech_to_text.svg)](https://pub.dev/packages/flutter_native_speech_to_text)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

A lightweight Flutter plugin for speech-to-text recognition leveraging Android's native `RecognizerIntent.ACTION_RECOGNIZE_SPEECH`.

---

## Features

- 🎯 **Native Android Integration**: Direct access to Android's built-in `RecognizerIntent.ACTION_RECOGNIZE_SPEECH` dialog.
- ⚙️ **Fully Configurable**: Exposes all major `RecognizerIntent` extras as typed Dart parameters.
- 🌐 **Multi-language Support**: Specify custom language codes (e.g., `en-US`, `bn-BD`, `es-ES`).
- 📊 **Detailed Results**: Access primary transcription, alternate transcript hypotheses, and confidence scores.
- ⏱️ **Silence & Length Controls**: Fine-tune speech input minimum length and silence thresholds.
- 🛡️ **Zero Bloat**: Lightweight implementation using Android system speech services without heavy third-party models.

---

## Platform Support

| Platform | Supported | Notes |
|:---:|:---:|:---|
| Android | ✅ | Requires device with Google Play Services or Speech Recognizer installed (API 24+) |
| iOS | ⚠️ | Safe stub only: never crashes, reports a `PLATFORM_NOT_SUPPORTED` error instead (see [iOS behaviour](#ios-behaviour)) |
| Web / Desktop | ❌ | Not currently supported |

---

## Installation

Add `flutter_native_speech_to_text` to your `pubspec.yaml`:

```yaml
dependencies:
  flutter_native_speech_to_text: ^1.0.0
```

Or run:

```bash
flutter pub add flutter_native_speech_to_text
```

---

## Android Setup

### 1. Permissions & Queries

Add the following permissions and `<queries>` declaration to your `android/app/src/main/AndroidManifest.xml` (inside the `<manifest>` tag):

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">

    <!-- Permissions required for speech recognition -->
    <uses-permission android:name="android.permission.RECORD_AUDIO" />
    <uses-permission android:name="android.permission.INTERNET" />

    <!-- Queries declaration for Android 11+ (API 30+) package visibility -->
    <queries>
        <intent>
            <action android:name="android.speech.action.RECOGNIZE_SPEECH" />
        </intent>
    </queries>

    <application ...>
        ...
    </application>
</manifest>
```

> **Note on Android 6.0+ (API 23+)**: Make sure your application requests the runtime `RECORD_AUDIO` permission before initiating speech recognition if needed, or handle permission callbacks accordingly.

---

## Usage

### 1. Check Availability

Before starting recognition, check if a speech recognizer service is available on the user's device:

```dart
import 'package:flutter_native_speech_to_text/flutter_native_speech_to_text.dart';

final bool isAvailable = await FlutterNativeSpeechToText.isAvailable();

if (!isAvailable) {
  print('Speech recognition is not available on this device.');
}
```

### 2. Start Speech Recognition

Invoke `FlutterNativeSpeechToText.listen()` to open Android's native speech recognition dialog:

```dart
try {
  final SpeechResult result = await FlutterNativeSpeechToText.listen(
    language: 'en-US',
    languageModel: 'free_form', // 'free_form' or 'web_search'
    prompt: 'Say something...',
    maxResults: 5,
    partialResults: true,
    confidenceScores: true,
    speechInputMinimumLengthMillis: 2000,
    speechInputCompleteSilenceLengthMillis: 1500,
    speechInputPossiblyCompleteSilenceLengthMillis: 500,
  );

  print('Top Result: ${result.recognizedText}');
  print('Alternatives: ${result.alternativeResults}');
  print('Confidence Scores: ${result.confidenceScores}');
} on SpeechToTextException catch (e) {
  print('Error code: ${e.code}, message: ${e.message}');
}
```

### 3. Stop or Cancel Recognition

```dart
// Stop listening
await FlutterNativeSpeechToText.stop();

// Cancel listening session
await FlutterNativeSpeechToText.cancel();
```

---

## API & Parameter Reference

### `FlutterNativeSpeechToText.listen()` Parameters

| Parameter | RecognizerIntent Equivalent | Type | Description |
|-----------|----------------------------|------|-------------|
| `language` | `EXTRA_LANGUAGE` | `String?` | Recognition language tag (e.g. `'en-US'`, `'bn-BD'`). Defaults to `'bn-BD'`. |
| `languagePreference` | `EXTRA_LANGUAGE_PREFERENCE` | `String?` | Preferred language for recognition. |
| `languageModel` | `EXTRA_LANGUAGE_MODEL` | `String?` | `'free_form'` (default) or `'web_search'`. |
| `prompt` | `EXTRA_PROMPT` | `String?` | Prompt text displayed to the user in the system dialog. |
| `maxResults` | `EXTRA_MAX_RESULTS` | `int?` | Maximum number of alternative recognition results. |
| `partialResults` | `EXTRA_PARTIAL_RESULTS` | `bool?` | Request partial recognition results. |
| `confidenceScores` | `EXTRA_CONFIDENCE_SCORES` | `bool?` | Request confidence scores for transcriptions. |
| `speechInputMinimumLengthMillis` | `EXTRA_SPEECH_INPUT_MINIMUM_LENGTH_MILLIS` | `int?` | Minimum duration in milliseconds to consider speech input. |
| `speechInputCompleteSilenceLengthMillis` | `EXTRA_SPEECH_INPUT_COMPLETE_SILENCE_LENGTH_MILLIS` | `int?` | Silence duration to consider speech complete. |
| `speechInputPossiblyCompleteSilenceLengthMillis` | `EXTRA_SPEECH_INPUT_POSSIBLY_COMPLETE_SILENCE_LENGTH_MILLIS` | `int?` | Silence duration to consider speech possibly complete. |
| `getAudio` | `EXTRA_GET_AUDIO` | `bool?` | Whether to request recorded audio data. |
| `getAudioFormat` | `EXTRA_GET_AUDIO_FORMAT` | `String?` | MIME type format for requested audio (e.g., `'audio/AMR'`). |
| `originatorPackage` | `EXTRA_ORIGINATOR_PKG` | `String?` | Calling package name. |
| `isEnable` | N/A | `bool` | Set to `false` to disable recognition (throws error). Defaults to `true`. |

*Note: Unspecified parameters will use Android system defaults.*

---

## Result Model (`SpeechResult`)

| Property | Type | Description |
|----------|------|-------------|
| `recognizedText` | `String?` | The most probable recognized text string. |
| `alternativeResults` | `List<String>?` | List of alternative transcription hypotheses. |
| `confidenceScores` | `List<double>?` | Confidence scores corresponding to alternative results (0.0 to 1.0). |
| `isFinal` | `bool` | Indicates whether the result is final. |

---

## Error Codes

When a `SpeechToTextException` is thrown, the `code` property provides specific failure reasons:

| Code | Reason |
|------|--------|
| `NO_RECOGNIZER` | No speech recognizer service is installed or active on the device. |
| `NO_CONTEXT` | Android activity context was null or detached. |
| `ALREADY_LISTENING` | A speech recognition session is already in progress. |
| `DISABLED` | The `isEnable` flag was passed as `false`. |
| `ERROR` | An exception occurred while launching the intent. |
| `PLATFORM_NOT_SUPPORTED` | The method was called on iOS, which is not supported yet (no crash, just this error). |
| `MISSING_PLUGIN` | The native side of the plugin is not registered in the current binary (e.g. plugin missing from a release build). |

---

## iOS behaviour

The package ships with an iOS entry point purely so that iOS builds of your app
**link and run normally instead of crashing** (a missing plugin registration on
iOS causes `MissingPluginException`, and any native speech call would abort).

On iOS every call fails *gracefully*:

| Call | iOS result |
|------|------------|
| `FlutterNativeSpeechToText.isAvailable()` | returns `false` (so UIs can disable the mic button) |
| `FlutterNativeSpeechToText.listen(...)` | throws `SpeechToTextException(code: 'PLATFORM_NOT_SUPPORTED')` |
| `FlutterNativeSpeechToText.stop()` / `.cancel()` | throws `SpeechToTextException(code: 'PLATFORM_NOT_SUPPORTED')` |

Nothing is executed natively on iOS - no microphone permission is requested, no
`Info.plist` keys are required, and no Objective-C/Swift exception can escape.
Errors are converted to a normal Dart exception and printed with `debugPrint`,
so you can show and log them:

```dart
try {
  final result = await FlutterNativeSpeechToText.listen();
} on SpeechToTextException catch (e) {
  debugPrint('Speech error: ${e.message} (code: ${e.code})');
  ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(e.message)));
}
```

---

## Example

For a complete working sample application, check out the [example](example) folder in this repository.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.