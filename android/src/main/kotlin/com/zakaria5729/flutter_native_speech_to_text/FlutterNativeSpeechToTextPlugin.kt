package com.zakaria5729.flutter_native_speech_to_text

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.speech.RecognizerIntent
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.embedding.engine.plugins.activity.ActivityAware
import io.flutter.embedding.engine.plugins.activity.ActivityPluginBinding
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result
import io.flutter.plugin.common.PluginRegistry.ActivityResultListener

class FlutterNativeSpeechToTextPlugin : FlutterPlugin, MethodCallHandler, ActivityAware, ActivityResultListener {
    private var methodChannel: MethodChannel? = null
    private var pendingResult: Result? = null
    private var activityBinding: ActivityPluginBinding? = null
    private var hostActivity: Activity? = null

    // ---- FlutterPlugin ----
    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        methodChannel = MethodChannel(binding.binaryMessenger, CHANNEL_NAME).also {
            it.setMethodCallHandler(this)
        }
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        methodChannel?.setMethodCallHandler(null)
        methodChannel = null
    }

    // ---- ActivityAware ----
    override fun onAttachedToActivity(binding: ActivityPluginBinding) {
        activityBinding = binding
        hostActivity = binding.activity
        binding.addActivityResultListener(this)
    }

    override fun onDetachedFromActivityForConfigChanges() = detachActivity()

    override fun onReattachedToActivityForConfigChanges(binding: ActivityPluginBinding) =
        onAttachedToActivity(binding)

    override fun onDetachedFromActivity() = detachActivity()

    private fun detachActivity() {
        activityBinding?.removeActivityResultListener(this)
        activityBinding = null
        hostActivity = null
    }

    // ---- MethodCallHandler ----
    override fun onMethodCall(call: MethodCall, result: Result) {
        when (call.method) {
            "isAvailable" -> result.success(isRecognizerAvailable())
            "listen" -> {
                @Suppress("UNCHECKED_CAST")
                val arguments = call.arguments as? Map<String, Any?> ?: emptyMap()
                startRecognition(arguments, result)
            }
            "stop" -> {
                stopRecognition()
                result.success(null)
            }
            "cancel" -> {
                cancelRecognition()
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }

    private fun isRecognizerAvailable(): Boolean {
        val activity = hostActivity ?: return false
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
        @Suppress("DEPRECATION")
        val activities = activity.packageManager.queryIntentActivities(
            intent,
            PackageManager.MATCH_DEFAULT_ONLY
        )
        return activities.isNotEmpty()
    }

    private fun startRecognition(arguments: Map<String, Any?>, result: Result) {
        val activity = hostActivity
        if (activity == null) {
            result.error("NO_CONTEXT", "Activity context not available", null)
            return
        }

        if (pendingResult != null) {
            result.error("ALREADY_LISTENING", "A recognition session is already in progress", null)
            return
        }

        if (!isRecognizerAvailable()) {
            result.error("NO_RECOGNIZER", "No speech recognizer found", null)
            return
        }

        val isEnabled = arguments["isEnable"] as? Boolean ?: true
        if (!isEnabled) {
            result.error("DISABLED", "Speech to text recognition unavailable", null)
            return
        }

        val recognitionIntent = buildRecognitionIntent(arguments)

        try {
            pendingResult = result
            activity.startActivityForResult(recognitionIntent, SPEECH_REQUEST_CODE)
        } catch (e: Exception) {
            pendingResult = null
            result.error("ERROR", "Failed to start speech recognition: ${e.message}", null)
        }
    }

    private fun buildRecognitionIntent(arguments: Map<String, Any?>): Intent {
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)

        // Language model is required by the recognizer; default to free form.
        val languageModel = when (arguments["languageModel"] as? String) {
            "web_search" -> RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH
            else -> RecognizerIntent.LANGUAGE_MODEL_FREE_FORM
        }
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, languageModel)

        (arguments["languagePreference"] as? String)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_PREFERENCE, it)
        }
        (arguments["partialResults"] as? Boolean)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, it)
        }
        (arguments["confidenceScores"] as? Boolean)?.let {
            // EXTRA_CONFIDENCE_SCORES is a result extra; keep the request flag harmless.
            intent.putExtra("android.speech.extra.CONFIDENCE_SCORES", it)
        }
        (arguments["originatorPackage"] as? String)?.let {
            intent.putExtra("calling_package", it)
        }
        (arguments["speechInputMinimumLengthMillis"] as? Number)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_MINIMUM_LENGTH_MILLIS, it.toLong())
        }
        (arguments["speechInputCompleteSilenceLengthMillis"] as? Number)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_COMPLETE_SILENCE_LENGTH_MILLIS, it.toLong())
        }
        (arguments["speechInputPossiblyCompleteSilenceLengthMillis"] as? Number)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_POSSIBLY_COMPLETE_SILENCE_LENGTH_MILLIS, it.toLong())
        }
        (arguments["getAudioFormat"] as? String)?.let {
            intent.putExtra(EXTRA_GET_AUDIO_FORMAT_KEY, it)
        }
        (arguments["getAudio"] as? Boolean)?.let {
            intent.putExtra(EXTRA_GET_AUDIO_KEY, it)
        }
        // Falls back to Bengali (Bangladesh) when no language is provided.
        val language = arguments["language"] as? String ?: DEFAULT_LANGUAGE
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, language)
        (arguments["prompt"] as? String)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_PROMPT, it)
        }
        (arguments["maxResults"] as? Number)?.let {
            intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, it.toInt())
        }

        return intent
    }

    private fun stopRecognition() {
        // The system recognizer UI is modal; there is nothing to stop programmatically.
    }

    private fun cancelRecognition() {
        pendingResult?.success(null)
        pendingResult = null
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?): Boolean {
        if (requestCode != SPEECH_REQUEST_CODE) return false

        val result = pendingResult
        pendingResult = null
        if (result == null) return true

        var recognizedText: String? = null
        var alternativeResults: List<String>? = null
        var confidenceScores: List<Double>? = null

        if (resultCode == Activity.RESULT_OK && data != null) {
            val results = data.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS)
            recognizedText = results?.firstOrNull()
            alternativeResults = results?.toList()

            val scores = data.getFloatArrayExtra(RecognizerIntent.EXTRA_CONFIDENCE_SCORES)
            confidenceScores = scores?.map { it.toDouble() }
        }

        result.success(
            mapOf(
                "recognizedText" to recognizedText,
                "alternativeResults" to alternativeResults,
                "confidenceScores" to confidenceScores,
                "isFinal" to true
            )
        )
        return true
    }

    companion object {
        private const val CHANNEL_NAME = "com.zakaria5729/flutter_native_speech_to_text"
        private const val SPEECH_REQUEST_CODE = 100

        // Default value for RecognizerIntent.EXTRA_LANGUAGE.
        private const val DEFAULT_LANGUAGE = "bn-BD"

        // Not exposed as public constants in the SDK, so use the raw intent keys.
        private const val EXTRA_GET_AUDIO_FORMAT_KEY = "android.speech.extra.GET_AUDIO_FORMAT"
        private const val EXTRA_GET_AUDIO_KEY = "android.speech.extra.GET_AUDIO"
    }
}
