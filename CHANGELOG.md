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
* Complete example application demonstrating integration.
