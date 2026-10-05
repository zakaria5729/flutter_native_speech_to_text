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
