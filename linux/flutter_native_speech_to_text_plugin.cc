#include "include/flutter_native_speech_to_text/flutter_native_speech_to_text_plugin.h"

#include <dlfcn.h>
#include <flutter_linux/flutter_linux.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), flutter_native_speech_to_text_plugin_get_type(), \
                              FlutterNativeSpeechToTextPlugin))

namespace {

const char kChannelName[] = "com.zakaria5729/flutter_native_speech_to_text";
const char kDefaultLanguage[] = "bn-BD";
const char kModelPathEnv[] = "VOSK_MODEL_PATH";
const int kSampleRate = 16000;
const int kReadBytes = 3200;
const double kNoSpeechSeconds = 8.0;
const double kMaxSeconds = 30.0;

struct VoskModel;
struct VoskRecognizer;

struct VoskApi {
  VoskModel* (*model_new)(const char*) = nullptr;
  VoskRecognizer* (*recognizer_new)(VoskModel*, float) = nullptr;
  void (*set_max_alternatives)(VoskRecognizer*, int) = nullptr;
  int (*accept_waveform)(VoskRecognizer*, const char*, int) = nullptr;
  const char* (*result)(VoskRecognizer*) = nullptr;
  const char* (*partial_result)(VoskRecognizer*) = nullptr;
  const char* (*final_result)(VoskRecognizer*) = nullptr;
  void (*recognizer_free)(VoskRecognizer*) = nullptr;
  void (*set_log_level)(int) = nullptr;
  bool loaded = false;
};

struct PulseSampleSpec {
  int format;
  unsigned int rate;
  unsigned char channels;
};

struct PulseApi {
  void* (*simple_new)(const char*, const char*, int, const char*, const char*,
                      const PulseSampleSpec*, const void*, const void*, int*) = nullptr;
  int (*simple_read)(void*, void*, size_t, int*) = nullptr;
  void (*simple_free)(void*) = nullptr;
  bool loaded = false;
};

const int kPulseStreamRecord = 2;
const int kPulseSampleS16LE = 3;

VoskApi& GetVosk() {
  static VoskApi api;
  static std::once_flag once;
  std::call_once(once, []() {
    void* handle = dlopen("libvosk.so", RTLD_LAZY);
    if (handle == nullptr) return;
    api.model_new = reinterpret_cast<decltype(api.model_new)>(dlsym(handle, "vosk_model_new"));
    api.recognizer_new =
        reinterpret_cast<decltype(api.recognizer_new)>(dlsym(handle, "vosk_recognizer_new"));
    api.set_max_alternatives = reinterpret_cast<decltype(api.set_max_alternatives)>(
        dlsym(handle, "vosk_recognizer_set_max_alternatives"));
    api.accept_waveform = reinterpret_cast<decltype(api.accept_waveform)>(
        dlsym(handle, "vosk_recognizer_accept_waveform"));
    api.result =
        reinterpret_cast<decltype(api.result)>(dlsym(handle, "vosk_recognizer_result"));
    api.partial_result = reinterpret_cast<decltype(api.partial_result)>(
        dlsym(handle, "vosk_recognizer_partial_result"));
    api.final_result = reinterpret_cast<decltype(api.final_result)>(
        dlsym(handle, "vosk_recognizer_final_result"));
    api.recognizer_free =
        reinterpret_cast<decltype(api.recognizer_free)>(dlsym(handle, "vosk_recognizer_free"));
    api.set_log_level =
        reinterpret_cast<decltype(api.set_log_level)>(dlsym(handle, "vosk_set_log_level"));
    api.loaded = api.model_new && api.recognizer_new && api.accept_waveform && api.result &&
                 api.partial_result && api.final_result && api.recognizer_free;
    if (api.loaded && api.set_log_level) api.set_log_level(-1);
  });
  return api;
}

PulseApi& GetPulse() {
  static PulseApi api;
  static std::once_flag once;
  std::call_once(once, []() {
    void* handle = dlopen("libpulse-simple.so.0", RTLD_LAZY);
    if (handle == nullptr) return;
    api.simple_new =
        reinterpret_cast<decltype(api.simple_new)>(dlsym(handle, "pa_simple_new"));
    api.simple_read =
        reinterpret_cast<decltype(api.simple_read)>(dlsym(handle, "pa_simple_read"));
    api.simple_free =
        reinterpret_cast<decltype(api.simple_free)>(dlsym(handle, "pa_simple_free"));
    api.loaded = api.simple_new && api.simple_read && api.simple_free;
  });
  return api;
}

std::string Lowercase(std::string value) {
  for (char& c : value) {
    if (c == '_') c = '-';
    c = static_cast<char>(g_ascii_tolower(c));
  }
  return value;
}

std::string ModelRoot() {
  const char* root = std::getenv(kModelPathEnv);
  return root == nullptr ? std::string() : std::string(root);
}

std::string FindModelPath(const std::string& language) {
  std::string root = ModelRoot();
  if (root.empty() || language.empty()) return std::string();
  std::string wanted = Lowercase(language);
  bool bare = wanted.find('-') == std::string::npos;

  GDir* dir = g_dir_open(root.c_str(), 0, nullptr);
  if (dir == nullptr) return std::string();
  std::string found;
  std::string prefix_match;
  const gchar* name;
  while ((name = g_dir_read_name(dir)) != nullptr) {
    std::string entry = Lowercase(name);
    gchar* full = g_build_filename(root.c_str(), name, nullptr);
    bool is_dir = g_file_test(full, G_FILE_TEST_IS_DIR);
    std::string full_path = full;
    g_free(full);
    if (!is_dir) continue;
    if (entry == wanted) {
      found = full_path;
      break;
    }
    if (bare && entry.compare(0, wanted.size() + 1, wanted + "-") == 0 && prefix_match.empty()) {
      prefix_match = full_path;
    }
  }
  g_dir_close(dir);
  return found.empty() ? prefix_match : found;
}

bool HasAnyModel() {
  std::string root = ModelRoot();
  if (root.empty()) return false;
  GDir* dir = g_dir_open(root.c_str(), 0, nullptr);
  if (dir == nullptr) return false;
  bool any = false;
  const gchar* name;
  while ((name = g_dir_read_name(dir)) != nullptr) {
    gchar* full = g_build_filename(root.c_str(), name, nullptr);
    if (g_file_test(full, G_FILE_TEST_IS_DIR)) any = true;
    g_free(full);
    if (any) break;
  }
  g_dir_close(dir);
  return any;
}

bool EngineAvailable() { return GetVosk().loaded && GetPulse().loaded && HasAnyModel(); }

VoskModel* LoadModel(const std::string& path) {
  static std::mutex mutex;
  static std::map<std::string, VoskModel*> cache;
  std::lock_guard<std::mutex> lock(mutex);
  auto it = cache.find(path);
  if (it != cache.end()) return it->second;
  VoskModel* model = GetVosk().model_new(path.c_str());
  if (model != nullptr) cache[path] = model;
  return model;
}

std::string DecodeJsonString(const std::string& json, size_t& pos) {
  std::string out;
  while (pos < json.size() && json[pos] != '"') {
    char c = json[pos++];
    if (c != '\\' || pos >= json.size()) {
      out += c;
      continue;
    }
    char e = json[pos++];
    switch (e) {
      case 'n': out += '\n'; break;
      case 't': out += '\t'; break;
      case 'r': out += '\r'; break;
      case 'b': out += '\b'; break;
      case 'f': out += '\f'; break;
      case 'u': {
        if (pos + 4 > json.size()) break;
        unsigned int code = static_cast<unsigned int>(
            std::strtoul(json.substr(pos, 4).c_str(), nullptr, 16));
        pos += 4;
        gchar utf8[8];
        gint length = g_unichar_to_utf8(code, utf8);
        out.append(utf8, static_cast<size_t>(length));
        break;
      }
      default: out += e; break;
    }
  }
  if (pos < json.size()) pos++;
  return out;
}

std::vector<std::string> ExtractTexts(const std::string& json, const char* key) {
  std::vector<std::string> texts;
  std::string needle = std::string("\"") + key + "\"";
  size_t pos = 0;
  while ((pos = json.find(needle, pos)) != std::string::npos) {
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;
    if (pos < json.size() && json[pos] == '"') {
      pos++;
      std::string value = DecodeJsonString(json, pos);
      if (!value.empty()) texts.push_back(value);
    }
  }
  return texts;
}

struct Outcome {
  std::vector<std::string> texts;
  std::string error_code;
  std::string error_message;
};

struct Session {
  FlutterNativeSpeechToTextPlugin* plugin = nullptr;
  FlMethodCall* call = nullptr;
  std::string model_path;
  int max_results = 1;
  std::atomic<bool> stop{false};
  std::atomic<bool> cancel{false};
  Outcome outcome;
};

}  // namespace

struct _FlutterNativeSpeechToTextPlugin {
  GObject parent_instance;
  Session* active;
};

G_DEFINE_TYPE(FlutterNativeSpeechToTextPlugin, flutter_native_speech_to_text_plugin,
              g_object_get_type())

namespace {

FlMethodResponse* BuildResponse(const Outcome& outcome, bool cancelled) {
  if (!outcome.error_code.empty()) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        outcome.error_code.c_str(), outcome.error_message.c_str(), nullptr));
  }
  if (cancelled || outcome.texts.empty()) {
    g_autoptr(FlValue) empty = fl_value_new_null();
    return FL_METHOD_RESPONSE(fl_method_success_response_new(empty));
  }
  g_autoptr(FlValue) map = fl_value_new_map();
  fl_value_set_string_take(map, "recognizedText",
                           fl_value_new_string(outcome.texts.front().c_str()));
  FlValue* alternatives = fl_value_new_list();
  for (const std::string& text : outcome.texts) {
    fl_value_append_take(alternatives, fl_value_new_string(text.c_str()));
  }
  fl_value_set_string_take(map, "alternativeResults", alternatives);
  fl_value_set_string_take(map, "isFinal", fl_value_new_bool(TRUE));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(map));
}

gboolean FinishOnMainThread(gpointer data) {
  Session* session = static_cast<Session*>(data);
  g_autoptr(FlMethodResponse) response = BuildResponse(session->outcome, session->cancel.load());
  fl_method_call_respond(session->call, response, nullptr);
  if (session->plugin->active == session) session->plugin->active = nullptr;
  g_object_unref(session->call);
  delete session;
  return G_SOURCE_REMOVE;
}

void RunSession(Session* session) {
  VoskApi& vosk = GetVosk();
  PulseApi& pulse = GetPulse();
  Outcome& outcome = session->outcome;

  VoskModel* model = LoadModel(session->model_path);
  if (model == nullptr) {
    outcome.error_code = "ERROR";
    outcome.error_message = "Failed to load the speech model at " + session->model_path;
    g_main_context_invoke(nullptr, FinishOnMainThread, session);
    return;
  }

  VoskRecognizer* recognizer = vosk.recognizer_new(model, static_cast<float>(kSampleRate));
  if (recognizer != nullptr && session->max_results > 1 && vosk.set_max_alternatives) {
    vosk.set_max_alternatives(recognizer, session->max_results);
  }

  PulseSampleSpec spec = {kPulseSampleS16LE, kSampleRate, 1};
  int error = 0;
  void* stream = pulse.simple_new(nullptr, "flutter_native_speech_to_text", kPulseStreamRecord,
                                  nullptr, "listen", &spec, nullptr, nullptr, &error);
  if (recognizer == nullptr || stream == nullptr) {
    outcome.error_code = stream == nullptr ? "NO_MICROPHONE" : "ERROR";
    outcome.error_message = stream == nullptr ? "No microphone was found or it could not be used"
                                              : "Failed to create the speech recognizer";
    if (stream != nullptr) pulse.simple_free(stream);
    if (recognizer != nullptr) vosk.recognizer_free(recognizer);
    g_main_context_invoke(nullptr, FinishOnMainThread, session);
    return;
  }

  std::vector<char> buffer(kReadBytes);
  auto started = std::chrono::steady_clock::now();
  bool heard = false;
  const char* json = nullptr;

  while (!session->cancel.load()) {
    if (pulse.simple_read(stream, buffer.data(), buffer.size(), &error) < 0) {
      outcome.error_code = "NO_MICROPHONE";
      outcome.error_message = "Reading from the microphone failed";
      break;
    }
    int ended = vosk.accept_waveform(recognizer, buffer.data(), static_cast<int>(buffer.size()));
    if (ended == 1) {
      json = vosk.result(recognizer);
      break;
    }
    if (!heard) {
      const char* partial = vosk.partial_result(recognizer);
      if (partial != nullptr && !ExtractTexts(partial, "partial").empty()) heard = true;
    }
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    if (session->stop.load() || elapsed > kMaxSeconds) {
      json = vosk.final_result(recognizer);
      break;
    }
    if (!heard && elapsed > kNoSpeechSeconds) break;
  }

  if (json != nullptr && outcome.error_code.empty()) {
    std::string text = json;
    outcome.texts = ExtractTexts(text, "text");
  }

  pulse.simple_free(stream);
  vosk.recognizer_free(recognizer);
  g_main_context_invoke(nullptr, FinishOnMainThread, session);
}

FlMethodResponse* BoolResponse(bool value) {
  g_autoptr(FlValue) result = fl_value_new_bool(value ? TRUE : FALSE);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

FlMethodResponse* ErrorResponse(const char* code, const char* message) {
  return FL_METHOD_RESPONSE(fl_method_error_response_new(code, message, nullptr));
}

FlMethodResponse* NullResponse() {
  g_autoptr(FlValue) result = fl_value_new_null();
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

const char* StringArg(FlValue* args, const char* key) {
  if (args == nullptr || fl_value_get_type(args) != FL_VALUE_TYPE_MAP) return nullptr;
  FlValue* value = fl_value_lookup_string(args, key);
  if (value == nullptr || fl_value_get_type(value) != FL_VALUE_TYPE_STRING) return nullptr;
  return fl_value_get_string(value);
}

FlMethodResponse* StartListening(FlutterNativeSpeechToTextPlugin* self, FlMethodCall* method_call) {
  FlValue* args = fl_method_call_get_args(method_call);

  if (args != nullptr && fl_value_get_type(args) == FL_VALUE_TYPE_MAP) {
    FlValue* enabled = fl_value_lookup_string(args, "isEnable");
    if (enabled != nullptr && fl_value_get_type(enabled) == FL_VALUE_TYPE_BOOL &&
        !fl_value_get_bool(enabled)) {
      return ErrorResponse("DISABLED", "Speech to text recognition unavailable");
    }
  }
  if (self->active != nullptr) {
    return ErrorResponse("ALREADY_LISTENING", "A recognition session is already in progress");
  }
  if (!GetVosk().loaded || !GetPulse().loaded) {
    return ErrorResponse("NO_RECOGNIZER",
                         "Speech recognition on Linux needs libvosk and libpulse-simple "
                         "installed, and VOSK_MODEL_PATH set to a folder of Vosk models");
  }

  const char* language = StringArg(args, "language");
  if (language == nullptr) language = StringArg(args, "languagePreference");
  if (language == nullptr) language = kDefaultLanguage;

  std::string model_path = FindModelPath(language);
  if (model_path.empty()) {
    return ErrorResponse("LANGUAGE_NOT_SUPPORTED",
                         "No speech model for the requested language was found in "
                         "VOSK_MODEL_PATH");
  }

  Session* session = new Session();
  session->plugin = self;
  session->call = FL_METHOD_CALL(g_object_ref(method_call));
  session->model_path = model_path;
  if (args != nullptr && fl_value_get_type(args) == FL_VALUE_TYPE_MAP) {
    FlValue* max_results = fl_value_lookup_string(args, "maxResults");
    if (max_results != nullptr && fl_value_get_type(max_results) == FL_VALUE_TYPE_INT) {
      int64_t value = fl_value_get_int(max_results);
      session->max_results = value < 1 ? 1 : static_cast<int>(value);
    }
  }
  self->active = session;
  std::thread(RunSession, session).detach();
  return nullptr;
}

void HandleMethodCall(FlutterNativeSpeechToTextPlugin* self, FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "isAvailable") == 0) {
    response = BoolResponse(EngineAvailable());
  } else if (strcmp(method, "isLanguageSupported") == 0) {
    const char* language = StringArg(fl_method_call_get_args(method_call), "language");
    bool supported = language != nullptr && GetVosk().loaded && GetPulse().loaded &&
                     !FindModelPath(language).empty();
    response = BoolResponse(supported);
  } else if (strcmp(method, "listen") == 0) {
    response = StartListening(self, method_call);
    if (response == nullptr) return;
  } else if (strcmp(method, "stop") == 0) {
    if (self->active != nullptr) self->active->stop = true;
    response = NullResponse();
  } else if (strcmp(method, "cancel") == 0) {
    if (self->active != nullptr) self->active->cancel = true;
    response = NullResponse();
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

}  // namespace

static void flutter_native_speech_to_text_plugin_dispose(GObject* object) {
  FlutterNativeSpeechToTextPlugin* self = FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(object);
  if (self->active != nullptr) self->active->cancel = true;
  G_OBJECT_CLASS(flutter_native_speech_to_text_plugin_parent_class)->dispose(object);
}

static void flutter_native_speech_to_text_plugin_class_init(
    FlutterNativeSpeechToTextPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = flutter_native_speech_to_text_plugin_dispose;
}

static void flutter_native_speech_to_text_plugin_init(FlutterNativeSpeechToTextPlugin* self) {
  self->active = nullptr;
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  FlutterNativeSpeechToTextPlugin* plugin = FLUTTER_NATIVE_SPEECH_TO_TEXT_PLUGIN(user_data);
  HandleMethodCall(plugin, method_call);
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
