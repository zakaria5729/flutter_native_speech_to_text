#include "include/flutter_native_speech_to_text/flutter_native_speech_to_text_plugin.h"

#include <flutter_linux/flutter_linux.h>

#include <cstring>

#define FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), flutter_native_speech_to_text_plugin_get_type(), \
                              FlutterNativeSpeechToTextPlugin))

struct _FlutterNativeSpeechToTextPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(FlutterNativeSpeechToTextPlugin, flutter_native_speech_to_text_plugin,
              g_object_get_type())

static const char kChannelName[] = "com.zakaria5729/flutter_native_speech_to_text";
static const char kNotSupportedMessage[] =
    "Speech recognition is not supported on Linux yet.";

static void handle_method_call(FlutterNativeSpeechToTextPlugin* self,
                               FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "isAvailable") == 0 || strcmp(method, "isLanguageSupported") == 0) {
    g_autoptr(FlValue) result = fl_value_new_bool(FALSE);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  } else if (strcmp(method, "listen") == 0 || strcmp(method, "stop") == 0 ||
             strcmp(method, "cancel") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_error_response_new(
        "PLATFORM_NOT_SUPPORTED", kNotSupportedMessage, nullptr));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void flutter_native_speech_to_text_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(flutter_native_speech_to_text_plugin_parent_class)->dispose(object);
}

static void flutter_native_speech_to_text_plugin_class_init(
    FlutterNativeSpeechToTextPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = flutter_native_speech_to_text_plugin_dispose;
}

static void flutter_native_speech_to_text_plugin_init(FlutterNativeSpeechToTextPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  FlutterNativeSpeechToTextPlugin* plugin = FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(user_data);
  handle_method_call(plugin, method_call);
}

void flutter_native_speech_to_text_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  FlutterNativeSpeechToTextPlugin* plugin = FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(
      g_object_new(flutter_native_speech_to_text_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel = fl_method_channel_new(
      fl_plugin_registrar_get_messenger(registrar), kChannelName, FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb, g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
