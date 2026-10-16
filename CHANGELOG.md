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
