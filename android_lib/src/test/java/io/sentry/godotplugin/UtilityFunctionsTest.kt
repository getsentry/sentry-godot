package io.sentry.godotplugin

import io.sentry.SentryLevel
import org.junit.Assert.assertEquals
import org.junit.Test
import java.time.Instant
import java.util.Date
import kotlin.math.abs

class UtilityFunctionsTest {

    @Test
    fun toSentryLevel_convertsCorrectlyWithValidValues() {
        assertEquals(SentryLevel.DEBUG, 0.toSentryLevel())
        assertEquals(SentryLevel.INFO, 1.toSentryLevel())
        assertEquals(SentryLevel.WARNING, 2.toSentryLevel())
        assertEquals(SentryLevel.ERROR, 3.toSentryLevel())
        assertEquals(SentryLevel.FATAL, 4.toSentryLevel())
    }

    @Test
    fun toSentryLevel_defaultsToErrorForInvalidValues() {
        assertEquals(SentryLevel.ERROR, 5.toSentryLevel())
        assertEquals(SentryLevel.ERROR, (-1).toSentryLevel())
        assertEquals(SentryLevel.ERROR, 999.toSentryLevel())
    }

    @Test
    fun sentryLevelToInt_convertsAllLevelsCorrectly() {
        assertEquals(0, SentryLevel.DEBUG.toInt())
        assertEquals(1, SentryLevel.INFO.toInt())
        assertEquals(2, SentryLevel.WARNING.toInt())
        assertEquals(3, SentryLevel.ERROR.toInt())
        assertEquals(4, SentryLevel.FATAL.toInt())
    }

    @Test
    fun microsecondsToTimestamp_convertsZeroCorrectly() {
        val result = 0L.microsecondsToTimestamp()
        val expected = Date.from(Instant.ofEpochSecond(0, 0))
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsExactSecondsCorrectly() {
        val microseconds = 1_000_000L // 1 second in microseconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date.from(Instant.ofEpochSecond(1, 0))
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsWithMicrosecondsCorrectly() {
        val microseconds = 1_500_750L // 1.500750 seconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date.from(Instant.ofEpochSecond(1, 500_750_000)) // 500750 microseconds = 500750000 nanoseconds
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsLargeValueCorrectly() {
        val microseconds = 1_672_531_200_000_000L // January 1, 2023 00:00:00 UTC in microseconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date.from(Instant.ofEpochSecond(1_672_531_200, 0))
        assertEquals(expected, result)
    }

    @Test
    fun instantToMicros_convertsZeroCorrectly() {
        val instant = Instant.ofEpochSecond(0, 0)
        val result = instant.toMicros()
        assertEquals(0L, result)
    }

    @Test
    fun instantToMicros_convertsExactSecondsCorrectly() {
        val instant = Instant.ofEpochSecond(1, 0)
        val result = instant.toMicros()
        assertEquals(1_000_000L, result)
    }

    @Test
    fun instantToMicros_convertsWithNanosecondsCorrectly() {
        val instant = Instant.ofEpochSecond(1, 500_750_000) // 500750 microseconds
        val result = instant.toMicros()
        assertEquals(1_500_750L, result) // 500_750_000 nanos = 500750 micros exactly
    }

    @Test
    fun instantToMicros_convertsLargeValueCorrectly() {
        val instant = Instant.ofEpochSecond(1_672_531_200, 0) // January 1, 2023 00:00:00 UTC
        val result = instant.toMicros()
        assertEquals(1_672_531_200_000_000L, result)
    }

    @Test
    fun bidirectionalConversion_maintainsAccuracy() {
        val originalMicros = 1_672_531_200_500_000L // January 1, 2023 00:00:00.500000 UTC (exact millisecond)
        val date = originalMicros.microsecondsToTimestamp()
        val instant = date.toInstant()
        val convertedMicros = instant.toMicros()

        // For microsecond values that align with millisecond boundaries, conversion should be precise
        assertEquals(originalMicros, convertedMicros)
    }

    @Test
    fun bidirectionalConversion_showsPrecisionLimitations() {
        val originalMicros = 1_672_531_200_500_750L // January 1, 2023 00:00:00.500750 UTC (sub-millisecond precision)
        val date = originalMicros.microsecondsToTimestamp()
        val instant = date.toInstant()
        val convertedMicros = instant.toMicros()

        // Due to Date/Instant precision limitations, sub-millisecond precision is lost
        // The result should be rounded to the nearest millisecond (500000 microseconds)
        assertEquals(1_672_531_200_500_000L, convertedMicros)

        // The difference should be exactly the sub-millisecond part
        val difference = abs(originalMicros - convertedMicros)
        assertEquals(750L, difference)
    }

    @Test
    fun sentryLevelConversion_isSymmetric() {
        // Test that converting from int to SentryLevel and back gives the same result
        for (i in 0..4) {
            val sentryLevel = i.toSentryLevel()
            val backToInt = sentryLevel.toInt()
            assertEquals("Conversion should be symmetric for value $i", i, backToInt)
        }
    }
}
