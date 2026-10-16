#include "flutter_native_speech_to_text_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>

namespace flutter_native_speech_to_text {

void FlutterNativeSpeechToTextPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "com.zakaria5729/flutter_native_speech_to_text",
      &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FlutterNativeSpeechToTextPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FlutterNativeSpeechToTextPlugin::FlutterNativeSpeechToTextPlugin() {}

FlutterNativeSpeechToTextPlugin::~FlutterNativeSpeechToTextPlugin() {}

void FlutterNativeSpeechToTextPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const std::string &method = method_call.method_name();

  if (method == "isAvailable" || method == "isLanguageSupported") {
    result->Success(flutter::EncodableValue(false));
  } else if (method == "listen" || method == "stop" || method == "cancel") {
    result->Error("PLATFORM_NOT_SUPPORTED",
                  "Speech recognition is not supported on Windows yet.");
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter_native_speech_to_text
