package io.sentry.godotplugin

import io.sentry.SentryLevel
import org.junit.Assert.assertEquals
import org.junit.Test
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
        val expected = Date(0L)
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsExactSecondsCorrectly() {
        val microseconds = 1_000_000L // 1 second in microseconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date(1000L) // 1 second in milliseconds
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsWithMicrosecondsCorrectly() {
        val microseconds = 1_500_750L // 1.500750 seconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date(1500L) // Truncated to 1.5 seconds (1500 milliseconds)
        assertEquals(expected, result)
    }

    @Test
    fun microsecondsToTimestamp_convertsLargeValueCorrectly() {
        val microseconds = 1_672_531_200_000_000L // January 1, 2023 00:00:00 UTC in microseconds
        val result = microseconds.microsecondsToTimestamp()
        val expected = Date(1_672_531_200_000L) // January 1, 2023 00:00:00 UTC in milliseconds
        assertEquals(expected, result)
    }

    @Test
    fun dateToMicros_convertsZeroCorrectly() {
        val date = Date(0L)
        val result = date.toMicros()
        assertEquals(0L, result)
    }

    @Test
    fun dateToMicros_convertsExactMillisecondsCorrectly() {
        val date = Date(1000L) // 1 second in milliseconds
        val result = date.toMicros()
        assertEquals(1_000_000L, result) // 1 second in microseconds
    }

    @Test
    fun dateToMicros_convertsWithMillisecondsCorrectly() {
        val date = Date(1500L) // 1.5 seconds in milliseconds
        val result = date.toMicros()
        assertEquals(1_500_000L, result) // 1.5 seconds in microseconds
    }

    @Test
    fun dateToMicros_convertsLargeValueCorrectly() {
        val date = Date(1_672_531_200_000L) // January 1, 2023 00:00:00 UTC in milliseconds
        val result = date.toMicros()
        assertEquals(1_672_531_200_000_000L, result) // January 1, 2023 00:00:00 UTC in microseconds
    }

    @Test
    fun bidirectionalConversion_maintainsAccuracy() {
        val originalMicros = 1_672_531_200_500_000L // January 1, 2023 00:00:00.500000 UTC (exact millisecond)
        val date = originalMicros.microsecondsToTimestamp()
        val convertedMicros = date.toMicros()

        // For microsecond values that align with millisecond boundaries, conversion should be precise
        assertEquals(originalMicros, convertedMicros)
    }

    @Test
    fun bidirectionalConversion_showsPrecisionLimitations() {
        val originalMicros = 1_672_531_200_500_750L // January 1, 2023 00:00:00.500750 UTC (sub-millisecond precision)
        val date = originalMicros.microsecondsToTimestamp()
        val convertedMicros = date.toMicros()

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
