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
    val micros: Long = this@microsecondsToTimestamp
    val seconds = micros / 1_000_000
    val nanos = (micros % 1_000_000) * 1000

    val instant = Instant.ofEpochSecond(seconds, nanos.toLong())
    return Date.from(instant)
}


fun Instant.toMicros(): Long {
    val instant: Instant = this@toMicros
    return instant.epochSecond * 1_000_000 + (instant.nano + 500) / 1_000
}
