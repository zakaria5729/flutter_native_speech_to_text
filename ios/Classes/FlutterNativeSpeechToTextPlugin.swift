import Flutter
import Foundation

/// iOS stub implementation that safely returns errors instead of crashing.
/// This plugin is Android-only; iOS calls will return appropriate error codes.
public class FlutterNativeSpeechToTextPlugin: NSObject, FlutterPlugin {
  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(
      name: "com.zakaria5729/flutter_native_speech_to_text",
      binaryMessenger: registrar.messenger()
    )
    let instance = FlutterNativeSpeechToTextPlugin()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "isAvailable":
      // Always return false on iOS since this is Android-only
      result(false)
    case "listen":
      result(FlutterError(
        code: "PLATFORM_NOT_SUPPORTED",
        message: "Speech recognition via RecognizerIntent is not available on iOS. This plugin only supports Android.",
        details: nil
      ))
    case "stop":
      result(FlutterError(
        code: "PLATFORM_NOT_SUPPORTED",
        message: "Speech recognition via RecognizerIntent is not available on iOS.",
        details: nil
      ))
    case "cancel":
      result(FlutterError(
        code: "PLATFORM_NOT_SUPPORTED",
        message: "Speech recognition via RecognizerIntent is not available on iOS.",
        details: nil
      ))
    default:
      result(FlutterMethodNotImplemented)
    }
  }
}