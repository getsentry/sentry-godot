package io.sentry.godotplugin;

import org.junit.Assert.assertEquals
import org.junit.Test
import java.util.Calendar
import java.util.TimeZone

public class TimestampConversionTests {

    @Test
    fun parseTimestamp_withZuluTimeAndNoFraction() {
        val timestamp = "2025-06-14T17:45:30Z"
        val expectedMillis = 1749923130000L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withZuluTimeAndFraction() {
        val timestamp = "2025-06-14T17:45:30.123Z"
        val expectedMillis = 1749923130123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withZuluTimeAndFullFraction() {
        val timestamp = "2025-06-14T17:45:30.12356789Z"
        val expectedMillis = 1749923130123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withPositiveOffsetAndNoFraction() {
        val timestamp = "2025-06-14T17:45:30+02:15"
        val expectedMillis = 1749915030000L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withPositiveOffsetAndFraction() {
        val timestamp = "2025-06-14T17:45:30.123+02:15"
        val expectedMillis = 1749915030123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withPositiveOffsetAndFullFraction() {
        val timestamp = "2025-06-14T17:45:30.12356789+02:15"
        val expectedMillis = 1749915030123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withNegativeOffsetAndNoFraction() {
        val timestamp = "2025-06-14T17:45:30-01:45"
        val expectedMillis = 1749929430000L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withNegativeOffsetAndFraction() {
        val timestamp = "2025-06-14T17:45:30.123-01:45"
        val expectedMillis = 1749929430123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun parseTimestamp_withNegativeOffsetAndFullFraction() {
        val timestamp = "2025-06-14T17:45:30.123-01:45"
        val expectedMillis = 1749929430123L
        assertEquals(expectedMillis, timestamp.parseTimestamp().toInstant().toEpochMilli())
    }

    @Test
    fun toRfc3339_withMilliseconds() {
        val calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC")).apply {
            set(2025, 5, 14, 17, 45, 30) // months are zero-based
            set(Calendar.MILLISECOND, 123)
        }
        val date = calendar.time
        assertEquals("2025-06-14T17:45:30.123Z", date.toRfc3339())
    }
}
