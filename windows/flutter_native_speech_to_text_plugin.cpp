#include "flutter_native_speech_to_text_plugin.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <unknwn.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Media.SpeechRecognition.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wf = winrt::Windows::Foundation;
namespace wg = winrt::Windows::Globalization;
namespace sr = winrt::Windows::Media::SpeechRecognition;

namespace flutter_native_speech_to_text {

namespace {

constexpr UINT kRunTasksMessage = WM_APP + 0x5A;
constexpr char kDefaultLanguage[] = "bn-BD";
constexpr int kDefaultSilenceMillis = 5000;

using Result = flutter::MethodResult<flutter::EncodableValue>;

std::wstring Utf8ToWide(const std::string &value) {
  if (value.empty()) return std::wstring();
  int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                 nullptr, 0);
  std::wstring wide(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &wide[0], size);
  return wide;
}

std::string NormalizeTag(std::string tag) {
  for (char &c : tag) {
    if (c == '_') c = '-';
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return tag;
}

const flutter::EncodableValue *FindValue(const flutter::EncodableMap *map, const char *key) {
  if (map == nullptr) return nullptr;
  auto it = map->find(flutter::EncodableValue(key));
  return it == map->end() ? nullptr : &it->second;
}

std::optional<std::string> GetString(const flutter::EncodableMap *map, const char *key) {
  const flutter::EncodableValue *value = FindValue(map, key);
  if (value == nullptr) return std::nullopt;
  const std::string *text = std::get_if<std::string>(value);
  if (text == nullptr) return std::nullopt;
  return *text;
}

std::optional<int> GetInt(const flutter::EncodableMap *map, const char *key) {
  const flutter::EncodableValue *value = FindValue(map, key);
  if (value == nullptr) return std::nullopt;
  if (const int32_t *v32 = std::get_if<int32_t>(value)) return static_cast<int>(*v32);
  if (const int64_t *v64 = std::get_if<int64_t>(value)) return static_cast<int>(*v64);
  return std::nullopt;
}

bool IsLanguageSupported(const std::string &language) {
  try {
    std::string wanted = NormalizeTag(language);
    if (wanted.empty()) return false;
    bool bare = wanted.find('-') == std::string::npos;
    for (auto const &supported : sr::SpeechRecognizer::SupportedTopicLanguages()) {
      std::string tag = NormalizeTag(winrt::to_string(supported.LanguageTag()));
      if (tag == wanted) return true;
      if (bare && tag.substr(0, tag.find('-')) == wanted) return true;
    }
  } catch (winrt::hresult_error const &) {
  }
  return false;
}

}  // namespace

class MainThreadQueue {
 public:
  void SetWindow(HWND hwnd) {
    std::lock_guard<std::mutex> lock(mutex_);
    hwnd_ = hwnd;
  }

  void Post(std::function<void()> task) {
    HWND hwnd = nullptr;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (hwnd_ == nullptr) return;
      tasks_.push_back(std::move(task));
      hwnd = hwnd_;
    }
    PostMessage(hwnd, kRunTasksMessage, 0, 0);
  }

  void Run() {
    std::vector<std::function<void()>> pending;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      pending.swap(tasks_);
    }
    for (auto &task : pending) task();
  }

  void Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    hwnd_ = nullptr;
    tasks_.clear();
  }

 private:
  std::mutex mutex_;
  HWND hwnd_ = nullptr;
  std::vector<std::function<void()>> tasks_;
};

struct Hypothesis {
  std::string text;
  double confidence;
};

struct ListenState {
  std::shared_ptr<MainThreadQueue> queue;
  std::shared_ptr<Result> result;
  std::function<void()> on_finished;
  sr::SpeechRecognizer recognizer{nullptr};
  int max_results = 1;

  std::mutex mutex;
  bool finished = false;
  std::vector<std::vector<Hypothesis>> utterances;
};

namespace {

flutter::EncodableValue BuildResult(const ListenState &state) {
  std::string text;
  for (const auto &utterance : state.utterances) {
    if (utterance.empty()) continue;
    if (!text.empty()) text += ' ';
    text += utterance.front().text;
  }
  if (text.empty()) return flutter::EncodableValue();

  flutter::EncodableList alternatives;
  flutter::EncodableList confidences;
  if (state.utterances.size() == 1) {
    for (const auto &hypothesis : state.utterances.front()) {
      alternatives.emplace_back(hypothesis.text);
      confidences.emplace_back(hypothesis.confidence);
    }
  } else {
    double total = 0;
    for (const auto &utterance : state.utterances) total += utterance.front().confidence;
    alternatives.emplace_back(text);
    confidences.emplace_back(total / static_cast<double>(state.utterances.size()));
  }

  flutter::EncodableMap map;
  map[flutter::EncodableValue("recognizedText")] = flutter::EncodableValue(text);
  map[flutter::EncodableValue("alternativeResults")] = flutter::EncodableValue(alternatives);
  map[flutter::EncodableValue("confidenceScores")] = flutter::EncodableValue(confidences);
  map[flutter::EncodableValue("isFinal")] = flutter::EncodableValue(true);
  return flutter::EncodableValue(map);
}

void FinishWith(const std::shared_ptr<ListenState> &state,
                std::function<void(Result &)> responder) {
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->finished) return;
    state->finished = true;
  }
  state->queue->Post([state, responder]() {
    responder(*state->result);
    if (state->on_finished) state->on_finished();
    try {
      if (state->recognizer) state->recognizer.Close();
    } catch (winrt::hresult_error const &) {
    }
  });
}

void FinishSuccess(const std::shared_ptr<ListenState> &state) {
  flutter::EncodableValue value;
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    value = BuildResult(*state);
  }
  FinishWith(state, [value](Result &result) { result.Success(value); });
}

void FinishNull(const std::shared_ptr<ListenState> &state) {
  FinishWith(state, [](Result &result) { result.Success(flutter::EncodableValue()); });
}

void FinishError(const std::shared_ptr<ListenState> &state, std::string code,
                 std::string message) {
  FinishWith(state, [code, message](Result &result) { result.Error(code, message); });
}

void FinishFromHResult(const std::shared_ptr<ListenState> &state, int32_t code) {
  switch (static_cast<uint32_t>(code)) {
    case 0x80045509:
      FinishError(state, "PERMISSION_DENIED",
                  "Online speech recognition is turned off in Windows privacy settings");
      break;
    case 0x80070005:
      FinishError(state, "PERMISSION_DENIED", "Microphone access was denied");
      break;
    case 0x8004503A:
      FinishError(state, "LANGUAGE_NOT_SUPPORTED",
                  "The requested language is not supported by Windows speech recognition");
      break;
    default:
      FinishError(state, "ERROR", "Speech recognition failed");
      break;
  }
}

void OnResultGenerated(const std::shared_ptr<ListenState> &state,
                       sr::SpeechRecognitionResult const &result) {
  try {
    std::vector<Hypothesis> hypotheses;
    hypotheses.push_back({winrt::to_string(result.Text()), result.RawConfidence()});
    if (state->max_results > 1) {
      for (auto const &alternate :
           result.GetAlternates(static_cast<uint32_t>(state->max_results))) {
        std::string text = winrt::to_string(alternate.Text());
        if (text != hypotheses.front().text) {
          hypotheses.push_back({text, alternate.RawConfidence()});
        }
      }
    }
    if (hypotheses.front().text.empty()) return;
    std::lock_guard<std::mutex> lock(state->mutex);
    state->utterances.push_back(std::move(hypotheses));
  } catch (winrt::hresult_error const &) {
  }
}

void OnCompleted(const std::shared_ptr<ListenState> &state,
                 sr::SpeechRecognitionResultStatus status) {
  bool has_text;
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    has_text = !state->utterances.empty();
  }
  switch (status) {
    case sr::SpeechRecognitionResultStatus::Success:
    case sr::SpeechRecognitionResultStatus::PauseLimitExceeded:
    case sr::SpeechRecognitionResultStatus::TimeoutExceeded:
    case sr::SpeechRecognitionResultStatus::UserCanceled:
      FinishSuccess(state);
      break;
    case sr::SpeechRecognitionResultStatus::NetworkFailure:
      FinishError(state, "NETWORK", "The speech recognition service could not be reached");
      break;
    case sr::SpeechRecognitionResultStatus::MicrophoneUnavailable:
      FinishError(state, "NO_MICROPHONE", "No microphone was found or it could not be used");
      break;
    case sr::SpeechRecognitionResultStatus::TopicLanguageNotSupported:
      FinishError(state, "LANGUAGE_NOT_SUPPORTED",
                  "The requested language is not supported by Windows speech recognition");
      break;
    default:
      if (has_text) {
        FinishSuccess(state);
      } else {
        FinishError(state, "ERROR", "Speech recognition failed");
      }
      break;
  }
}

void StartRecognition(const std::shared_ptr<ListenState> &state, int silence_millis) {
  auto compile = state->recognizer.CompileConstraintsAsync();
  compile.Completed([state, silence_millis](
                        wf::IAsyncOperation<sr::SpeechRecognitionCompilationResult> const &operation,
                        wf::AsyncStatus status) {
    try {
      if (status != wf::AsyncStatus::Completed) {
        FinishError(state, "ERROR", "Failed to prepare speech recognition");
        return;
      }
      sr::SpeechRecognitionCompilationResult compiled = operation.GetResults();
      if (compiled.Status() != sr::SpeechRecognitionResultStatus::Success) {
        OnCompleted(state, compiled.Status());
        return;
      }

      sr::SpeechContinuousRecognitionSession session =
          state->recognizer.ContinuousRecognitionSession();
      session.AutoStopSilenceTimeout(std::chrono::milliseconds(silence_millis));
      session.ResultGenerated(
          [state](sr::SpeechContinuousRecognitionSession const &,
                  sr::SpeechContinuousRecognitionResultGeneratedEventArgs const &args) {
            OnResultGenerated(state, args.Result());
          });
      session.Completed([state](sr::SpeechContinuousRecognitionSession const &,
                                sr::SpeechContinuousRecognitionCompletedEventArgs const &args) {
        OnCompleted(state, args.Status());
      });
      session.StartAsync().Completed(
          [state](wf::IAsyncAction const &action, wf::AsyncStatus start_status) {
            if (start_status == wf::AsyncStatus::Completed) return;
            try {
              action.GetResults();
              FinishError(state, "ERROR", "Failed to start speech recognition");
            } catch (winrt::hresult_error const &error) {
              FinishFromHResult(state, error.code());
            }
          });
    } catch (winrt::hresult_error const &error) {
      FinishFromHResult(state, error.code());
    }
  });
}

}  // namespace

void FlutterNativeSpeechToTextPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "com.zakaria5729/flutter_native_speech_to_text",
      &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FlutterNativeSpeechToTextPlugin>(registrar);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FlutterNativeSpeechToTextPlugin::FlutterNativeSpeechToTextPlugin(
    flutter::PluginRegistrarWindows *registrar)
    : registrar_(registrar), queue_(std::make_shared<MainThreadQueue>()) {
  std::shared_ptr<MainThreadQueue> queue = queue_;
  window_proc_id_ = registrar_->RegisterTopLevelWindowProcDelegate(
      [queue](HWND, UINT message, WPARAM, LPARAM) -> std::optional<LRESULT> {
        if (message == kRunTasksMessage) {
          queue->Run();
          return 0;
        }
        return std::nullopt;
      });
}

FlutterNativeSpeechToTextPlugin::~FlutterNativeSpeechToTextPlugin() {
  if (window_proc_id_ != -1) {
    registrar_->UnregisterTopLevelWindowProcDelegate(window_proc_id_);
  }
  queue_->Close();
}

void FlutterNativeSpeechToTextPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const std::string &method = method_call.method_name();
  const flutter::EncodableMap *args = nullptr;
  if (method_call.arguments() != nullptr) {
    args = std::get_if<flutter::EncodableMap>(method_call.arguments());
  }

  if (method == "isAvailable") {
    bool available = false;
    try {
      available = sr::SpeechRecognizer::SupportedTopicLanguages().Size() > 0;
    } catch (winrt::hresult_error const &) {
    }
    result->Success(flutter::EncodableValue(available));
  } else if (method == "isLanguageSupported") {
    std::string language = GetString(args, "language").value_or("");
    result->Success(flutter::EncodableValue(IsLanguageSupported(language)));
  } else if (method == "listen") {
    Listen(args, std::shared_ptr<Result>(std::move(result)));
  } else if (method == "stop") {
    if (active_) {
      try {
        active_->recognizer.ContinuousRecognitionSession().StopAsync();
      } catch (winrt::hresult_error const &) {
      }
    }
    result->Success();
  } else if (method == "cancel") {
    if (active_) {
      try {
        active_->recognizer.ContinuousRecognitionSession().CancelAsync();
      } catch (winrt::hresult_error const &) {
      }
      FinishNull(active_);
    }
    result->Success();
  } else {
    result->NotImplemented();
  }
}

void FlutterNativeSpeechToTextPlugin::Listen(
    const flutter::EncodableMap *args,
    std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const flutter::EncodableValue *enabled = FindValue(args, "isEnable");
  if (enabled != nullptr && std::get_if<bool>(enabled) != nullptr &&
      !std::get<bool>(*enabled)) {
    result->Error("DISABLED", "Speech to text recognition unavailable");
    return;
  }
  if (active_) {
    result->Error("ALREADY_LISTENING", "A recognition session is already in progress");
    return;
  }

  flutter::FlutterView *view = registrar_->GetView();
  if (view == nullptr) {
    result->Error("NO_CONTEXT", "The Flutter window is not available");
    return;
  }
  queue_->SetWindow(GetAncestor(view->GetNativeWindow(), GA_ROOT));

  std::string language = GetString(args, "language")
                             .value_or(GetString(args, "languagePreference")
                                           .value_or(kDefaultLanguage));
  if (!IsLanguageSupported(language)) {
    result->Error("LANGUAGE_NOT_SUPPORTED",
                  "The language '" + language +
                      "' is not supported by Windows speech recognition");
    return;
  }

  auto state = std::make_shared<ListenState>();
  state->queue = queue_;
  state->result = result;
  int max_results = GetInt(args, "maxResults").value_or(1);
  state->max_results = max_results < 1 ? 1 : max_results;
  state->on_finished = [this]() { active_.reset(); };

  try {
    state->recognizer = sr::SpeechRecognizer(wg::Language(winrt::hstring{Utf8ToWide(language)}));
    state->recognizer.Constraints().Append(sr::SpeechRecognitionTopicConstraint(
        sr::SpeechRecognitionScenario::Dictation, L"dictation"));
  } catch (winrt::hresult_error const &error) {
    state->on_finished = nullptr;
    FinishFromHResult(state, error.code());
    return;
  }

  active_ = state;
  int silence_millis =
      GetInt(args, "speechInputCompleteSilenceLengthMillis").value_or(kDefaultSilenceMillis);
  StartRecognition(state, silence_millis);
}

}  // namespace flutter_native_speech_to_text
