package io.sentry.godotplugin

import android.util.Log
import com.jakewharton.threetenabp.AndroidThreeTen
import io.sentry.Attachment
import io.sentry.Breadcrumb
import io.sentry.Sentry
import io.sentry.SentryEvent
import io.sentry.SentryLevel
import io.sentry.android.core.SentryAndroid
import io.sentry.protocol.Message
import io.sentry.protocol.SentryException
import io.sentry.protocol.SentryId
import io.sentry.protocol.SentryStackFrame
import io.sentry.protocol.SentryStackTrace
import io.sentry.protocol.User
import org.godotengine.godot.Dictionary
import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.plugin.UsedByGodot
import org.threeten.bp.format.DateTimeParseException
import java.util.UUID


class SentryAndroidGodotPlugin(godot: Godot) : GodotPlugin(godot) {
    companion object {
        private const val TAG = "sentry-godot"
    }

    private val eventsById = mutableMapOf<String, SentryEvent>()
    private val exceptionsById = mutableMapOf<String, SentryException>()

    private fun getEvent(eventId: String): SentryEvent? {
        val event: SentryEvent? = eventsById[eventId]
        if (event == null) {
            Log.e(TAG, "Internal Error -- SentryEvent not found: $eventId")
        }
        return event
    }

    private fun getException(exceptionId: String): SentryException? {
        val exception: SentryException? = exceptionsById[exceptionId]
        if (exception == null) {
            Log.e(TAG, "Internal Error -- SentryException not found: $exceptionId")
        }
        return exception
    }

    override fun getPluginName(): String {
        return "SentryAndroidGodotPlugin";
    }

    override fun onGodotSetupCompleted() {
        super.onGodotSetupCompleted()
        AndroidThreeTen.init(this.activity!!.application) // needed for date time zones
    }

    @UsedByGodot
    fun initialize(
        dsn: String,
        debug: Boolean,
        release: String,
        dist: String,
        environment: String,
        sampleRate: Float,
        maxBreadcrumbs: Int,
    ) {
        Log.v(TAG, "Initializing Sentry Android")
        SentryAndroid.init(godot.getActivity()!!.applicationContext) { options ->
            options.dsn = dsn.ifEmpty { null }
            options.isDebug = debug
            options.release = release.ifEmpty { null }
            options.dist = dist.ifEmpty { null }
            options.environment = environment.ifEmpty { null }
            options.sampleRate = sampleRate.toDouble()
            options.maxBreadcrumbs = maxBreadcrumbs
            options.sdkVersion!!.name = "sentry.java.android.godot"
        }
    }

    @UsedByGodot
    fun addGlobalAttachment(path: String) {
        val attachment = Attachment(path)
        Sentry.getGlobalScope().addAttachment(attachment)
    }

    @UsedByGodot
    fun setContext(key: String, value: Dictionary) {
        Sentry.getGlobalScope().setContexts(key, value)
    }

    @UsedByGodot
    fun removeContext(key: String) {
        // Q: Godot SDK has no notion of scopes, should we set it on the global scope?
        Sentry.getGlobalScope().removeContexts(key)
    }

    @UsedByGodot
    fun setTag(key: String, value: String) {
        Sentry.setTag(key, value)
    }

    @UsedByGodot
    fun removeTag(key: String) {
        Sentry.removeTag(key)
    }

    @UsedByGodot
    fun setUser(id: String, name: String, email: String, ipAddress: String) {
        val user = User()
        if (id.isNotEmpty()) {
            user.id = id
        }
        if (name.isNotEmpty()) {
            user.name = name
        }
        if (email.isNotEmpty()) {
            user.email = email
        }
        if (ipAddress.isNotEmpty()) {
            user.ipAddress = ipAddress
        }
        Sentry.setUser(user)
    }

    @UsedByGodot
    fun removeUser() {
        Sentry.setUser(null)
    }

    @UsedByGodot
    fun addBreadcrumb(
        message: String,
        category: String,
        level: Int,
        type: String,
        data: Dictionary
    ) {
        val crumb = Breadcrumb()
        crumb.message = message
        crumb.category = category
        crumb.level = level.toSentryLevel()
        crumb.type = type

        val keys = data.keys.iterator()
        while (keys.hasNext()) {
            val k = keys.next()
            val v = data[k]
            crumb.data[k] = v
        }

        Sentry.addBreadcrumb(crumb)
    }

    @UsedByGodot
    fun captureMessage(message: String, level: Int): String {
        val id = Sentry.captureMessage(message, level.toSentryLevel())
        return id.toString()
    }

    @UsedByGodot
    fun getLastEventId(): String {
        return Sentry.getLastEventId().toString()
    }

    @UsedByGodot
    fun createEvent(): String {
        val event = SentryEvent()
        val id = UUID.randomUUID()
        event.eventId = SentryId(id)
        eventsById[id.toString()] = event
        return id.toString()
    }

    @UsedByGodot
    fun releaseEvent(eventId: String) {
        eventsById.remove(eventId)
    }

    @UsedByGodot
    fun captureEvent(eventId: String): String {
        val event: SentryEvent? = getEvent(eventId)
        if (event == null) {
            Log.e(TAG, "Failed to capture event: $eventId")
            return ""
        }
        val id = Sentry.captureEvent(event)
        return id.toString()
    }

    @UsedByGodot
    fun eventSetMessage(eventId: String, message: String) {
        val event: SentryEvent = getEvent(eventId) ?: return
        val sentryMessage = Message()
        sentryMessage.message = message
        event.message = sentryMessage
    }

    @UsedByGodot
    fun eventGetMessage(eventId: String): String {
        return getEvent(eventId)?.message?.message ?: return ""
    }

    @UsedByGodot
    fun eventSetTimestamp(eventId: String, timestamp: String) {
        // TODO: Revise how timestamps are handled.
        val event: SentryEvent = getEvent(eventId) ?: return
        try {
            event.timestamp = timestamp.parseTimestamp()
        } catch (e: DateTimeParseException) {
            Log.e(TAG, "Failed to parse datetime: $timestamp")
        }
    }

    @UsedByGodot
    fun eventGetTimestamp(eventId: String): String {
        val event: SentryEvent = getEvent(eventId) ?: return ""
        // TODO: Figure out proper format.
        return event.timestamp?.toString() ?: ""
    }

    @UsedByGodot
    fun eventGetPlatform(eventId: String): String {
        return getEvent(eventId)?.platform ?: return ""
    }

    @UsedByGodot
    fun eventSetLevel(eventId: String, level: Int) {
        getEvent(eventId)?.level = level.toSentryLevel()
    }

    @UsedByGodot
    fun eventGetLevel(eventId: String): Int {
        return getEvent(eventId)?.level?.toInt() ?: return SentryLevel.ERROR.toInt()
    }

    @UsedByGodot
    fun eventSetLogger(eventId: String, logger: String) {
        getEvent(eventId)?.logger = logger
    }

    @UsedByGodot
    fun eventGetLogger(eventId: String): String {
        return getEvent(eventId)?.logger ?: return ""
    }

    @UsedByGodot
    fun eventSetRelease(eventId: String, release: String) {
        getEvent(eventId)?.release = release
    }

    @UsedByGodot
    fun eventGetRelease(eventId: String): String {
        return getEvent(eventId)?.release ?: ""
    }

    @UsedByGodot
    fun eventSetDist(eventId: String, dist: String) {
        getEvent(eventId)?.dist = dist
    }

    @UsedByGodot
    fun eventGetDist(eventId: String): String {
        return getEvent(eventId)?.dist ?: ""
    }

    @UsedByGodot
    fun eventSetEnvironment(eventId: String, environment: String) {
        getEvent(eventId)?.environment = environment
    }

    @UsedByGodot
    fun eventGetEnvironment(eventId: String): String {
        return getEvent(eventId)?.environment ?: ""
    }

    @UsedByGodot
    fun eventSetTag(eventId: String, key: String, value: String) {
        getEvent(eventId)?.setTag(key, value)
    }

    @UsedByGodot
    fun eventGetTag(eventId: String, key: String): String {
        return getEvent(eventId)?.getTag(key) ?: ""
    }

    @UsedByGodot
    fun eventRemoveTag(eventId: String, key: String) {
        getEvent(eventId)?.removeTag(key)
    }

    @UsedByGodot
    fun createException(type: String, value: String): String {
        val exception = SentryException()
        exception.type = type
        exception.value = value
        exception.stacktrace = SentryStackTrace()
        exception.stacktrace!!.frames = mutableListOf()
        val id = UUID.randomUUID().toString()
        exceptionsById[id] = exception
        return id
    }

    @UsedByGodot
    fun releaseException(exceptionId: String) {
        exceptionsById.remove(exceptionId)
    }

    @UsedByGodot
    fun exceptionAppendStackFrame(exceptionId: String, frameData: Dictionary) {
        val exception = getException(exceptionId) ?: return
        val frame = SentryStackFrame()
        frame.filename = frameData["filename"] as? String
        frame.function = frameData["function"] as? String
        frame.lineno = frameData["lineno"] as? Int
        frame.contextLine = frameData["context_line"] as? String

        @Suppress("UNCHECKED_CAST")
        val preContext: Array<String> = frameData["pre_context"] as Array<String>
        if (preContext.isNotEmpty()) {
            frame.preContext = preContext.toList()
        }

        @Suppress("UNCHECKED_CAST")
        val postContext: Array<String> = frameData["post_context"] as Array<String>
        if (postContext.isNotEmpty()) {
            frame.postContext = postContext.toList()
        }

        exception.stacktrace!!.frames!!.add(frame)
    }

    @UsedByGodot
    fun eventAddException(eventId: String, exceptionId: String) {
        val event = getEvent(eventId) ?: return
        val exception = getException(exceptionId) ?: return
        if (event.exceptions == null) {
            event.exceptions = mutableListOf()
        }
        event.exceptions!!.add(exception)
    }
}