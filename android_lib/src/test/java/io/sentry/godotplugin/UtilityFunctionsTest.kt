package io.sentry.godotplugin

import io.sentry.SentryLevel
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertThrows
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

    @Test
    fun toIntOrThrow_acceptsIntegerAndLongBoxing() {
        val fromInteger: Any? = 42
        val fromLong: Any? = 42L

        assertEquals(42, fromInteger.toIntOrThrow())
        assertEquals(42, fromLong.toIntOrThrow())
    }

    @Test
    fun toIntOrThrow_throwsForNonIntegerValues() {
        assertThrows(IllegalArgumentException::class.java) { (null as Any?).toIntOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { ("42" as Any?).toIntOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { (4.2 as Any?).toIntOrThrow() }
    }

    @Test
    fun toLongOrThrow_acceptsIntegerAndLongBoxing() {
        val fromInteger: Any? = 42
        val fromLong: Any? = 42L

        assertEquals(42L, fromInteger.toLongOrThrow())
        assertEquals(42L, fromLong.toLongOrThrow())
    }

    @Test
    fun toLongOrThrow_throwsForNonIntegerValues() {
        assertThrows(IllegalArgumentException::class.java) { (null as Any?).toLongOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { ("42" as Any?).toLongOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { (4.2 as Any?).toLongOrThrow() }
    }

    @Test
    fun toDoubleOrThrow_acceptsEveryNumericBoxing() {
        val fromDouble: Any? = 0.25
        val fromFloat: Any? = 0.25f
        val fromInteger: Any? = 3
        val fromLong: Any? = 3L

        assertEquals(0.25, fromDouble.toDoubleOrThrow(), 0.0)
        assertEquals(0.25, fromFloat.toDoubleOrThrow(), 0.0)
        assertEquals(3.0, fromInteger.toDoubleOrThrow(), 0.0)
        assertEquals(3.0, fromLong.toDoubleOrThrow(), 0.0)
    }

    @Test
    fun toDoubleOrThrow_throwsForNonNumericValues() {
        assertThrows(IllegalArgumentException::class.java) { (null as Any?).toDoubleOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { ("0.25" as Any?).toDoubleOrThrow() }
        assertThrows(IllegalArgumentException::class.java) { (true as Any?).toDoubleOrThrow() }
    }

    @Test
    fun toIntOrNull_acceptsEveryNumericBoxingAndNumericStrings() {
        val fromInteger: Any? = 42
        val fromLong: Any? = 42L
        val fromShort: Any? = 42.toShort()
        val fromByte: Any? = 42.toByte()
        val fromDouble: Any? = 42.9
        val fromString: Any? = "42"

        assertEquals(42, fromInteger.toIntOrNull())
        assertEquals(42, fromLong.toIntOrNull())
        assertEquals(42, fromShort.toIntOrNull())
        assertEquals(42, fromByte.toIntOrNull())
        assertEquals(42, fromDouble.toIntOrNull())
        assertEquals(42, fromString.toIntOrNull())
    }

    @Test
    fun toIntOrNull_returnsNullForNonNumericValues() {
        assertNull((null as Any?).toIntOrNull())
        assertNull(("abc" as Any?).toIntOrNull())
        assertNull((true as Any?).toIntOrNull())
    }

    @Test
    fun toLongOrNull_acceptsEveryNumericBoxingAndNumericStrings() {
        val fromInteger: Any? = 42
        val fromLong: Any? = 42L
        val fromShort: Any? = 42.toShort()
        val fromByte: Any? = 42.toByte()
        val fromDouble: Any? = 42.9
        val fromString: Any? = "42"

        assertEquals(42L, fromInteger.toLongOrNull())
        assertEquals(42L, fromLong.toLongOrNull())
        assertEquals(42L, fromShort.toLongOrNull())
        assertEquals(42L, fromByte.toLongOrNull())
        assertEquals(42L, fromDouble.toLongOrNull())
        assertEquals(42L, fromString.toLongOrNull())
    }

    @Test
    fun toLongOrNull_returnsNullForNonNumericValues() {
        assertNull((null as Any?).toLongOrNull())
        assertNull(("abc" as Any?).toLongOrNull())
        assertNull((true as Any?).toLongOrNull())
    }
}
