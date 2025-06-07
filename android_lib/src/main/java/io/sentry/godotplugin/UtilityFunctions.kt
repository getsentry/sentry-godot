package io.sentry.godotplugin

import io.sentry.SentryLevel
import org.threeten.bp.OffsetDateTime
import org.threeten.bp.ZoneOffset
import org.threeten.bp.format.DateTimeFormatter
import java.util.Date

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

fun String.parseTimestamp(): Date {
    val offsetDateTime = OffsetDateTime.parse(this)
    return Date(offsetDateTime.toInstant().toEpochMilli())
}

fun Date.toRfc3339(): String {
    val instant = org.threeten.bp.Instant.ofEpochMilli(this.time)
    val offsetDateTime = OffsetDateTime.ofInstant(instant, ZoneOffset.UTC)
    return offsetDateTime.format(DateTimeFormatter.ISO_OFFSET_DATE_TIME)
}
