#ifndef FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace flutter_native_speech_to_text {

class MainThreadQueue;
struct ListenState;

class FlutterNativeSpeechToTextPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  explicit FlutterNativeSpeechToTextPlugin(flutter::PluginRegistrarWindows *registrar);

  virtual ~FlutterNativeSpeechToTextPlugin();

  FlutterNativeSpeechToTextPlugin(const FlutterNativeSpeechToTextPlugin &) = delete;
  FlutterNativeSpeechToTextPlugin &operator=(const FlutterNativeSpeechToTextPlugin &) = delete;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

 private:
  void Listen(const flutter::EncodableMap *args,
              std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  flutter::PluginRegistrarWindows *registrar_;
  int window_proc_id_ = -1;
  std::shared_ptr<MainThreadQueue> queue_;
  std::shared_ptr<ListenState> active_;
};

}  // namespace flutter_native_speech_to_text

#endif  // FLUTTER_PLUGIN_FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN_H_
