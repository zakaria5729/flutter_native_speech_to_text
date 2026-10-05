plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

group = "com.zakaria5729.flutter_native_speech_to_text"
version = "1.0.0"

android {
    namespace = "com.zakaria5729.flutter_native_speech_to_text"
    compileSdk = 36

    defaultConfig {
        minSdk = 24
        consumerProguardFiles("consumer-rules.pro")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

kotlin {
    compilerOptions {
        jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
    }
}
