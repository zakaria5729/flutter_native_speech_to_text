# flutter_native_speech_to_text

[![pub package](https://img.shields.io/pub/v/flutter_native_speech_to_text.svg)](https://pub.dev/packages/flutter_native_speech_to_text)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

A lightweight Flutter plugin for speech-to-text recognition with one Dart API on every platform: Android's native `RecognizerIntent.ACTION_RECOGNIZE_SPEECH`, Apple's Speech framework on iOS and macOS, the Windows speech recognizer, Vosk on Linux, and the browser's Web Speech API on web.

🚀 **[Live web demo](https://zakaria5729.github.io/flutter_native_speech_to_text/)** — try the example app directly in your browser, with a Bangla/English language switch. Tested in Chrome; see [Web behaviour](#web-behaviour) for other browsers.

---

## Features

- 🎯 **Native Android Integration**: Direct access to Android's built-in `RecognizerIntent.ACTION_RECOGNIZE_SPEECH` dialog.
- 🍎 **iOS and macOS**: Uses Apple's Speech framework, with no extra dependencies.
- 🌍 **Web Support**: Runs in Chrome, Edge and Safari through the Web Speech API with the same Dart API.
- 🔎 **Language Check**: `isLanguageSupported()` tells you whether a language works on the current platform.
- 🖥️ **Desktop**: Windows uses the built-in recognizer; Linux uses Vosk when installed.
- ⚙️ **Fully Configurable**: Exposes all major `RecognizerIntent` extras as typed Dart parameters.
- 🌐 **Multi-language Support**: Specify custom language codes (e.g., `en-US`, `bn-BD`, `es-ES`).
- 📊 **Detailed Results**: Access primary transcription, alternate transcript hypotheses, and confidence scores.
- ⏱️ **Silence & Length Controls**: Fine-tune speech input minimum length and silence thresholds.
- 🪶 **Zero Bloat**: Lightweight implementation using each platform's own speech services, without heavy third-party models.

---

## Platform Support

| Platform | Supported | Notes |
|:---:|:---:|:---|
| Android | ✅ | Requires device with Google Play Services or Speech Recognizer installed (API 24+) |
| iOS | ✅ | Apple Speech framework; Bengali is not supported by Apple (see [iOS and macOS](#ios-and-macos)) |
| macOS | ✅ | Apple Speech framework (see [iOS and macOS](#ios-and-macos)) |
| Web | ✅ | Uses the browser's Web Speech API (see [Web behaviour](#web-behaviour)) |
| Windows | ⚠️ | Windows speech recognizer (untested, see [Windows](#windows)) |
| Linux | ⚠️ | Vosk + PulseAudio, loaded at runtime (untested, see [Linux](#linux)) |

---

## Installation

Add `flutter_native_speech_to_text` to your `pubspec.yaml`:

```yaml
dependencies:
  flutter_native_speech_to_text: ^1.3.0
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

### 2. Check Language Support (optional)

```dart
final bool ok = await FlutterNativeSpeechToText.isLanguageSupported('en-US');
```

See [Checking language support](#checking-language-support) for how each platform answers.

### 3. Start Speech Recognition

Invoke `FlutterNativeSpeechToText.listen()`. Android opens the system speech dialog; iOS, macOS and web record without showing a dialog. Pass `language` explicitly on iOS, macOS and web if you don't use the default `bn-BD`:

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

### 4. Stop or Cancel Recognition

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

*Note: Unspecified parameters use each platform's defaults. Parameters a platform does not support are ignored (see the web, iOS and macOS sections).*

---

## Checking language support

Use `isLanguageSupported` before `listen()` to find out whether a language works on the
current platform:

```dart
if (await FlutterNativeSpeechToText.isLanguageSupported('bn-BD')) {
  final result = await FlutterNativeSpeechToText.listen(language: 'bn-BD');
} else {
  // Fall back to another language or show a message.
}
```

| Platform | Result |
|----------|--------|
| iOS / macOS | `true` only for languages in Apple's supported list (`bn-BD` is `false`) |
| Android | Uses the recognizer's language list. If the device reports none, returns `true` |
| Web | `true` when the browser has speech recognition. Browsers do not list languages, so an unsupported language is reported by `listen()` instead |
| Windows | `true` for languages in the Windows speech recognizer's list |
| Linux | `true` when a model folder for the language exists in `VOSK_MODEL_PATH` |

A bare code such as `en` matches any region (`en-US`, `en-GB`, ...).

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
| `NO_RECOGNIZER` | No speech recognizer service is installed or active on the device (on Linux, Vosk or PulseAudio is missing). |
| `NO_CONTEXT` | Android activity context was null or detached. |
| `ALREADY_LISTENING` | A speech recognition session is already in progress. |
| `DISABLED` | The `isEnable` flag was passed as `false`. |
| `ERROR` | An exception occurred while launching the intent. |
| `PLATFORM_NOT_SUPPORTED` | The platform has no implementation. |
| `PERMISSION_DENIED` | *(Web)* Microphone permission was denied. |
| `SERVICE_NOT_ALLOWED` | *(Web)* The browser blocked its speech service, for example Safari with Dictation off or the language not installed. |
| `NO_MICROPHONE` | *(Web)* No usable microphone was found. |
| `NETWORK` | *(Web)* The browser's speech service could not be reached. |
| `LANGUAGE_NOT_SUPPORTED` | *(Web, iOS, macOS)* The requested `language` is not supported by the engine. |
| `MISSING_PLUGIN` | The native side of the plugin is not registered in the current binary (e.g. plugin missing from a release build). |

---

## Web behaviour

On web the plugin uses the browser's [Web Speech API](https://developer.mozilla.org/docs/Web/API/SpeechRecognition).
No setup or extra dependencies are needed - the same `listen()`, `stop()`, `cancel()`
and `isAvailable()` calls work unchanged.

> **Note:** The [live web demo](https://zakaria5729.github.io/flutter_native_speech_to_text/) has been tested in **Chrome**. For other browsers' support, see below.

- **Browser support**:
  - Chrome and Edge: supported, including languages such as `bn-BD`.
  - Safari: supported, but it uses the macOS speech engine, so Dictation must be
    turned on and the requested `language` must be installed under System Settings
    > Keyboard > Dictation. Otherwise `listen()` throws `SERVICE_NOT_ALLOWED`.
  - Brave: blocks the speech service, so `isAvailable()` returns `false` and
    `listen()` throws `NO_RECOGNIZER`.
  - Firefox: does not implement speech recognition, so `isAvailable()` returns
    `false` and `listen()` throws `NO_RECOGNIZER`.
- **Secure context**: the page must be served over HTTPS (or `localhost`), and the
  user must grant microphone permission when prompted.
- **Server-side recognition**: Chrome sends audio to Google's servers, so an internet
  connection is required.
- **Single utterance**: like Android, `listen()` completes after one utterance (or
  when you call `stop()`); `cancel()` completes it with an empty result.
- **Supported parameters**: `language` / `languagePreference` (default `bn-BD`),
  `partialResults` (enables interim hypotheses internally), `maxResults`
  (alternatives count) and `isEnable`. Android-only options (`prompt`, silence
  lengths, `getAudio`, ...) are ignored.
- `confidenceScores` and `alternativeResults` are filled from the browser's result.

---

## iOS and macOS

On iOS and macOS the plugin uses Apple's Speech framework (`SFSpeechRecognizer`) and
the microphone. No dialog is shown: `listen()` starts recording, stops after about two
seconds of silence (`speechInputCompleteSilenceLengthMillis`), and returns the result.
`stop()` ends the session early and `cancel()` completes it with an empty result.

**iOS** (`ios/Runner/Info.plist`):

```xml
<key>NSMicrophoneUsageDescription</key>
<string>Used to recognise your speech.</string>
<key>NSSpeechRecognitionUsageDescription</key>
<string>Used to convert your speech to text.</string>
```

**macOS**: add the same two keys to `macos/Runner/Info.plist`, and these entitlements
to `DebugProfile.entitlements` and `Release.entitlements`:

```xml
<key>com.apple.security.device.audio-input</key>
<true/>
<key>com.apple.security.network.client</key>
<true/>
```

Notes:

- **Languages**: only the languages Apple supports are available. Bengali (`bn-BD`) is
  **not** supported by Apple's speech engine, so `listen()` throws `LANGUAGE_NOT_SUPPORTED`
  unless you pass another `language`, for example `en-US`. The default language is `bn-BD`,
  so pass `language` explicitly on Apple platforms.
- **Supported parameters**: `language` / `languagePreference`, `maxResults`,
  `speechInputCompleteSilenceLengthMillis` and `isEnable`. Other Android-only options are
  ignored.
- Some languages need an internet connection for recognition.

## Windows

Uses the built-in Windows speech recognizer (`Windows.Media.SpeechRecognition`). No dialog
is shown. `listen()` records until about five seconds of silence, or the value of
`speechInputCompleteSilenceLengthMillis`, then returns the result.

- Turn on **Settings > Privacy & security > Speech > Online speech recognition**, otherwise
  `listen()` throws `PERMISSION_DENIED`.
- Allow desktop apps to use the microphone in **Settings > Privacy & security > Microphone**.
- Only the languages installed for Windows speech recognition work. Other languages, which
  may include Bengali, throw `LANGUAGE_NOT_SUPPORTED`.
- Supported parameters: `language` / `languagePreference`, `maxResults`,
  `speechInputCompleteSilenceLengthMillis` and `isEnable`.
- This implementation has not been tested on a Windows machine yet.

## Linux

Linux has no built-in speech engine, so the plugin uses [Vosk](https://alphacephei.com/vosk/)
for recognition and PulseAudio for the microphone. Both are loaded at runtime, so your app
builds and runs without them and only needs them for recognition:

1. Install `libvosk.so` (so the loader can find it) and `libpulse-simple` (for example the
   `libpulse0` package).
2. Download Vosk models and put each in its own folder named after its language tag under one
   directory, for example `models/en-us` and `models/bn-bd`.
3. Set the `VOSK_MODEL_PATH` environment variable to that directory.

Without these, `isAvailable()` returns `false` and `listen()` throws `NO_RECOGNIZER`. A language
with no model folder throws `LANGUAGE_NOT_SUPPORTED`. Recognition ends when Vosk detects the
end of an utterance, so `speechInputCompleteSilenceLengthMillis` is ignored, and
`confidenceScores` is not filled. Vosk's available languages depend on the models you download.
This implementation has not been tested on Linux yet.

---

## Example

For a complete working sample application, check out the [example](example) folder in this repository.

---

## Author

Created and maintained by [zakaria5729](https://github.com/zakaria5729). Issues and pull requests are welcome on [GitHub](https://github.com/zakaria5729/flutter_native_speech_to_text).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.