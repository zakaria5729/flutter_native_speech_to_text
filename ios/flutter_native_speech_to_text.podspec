Pod::Spec.new do |s|
  s.name             = 'flutter_native_speech_to_text'
  s.version          = '1.0.0'
  s.summary          = 'A Flutter plugin for native speech-to-text using Android\'s RecognizerIntent (Android only).'
  s.description      = <<-DESC
A Flutter plugin for native speech-to-text using Android's native RecognizerIntent.ACTION_RECOGNIZE_SPEECH.
iOS is not supported; all calls return PLATFORM_NOT_SUPPORTED error.
                       DESC
  s.homepage         = 'https://github.com/zakaria5729/flutter_native_speech_to_text'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'zakaria5729' => 'zakariahossain143@gmail.com' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.dependency 'Flutter'
  s.platform         = :ios, '13.0'
  s.swift_version    = '5.0'
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386',
  }
end