import Foundation
import Speech

#if os(iOS)
import Flutter
#else
import FlutterMacOS
#endif

public class FlutterNativeSpeechToTextPlugin: NSObject, FlutterPlugin {
  private static let defaultLanguage = "bn-BD"
  private static let defaultSilenceMillis = 2000

  private var session: SpeechSession?
  private var isListening = false

  public static func register(with registrar: FlutterPluginRegistrar) {
    #if os(iOS)
    let messenger = registrar.messenger()
    #else
    let messenger = registrar.messenger
    #endif
    let channel = FlutterMethodChannel(
      name: "com.zakaria5729/flutter_native_speech_to_text",
      binaryMessenger: messenger
    )
    registrar.addMethodCallDelegate(FlutterNativeSpeechToTextPlugin(), channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "isAvailable":
      result(isAvailable())
    case "isLanguageSupported":
      let language = (call.arguments as? [String: Any])?["language"] as? String ?? ""
      result(isLanguageSupported(language))
    case "listen":
      listen(call.arguments as? [String: Any] ?? [:], result: result)
    case "stop":
      session?.stop()
      result(nil)
    case "cancel":
      session?.cancel()
      result(nil)
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  private func isAvailable() -> Bool {
    let status = SFSpeechRecognizer.authorizationStatus()
    if status == .denied || status == .restricted { return false }
    return !SFSpeechRecognizer.supportedLocales().isEmpty
  }

  private func isLanguageSupported(_ language: String) -> Bool {
    let wanted = normalize(language)
    if wanted.isEmpty { return false }
    let supported = SFSpeechRecognizer.supportedLocales().map { normalize($0.identifier) }
    if supported.contains(wanted) { return true }
    if !wanted.contains("-") {
      return supported.contains { $0.split(separator: "-").first.map(String.init) == wanted }
    }
    return false
  }

  private func normalize(_ tag: String) -> String {
    return tag.replacingOccurrences(of: "_", with: "-").lowercased()
  }

  private func listen(_ args: [String: Any], result: @escaping FlutterResult) {
    if args["isEnable"] as? Bool == false {
      result(FlutterError(
        code: "DISABLED", message: "Speech to text recognition unavailable", details: nil))
      return
    }
    if isListening {
      result(FlutterError(
        code: "ALREADY_LISTENING",
        message: "A recognition session is already in progress",
        details: nil))
      return
    }

    let language = args["language"] as? String
      ?? args["languagePreference"] as? String
      ?? FlutterNativeSpeechToTextPlugin.defaultLanguage
    guard let recognizer = SFSpeechRecognizer(locale: Locale(identifier: language)) else {
      result(FlutterError(
        code: "LANGUAGE_NOT_SUPPORTED",
        message: "The language '\(language)' is not supported by Apple speech recognition",
        details: nil))
      return
    }
    guard recognizer.isAvailable else {
      result(FlutterError(
        code: "NO_RECOGNIZER",
        message: "Speech recognition is currently unavailable for '\(language)'",
        details: nil))
      return
    }

    let maxResults = (args["maxResults"] as? NSNumber)?.intValue ?? 1
    let silenceMillis = (args["speechInputCompleteSilenceLengthMillis"] as? NSNumber)?.intValue
      ?? FlutterNativeSpeechToTextPlugin.defaultSilenceMillis

    isListening = true
    SpeechSession.requestPermissions { [weak self] failure in
      guard let self = self else { return }
      if let failure = failure {
        self.isListening = false
        result(FlutterError(code: failure.code, message: failure.message, details: nil))
        return
      }
      let session = SpeechSession(
        recognizer: recognizer,
        maxResults: maxResults,
        silenceInterval: TimeInterval(silenceMillis) / 1000)
      self.session = session
      session.start { [weak self] outcome in
        self?.isListening = false
        self?.session = nil
        switch outcome {
        case .success(let map):
          result(map)
        case .failure(let failure):
          result(FlutterError(code: failure.code, message: failure.message, details: nil))
        }
      }
    }
  }
}
