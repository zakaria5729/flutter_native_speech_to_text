#ifndef FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace flutter_native_speech_to_text {

class FlutterNativeSpeechToTextPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FlutterNativeSpeechToTextPlugin();

  virtual ~FlutterNativeSpeechToTextPlugin();

  FlutterNativeSpeechToTextPlugin(const FlutterNativeSpeechToTextPlugin &) = delete;
  FlutterNativeSpeechToTextPlugin &operator=(const FlutterNativeSpeechToTextPlugin &) = delete;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace flutter_native_speech_to_text

#endif  // FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_
