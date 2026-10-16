#include "include/flutter_native_speech_to_text/flutter_native_speech_to_text_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_native_speech_to_text_plugin.h"

void FlutterNativeSpeechToTextPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter_native_speech_to_text::FlutterNativeSpeechToTextPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
