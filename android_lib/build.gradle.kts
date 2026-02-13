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
    implementation("org.godotengine:godot:4.4.0.stable")
    testImplementation("junit:junit:4.13.2")

    // NOTE: All dependencies below must be also updated in sentry_editor_export_plugin.cpp.
    implementation("io.sentry:sentry-android:8.32.0")
}

val copyDebugAarToProject by tasks.registering(Copy::class) {
    description = "Copies generated debug AAR to project"
    from("build/outputs/aar")
    include("sentry_android_godot_plugin-debug.aar")
    into("../project/addons/sentry/bin/android/")
    rename("sentry_android_godot_plugin-debug.aar", "sentry_android_godot_plugin.debug.aar")
}

val copyReleaseAarToProject by tasks.registering(Copy::class) {
    description = "Copies generated release AAR to project"
    from("build/outputs/aar")
    include("sentry_android_godot_plugin-release.aar")
    into("../project/addons/sentry/bin/android/")
    rename("sentry_android_godot_plugin-release.aar", "sentry_android_godot_plugin.release.aar")
}

tasks.named("assemble").configure {
    finalizedBy(copyDebugAarToProject)
    finalizedBy(copyReleaseAarToProject)
}
