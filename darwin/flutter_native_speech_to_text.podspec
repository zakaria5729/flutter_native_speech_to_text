Pod::Spec.new do |s|
  s.name             = 'flutter_native_speech_to_text'
  s.version          = '1.3.0'
  s.summary          = 'Native speech-to-text for Flutter on Android, iOS, macOS and web.'
  s.description      = <<-DESC
Speech-to-text using Android's RecognizerIntent, Apple's Speech framework on iOS and macOS, and the Web Speech API on web.
                       DESC
  s.homepage         = 'https://github.com/zakaria5729/flutter_native_speech_to_text'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'zakaria5729' => 'zakariahossain143@gmail.com' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.ios.dependency 'Flutter'
  s.osx.dependency 'FlutterMacOS'
  s.ios.deployment_target = '13.0'
  s.osx.deployment_target = '10.15'
  s.swift_version    = '5.0'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
end
