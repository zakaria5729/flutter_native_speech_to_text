import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_native_speech_to_text_example/main.dart';

void main() {
  testWidgets('Verify SpeechToTextExampleApp renders',
      (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const SpeechToTextExampleApp());

    // Verify that the title is displayed in the AppBar.
    expect(find.text('Flutter Speech to Text'), findsOneWidget);
  });
}
