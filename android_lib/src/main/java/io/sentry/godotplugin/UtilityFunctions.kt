package io.sentry.godotplugin

import io.sentry.SentryLevel
import java.util.Calendar
import java.util.Date
import java.util.Locale
import java.util.TimeZone

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

/**
 * Converts an RFC 3339 formatted string to Date.
 */
fun String.parseTimestamp(): Date {
    // Expecting inputs like these:
    // 2025-06-14T15:45:30Z
    // 2025-06-14T15:45:30.123Z
    // 2025-06-14T15:45:30.123456Z
    // 2025-06-14T15:45:30.123456+01:00
    // 2025-06-14T15:45:30.123456-05:30

    val datePart = substring(0, 10) // yyyy-MM-dd

    val offsetIndex = when {
        endsWith("Z") -> length - 1
        lastIndexOf('+') > 19 -> lastIndexOf('+')
        lastIndexOf('-') > 19 -> lastIndexOf('-')
        else -> throw IllegalArgumentException("Invalid timestamp format (expecting RFC 3339).")
    }

    val timeParts = substring(11, offsetIndex).split(".") // hh:mm:ss.sss => hh:mm:ss, sss
    val timePart = timeParts[0] // hh:mm:ss
    val fractionPart = if (timeParts.size > 1) { // sss
        timeParts[1]
    } else {
        ""
    }

    val offsetPart = substring(offsetIndex) // +hh:mm or -hh:mm or Z

    val (year, month, day) = datePart.split("-").map { it.toInt() }

    val (hour, minute, second) = timePart.split(":").map { it.toInt() }

    // Pad or trim fractional part to microseconds (6 digits). Examples: 123 => 123000, 123456789 => 123456.
    val fractionSixDigits = fractionPart.padEnd(6, '0').take(6).toInt()

    val millis = fractionSixDigits / 1000
//    val micros = fractionInMicros % 1000  // micros are ignored by Date

    val calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC")).apply {
        set(Calendar.YEAR, year)
        set(Calendar.MONTH, month - 1)
        set(Calendar.DAY_OF_MONTH, day)
        set(Calendar.HOUR_OF_DAY, hour)
        set(Calendar.MINUTE, minute)
        set(Calendar.SECOND, second)
        set(Calendar.MILLISECOND, millis)
    }

    val baseMillis = calendar.timeInMillis

    val offsetMillis = when {
        offsetPart == "Z" -> 0L
        else -> {
            // Offset like +hh:mm or -hh:mm
            val sign = if (offsetPart[0] == '+') 1 else -1
            val offsetHours = offsetPart.substring(1, 3).toInt()
            val offsetMinutes = offsetPart.substring(4, 6).toInt()
             sign * (offsetHours * 60 + offsetMinutes) * 60 * 1000L
        }
    }

    val utcMillis = baseMillis - offsetMillis

    return Date(utcMillis)
}

/**
 * Converts Date to an RFC 3339 formatted string.
 *
 * Example output: 2025-06-14T15:45:30.123Z
 * */
fun Date.toRfc3339(): String {
    val calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC")).apply {
        time = this@toRfc3339
    }

    val year = calendar.get(Calendar.YEAR)
    val month = calendar.get(Calendar.MONTH) + 1 // months are zero-based
    val day = calendar.get(Calendar.DAY_OF_MONTH)
    val hour = calendar.get(Calendar.HOUR_OF_DAY)
    val minute = calendar.get(Calendar.MINUTE)
    val second = calendar.get(Calendar.SECOND)
    val millis = calendar.get(Calendar.MILLISECOND)

    return String.format(
        Locale.ROOT,
        "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", //
        year, month, day, hour, minute, second, millis
    )
}
