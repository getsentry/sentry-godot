package io.sentry.godotplugin

import io.sentry.SentryLevel
import io.sentry.SentryLogLevel
import java.util.Date
import java.time.Instant

fun Int.toSentryLevel(): SentryLevel =
    when (this) {
        0 -> SentryLevel.DEBUG
        1 -> SentryLevel.INFO
        2 -> SentryLevel.WARNING
        3 -> SentryLevel.ERROR
        4 -> SentryLevel.FATAL
        else -> SentryLevel.ERROR
    }

fun Int.toSentryLogLevel(): SentryLogLevel =
    when (this) {
        0 -> SentryLogLevel.TRACE
        1 -> SentryLogLevel.DEBUG
        2 -> SentryLogLevel.INFO
        3 -> SentryLogLevel.WARN
        4 -> SentryLogLevel.ERROR
        5 -> SentryLogLevel.FATAL
        else -> SentryLogLevel.INFO
    }

fun SentryLevel.toInt(): Int =
    when (this) {
        SentryLevel.DEBUG -> 0
        SentryLevel.INFO -> 1
        SentryLevel.WARNING -> 2
        SentryLevel.ERROR -> 3
        SentryLevel.FATAL -> 4
    }

fun SentryLogLevel.toInt(): Int =
    when (this) {
        SentryLogLevel.TRACE -> 0
        SentryLogLevel.DEBUG -> 1
        SentryLogLevel.INFO -> 2
        SentryLogLevel.WARN -> 3
        SentryLogLevel.ERROR -> 4
        SentryLogLevel.FATAL -> 5
    }

fun Long.microsecondsToTimestamp(): Date {
    val millis = this / 1_000
    return Date(millis)
}


fun Date.toMicros(): Long {
    val date: Date = this@toMicros
    return date.time * 1000
}
