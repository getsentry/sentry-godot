package io.sentry.godotplugin

import io.sentry.SentryLevel
import org.threeten.bp.LocalDateTime
import org.threeten.bp.OffsetDateTime
import org.threeten.bp.ZoneOffset
import org.threeten.bp.format.DateTimeFormatter
import java.util.Date
import org.threeten.bp.ZoneId

fun Int.toSentryLevel(): SentryLevel =
    when(this) {
        0 -> SentryLevel.DEBUG
        1 -> SentryLevel.INFO
        2 -> SentryLevel.WARNING
        3 -> SentryLevel.ERROR
        4 -> SentryLevel.FATAL
        else -> SentryLevel.ERROR
    }

fun SentryLevel.toInt(): Int =
    when(this) {
        SentryLevel.DEBUG -> 0
        SentryLevel.INFO -> 1
        SentryLevel.WARNING -> 2
        SentryLevel.ERROR -> 3
        SentryLevel.FATAL -> 4
    }

fun String.parseTimestamp(): Date {
    // TODO: Handle more formats.
    // Parse "2025-04-29T17:19:40" and interpret it as device's local time zone (not UTC).
    val ldt = LocalDateTime.parse(this, DateTimeFormatter.ISO_LOCAL_DATE_TIME)
    val zoneId = ZoneId.systemDefault()
    return Date(ldt.atZone(zoneId).toInstant().toEpochMilli())
}