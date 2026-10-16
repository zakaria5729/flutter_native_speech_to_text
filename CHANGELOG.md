## 1.1.1

* Web: `listen()` now completes as soon as the browser reports an error, instead
  of waiting for an end event that Safari may never send.
* Web: `stop()` falls back to the latest result if the browser does not end the
  session within three seconds.
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
