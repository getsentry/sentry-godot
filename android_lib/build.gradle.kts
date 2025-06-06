plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "io.sentry.godotplugin"
    compileSdk = 35

    defaultConfig {
        minSdk = 24

        consumerProguardFiles("consumer-rules.pro")

        setProperty("archivesBaseName", "sentry_android_godot_plugin")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("org.godotengine:godot:4.3.0.stable")

    // Note: Must also be updated in sentry_editor_export_plugin.cpp.
    implementation("io.sentry:sentry-android:8.11.1")
    implementation("com.jakewharton.threetenabp:threetenabp:1.4.9")
}

val copyLibraryToProject by tasks.registering(Copy::class) {
    description = "Copies the generated debug AAR to the project"
    from("build/outputs/aar")
    include("sentry_android_godot_plugin-debug.aar")
    into("../project/addons/sentry/bin/android/")
    rename("sentry_android_godot_plugin-debug.aar", "sentry_android_godot_plugin.aar")
}

tasks.named("assemble").configure {
    finalizedBy(copyLibraryToProject)
}
