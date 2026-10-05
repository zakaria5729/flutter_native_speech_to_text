import 'package:flutter/material.dart';
import 'package:flutter_native_speech_to_text/flutter_native_speech_to_text.dart';

void main() {
  runApp(const SpeechToTextExampleApp());
}

class SpeechToTextExampleApp extends StatelessWidget {
  const SpeechToTextExampleApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Speech to Text Example',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        useMaterial3: true,
      ),
      home: const SpeechToTextHomePage(),
    );
  }
}

class SpeechToTextHomePage extends StatefulWidget {
  const SpeechToTextHomePage({super.key});

  @override
  State<SpeechToTextHomePage> createState() => _SpeechToTextHomePageState();
}

class _SpeechToTextHomePageState extends State<SpeechToTextHomePage> {
  final String _languageModel = 'free_form';
  final String _language = 'bn-BD';
  final int _minimumSpeechLengthMillis = 10000;

  String _recognizedText = 'Press the button and speak...';
  bool _isListening = false;
  bool _isAvailable = false;
  List<String> _alternatives = [];
  List<double> _confidenceScores = [];

  @override
  void initState() {
    super.initState();
    _checkAvailability();
  }

  Future<void> _checkAvailability() async {
    final available = await FlutterSpeechToText.isAvailable();
    setState(() {
      _isAvailable = available;
    });
  }

  Future<void> _startListening() async {
    if (_isListening) return;

    setState(() {
      _isListening = true;
      _recognizedText = 'Listening...';
      _alternatives = [];
      _confidenceScores = [];
    });

    try {
      final result = await FlutterSpeechToText.listen(
        language: _language,
        languageModel: _languageModel,
        prompt: 'Speak now',
        maxResults: 5,
        partialResults: true,
        confidenceScores: true,
        speechInputMinimumLengthMillis: _minimumSpeechLengthMillis,
        isEnable: true,
      );

      setState(() {
        _recognizedText = result.recognizedText ?? 'No speech recognized';
        _alternatives = result.alternativeResults ?? [];
        _confidenceScores = result.confidenceScores ?? [];
        _isListening = false;
      });
    } on SpeechToTextException catch (e) {
      setState(() {
        _recognizedText = 'Error: ${e.message}';
        _isListening = false;
      });
    }
  }

  Future<void> _stopListening() async {
    await FlutterSpeechToText.stop();
    setState(() {
      _isListening = false;
    });
  }

  Future<void> _cancelListening() async {
    await FlutterSpeechToText.cancel();
    setState(() {
      _isListening = false;
      _recognizedText = 'Cancelled';
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Flutter Speech to Text'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Status: ${_isAvailable ? "Available" : "Unavailable"}',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _isListening ? 'Listening...' : 'Idle',
                      style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                            color: _isListening ? Colors.red : Colors.green,
                            fontWeight: FontWeight.bold,
                          ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Intent Extras:',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'EXTRA_LANGUAGE_MODEL: $_languageModel '
                      '(${_languageModel == 'web_search' ? "RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH" : "RecognizerIntent.LANGUAGE_MODEL_FREE_FORM"})',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                    Text(
                      'EXTRA_LANGUAGE: $_language',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                    Text(
                      'EXTRA_SPEECH_INPUT_MINIMUM_LENGTH_MILLIS: $_minimumSpeechLengthMillis',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Recognized Text:',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    SelectableText(
                      _recognizedText,
                      style: Theme.of(context).textTheme.bodyLarge,
                    ),
                  ],
                ),
              ),
            ),
            if (_alternatives.isNotEmpty) ...[
              const SizedBox(height: 16),
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Alternative Results:',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      ..._alternatives.asMap().entries.map((entry) {
                        final index = entry.key;
                        final alt = entry.value;
                        final confidence = index < _confidenceScores.length
                            ? _confidenceScores[index]
                            : 0.0;
                        return Padding(
                          padding: const EdgeInsets.symmetric(vertical: 4.0),
                          child: Text(
                            '$alt (Confidence: ${(confidence * 100).toStringAsFixed(1)}%)',
                            style: Theme.of(context).textTheme.bodyMedium,
                          ),
                        );
                      }),
                    ],
                  ),
                ),
              ),
            ],
            const SizedBox(height: 24),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed:
                        _isAvailable && !_isListening ? _startListening : null,
                    icon: const Icon(Icons.mic),
                    label: const Text('Start Listening'),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _isListening ? _stopListening : null,
                    icon: const Icon(Icons.stop),
                    label: const Text('Stop'),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.symmetric(vertical: 16),
                      backgroundColor: Colors.orange,
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            ElevatedButton.icon(
              onPressed: _isListening ? _cancelListening : null,
              icon: const Icon(Icons.cancel),
              label: const Text('Cancel'),
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.symmetric(vertical: 16),
                backgroundColor: Colors.red,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
