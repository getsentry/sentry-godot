package io.sentry.godotplugin

import io.sentry.SentryLevel
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

fun SentryLevel.toInt(): Int =
    when (this) {
        SentryLevel.DEBUG -> 0
        SentryLevel.INFO -> 1
        SentryLevel.WARNING -> 2
        SentryLevel.ERROR -> 3
        SentryLevel.FATAL -> 4
    }

fun Long.microsecondsToTimestamp(): Date {
    val millis = this / 1_000
    return Date(millis)
}


fun Date.toMicros(): Long {
    val date: Date = this@toMicros
    return date.time * 1000
}
