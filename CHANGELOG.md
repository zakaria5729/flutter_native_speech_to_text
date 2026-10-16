## 1.3.0

* Windows: real speech recognition using the Windows speech recognizer
  (`Windows.Media.SpeechRecognition`), replacing the stub.
* Linux: speech recognition using Vosk and PulseAudio, loaded at runtime. It needs
  `libvosk`, `libpulse-simple` and a `VOSK_MODEL_PATH` folder of models; without them
  `isAvailable()` is `false` and `listen()` throws `NO_RECOGNIZER`.
* The Windows and Linux implementations have not been tested on those platforms yet.

## 1.2.0

* Added iOS and macOS support using Apple's Speech framework (`SFSpeechRecognizer`),
  shared between both platforms. The previous iOS stub is replaced.
* Apple platforms do not support Bengali; `listen()` throws `LANGUAGE_NOT_SUPPORTED`
  for unsupported languages.
* Added `FlutterNativeSpeechToText.isLanguageSupported(language)` on every platform.
* Added Windows and Linux native stubs: they never crash, `isAvailable()` and
  `isLanguageSupported()` return `false`, and `listen()`/`stop()`/`cancel()` fail with
  `PLATFORM_NOT_SUPPORTED`.
* Includes the web fixes from 1.1.1 below.

## 1.1.1

* Web: `listen()` now completes as soon as the browser reports an error, instead
  of waiting for an end event that Safari may never send.
* Web: `stop()` falls back to the latest result if the browser does not end the
  session within three seconds.
* Web: Brave is detected, since it blocks the speech service. `isAvailable()`
  returns `false` and `listen()` throws `NO_RECOGNIZER` with an explanatory message.
* Web: new `SERVICE_NOT_ALLOWED` error code, separate from `PERMISSION_DENIED`,
  for browsers that block their speech service (for example Safari when the
  requested language is not installed for Dictation).

## 1.1.0

* Added web support using the browser's Web Speech API (`SpeechRecognition` /
  `webkitSpeechRecognition`), exposed through the existing `listen`, `stop`,
  `cancel` and `isAvailable` API.
* New error codes on web: `PERMISSION_DENIED`, `NO_MICROPHONE`, `NETWORK`,
  `LANGUAGE_NOT_SUPPORTED`.
* Minimum SDK raised to Dart 3.4 / Flutter 3.22.

## 1.0.0

* Initial release of the plugin.
* Speech recognition using Android's native `ACTION_RECOGNIZE_SPEECH` Intent.
* Configurable parameters for `RecognizerIntent` extras:
  * `languageModel` (free form or web search)
  * `language` and `languagePreference`
  * `prompt`, `maxResults`
  * `partialResults`, `confidenceScores`
  * Speech silence length limits (`speechInputMinimumLengthMillis`, `speechInputCompleteSilenceLengthMillis`, `speechInputPossiblyCompleteSilenceLengthMillis`)
* Availability check with `FlutterNativeSpeechToText.isAvailable()`.
* Stop and cancel session controls.
* iOS support scaffold: the iOS implementation is a safe stub that never crashes.
  * `isAvailable()` returns `false`.
  * `listen()` / `stop()` / `cancel()` throw `SpeechToTextException` with code
    `PLATFORM_NOT_SUPPORTED`, so host apps can display and log the error.
* Complete example application demonstrating integration.
