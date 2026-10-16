import AVFoundation
import Foundation
import Speech

struct SpeechFailure: Error {
  let code: String
  let message: String
}

final class SpeechSession {
  typealias Outcome = Result<[String: Any]?, SpeechFailure>

  private static let noSpeechTimeout: TimeInterval = 8
  private static let finalizeTimeout: TimeInterval = 3

  private let recognizer: SFSpeechRecognizer
  private let maxResults: Int
  private let silenceInterval: TimeInterval
  private let engine = AVAudioEngine()
  private var request: SFSpeechAudioBufferRecognitionRequest?
  private var task: SFSpeechRecognitionTask?
  private var latest: SFSpeechRecognitionResult?
  private var completion: ((Outcome) -> Void)?
  private var silenceWork: DispatchWorkItem?
  private var noSpeechWork: DispatchWorkItem?
  private var finalizeWork: DispatchWorkItem?

  init(recognizer: SFSpeechRecognizer, maxResults: Int, silenceInterval: TimeInterval) {
    self.recognizer = recognizer
    self.maxResults = max(maxResults, 1)
    self.silenceInterval = silenceInterval
  }

  static func requestPermissions(_ done: @escaping (SpeechFailure?) -> Void) {
    SFSpeechRecognizer.requestAuthorization { status in
      DispatchQueue.main.async {
        guard status == .authorized else {
          done(SpeechFailure(
            code: "PERMISSION_DENIED",
            message: "Speech recognition permission was denied"))
          return
        }
        requestMicrophone { granted in
          DispatchQueue.main.async {
            done(granted ? nil : SpeechFailure(
              code: "PERMISSION_DENIED",
              message: "Microphone permission was denied"))
          }
        }
      }
    }
  }

  private static func requestMicrophone(_ done: @escaping (Bool) -> Void) {
    #if os(iOS)
    AVAudioSession.sharedInstance().requestRecordPermission(done)
    #else
    AVCaptureDevice.requestAccess(for: .audio, completionHandler: done)
    #endif
  }

  func start(completion: @escaping (Outcome) -> Void) {
    self.completion = completion
    do {
      try startEngine()
    } catch let failure as SpeechFailure {
      finish(.failure(failure))
    } catch {
      finish(.failure(SpeechFailure(
        code: "ERROR",
        message: "Failed to start speech recognition: \(error.localizedDescription)")))
    }
  }

  func stop() {
    guard completion != nil else { return }
    endAudio()
  }

  func cancel() {
    finish(.success(nil))
  }

  private func startEngine() throws {
    #if os(iOS)
    let audioSession = AVAudioSession.sharedInstance()
    try audioSession.setCategory(.record, mode: .measurement, options: .duckOthers)
    try audioSession.setActive(true, options: .notifyOthersOnDeactivation)
    #endif

    let request = SFSpeechAudioBufferRecognitionRequest()
    request.shouldReportPartialResults = true
    self.request = request

    let input = engine.inputNode
    let format = input.outputFormat(forBus: 0)
    guard format.sampleRate > 0, format.channelCount > 0 else {
      throw SpeechFailure(
        code: "NO_MICROPHONE",
        message: "No microphone was found or it could not be used")
    }
    input.installTap(onBus: 0, bufferSize: 1024, format: format) { [weak request] buffer, _ in
      request?.append(buffer)
    }
    engine.prepare()
    try engine.start()

    task = recognizer.recognitionTask(with: request) { [weak self] result, error in
      DispatchQueue.main.async { self?.handle(result: result, error: error) }
    }

    let work = DispatchWorkItem { [weak self] in
      guard let self = self, self.latest == nil else { return }
      self.finish(.success(nil))
    }
    noSpeechWork = work
    DispatchQueue.main.asyncAfter(deadline: .now() + SpeechSession.noSpeechTimeout, execute: work)
  }

  private func handle(result: SFSpeechRecognitionResult?, error: Error?) {
    guard completion != nil else { return }
    if let result = result {
      latest = result
      if result.isFinal {
        finish(.success(map(result)))
        return
      }
      if !result.bestTranscription.formattedString.isEmpty {
        scheduleSilenceStop()
      }
    }
    if let error = error {
      if let latest = latest {
        finish(.success(map(latest)))
      } else if (error as NSError).code == 1110 {
        finish(.success(nil))
      } else {
        finish(.failure(SpeechFailure(code: "ERROR", message: error.localizedDescription)))
      }
    }
  }

  private func scheduleSilenceStop() {
    silenceWork?.cancel()
    let work = DispatchWorkItem { [weak self] in self?.endAudio() }
    silenceWork = work
    DispatchQueue.main.asyncAfter(deadline: .now() + silenceInterval, execute: work)
  }

  private func endAudio() {
    silenceWork?.cancel()
    stopEngine()
    request?.endAudio()
    finalizeWork?.cancel()
    let work = DispatchWorkItem { [weak self] in
      guard let self = self else { return }
      if let latest = self.latest {
        self.finish(.success(self.map(latest)))
      } else {
        self.finish(.success(nil))
      }
    }
    finalizeWork = work
    DispatchQueue.main.asyncAfter(deadline: .now() + SpeechSession.finalizeTimeout, execute: work)
  }

  private func stopEngine() {
    if engine.isRunning {
      engine.stop()
      engine.inputNode.removeTap(onBus: 0)
    }
  }

  private func finish(_ outcome: Outcome) {
    guard let completion = completion else { return }
    self.completion = nil
    silenceWork?.cancel()
    noSpeechWork?.cancel()
    finalizeWork?.cancel()
    stopEngine()
    task?.cancel()
    task = nil
    request = nil
    #if os(iOS)
    try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
    #endif
    completion(outcome)
  }

  private func map(_ result: SFSpeechRecognitionResult) -> [String: Any] {
    let transcriptions = Array(result.transcriptions.prefix(maxResults))
    let confidences = transcriptions.map { transcription -> Double in
      let segments = transcription.segments
      guard !segments.isEmpty else { return 0 }
      let total = segments.reduce(Float(0)) { $0 + $1.confidence }
      return Double(total) / Double(segments.count)
    }
    return [
      "recognizedText": result.bestTranscription.formattedString,
      "alternativeResults": transcriptions.map { $0.formattedString },
      "confidenceScores": confidences,
      "isFinal": true,
    ]
  }
}
