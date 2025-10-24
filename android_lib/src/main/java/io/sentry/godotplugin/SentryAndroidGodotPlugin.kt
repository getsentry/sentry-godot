package io.sentry.godotplugin

import android.util.Log
import io.sentry.Attachment
import io.sentry.Breadcrumb
import io.sentry.Hint
import io.sentry.ISerializer
import io.sentry.Sentry
import io.sentry.SentryAttributes
import io.sentry.SentryEvent
import io.sentry.SentryLevel
import io.sentry.SentryLogEvent
import io.sentry.SentryLogEventAttributeValue
import io.sentry.SentryLogLevel
import io.sentry.SentryOptions
import io.sentry.android.core.SentryAndroid
import io.sentry.logger.SentryLogParameters
import io.sentry.protocol.Feedback
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
import org.godotengine.godot.variant.Callable
import java.io.File
import java.io.StringWriter
import kotlin.random.Random


@Suppress("unused")
class SentryAndroidGodotPlugin(godot: Godot) : GodotPlugin(godot) {
    companion object {
        private const val TAG = "sentry-godot"
    }

    private val eventsByHandle = object : ThreadLocal<MutableMap<Int, SentryEvent>>() {
        override fun initialValue(): MutableMap<Int, SentryEvent> {
            return mutableMapOf()
        }
    }

    private val exceptionsByHandle = object : ThreadLocal<MutableMap<Int, SentryException>>() {
        override fun initialValue(): MutableMap<Int, SentryException> {
            return mutableMapOf()
        }
    }

    private val breadcrumbsByHandle = object : ThreadLocal<MutableMap<Int, Breadcrumb>>() {
        override fun initialValue(): MutableMap<Int, Breadcrumb> {
            return mutableMapOf()
        }
    }

    private val logsByHandle = object : ThreadLocal<MutableMap<Int, SentryLogEvent>>() {
        override fun initialValue(): MutableMap<Int, SentryLogEvent> {
            return mutableMapOf()
        }
    }

    private fun getEvent(eventHandle: Int): SentryEvent? {
        val event: SentryEvent? = eventsByHandle.get()?.get(eventHandle)
        if (event == null) {
            Log.e(TAG, "Internal Error -- SentryEvent not found: $eventHandle")
        }
        return event
    }

    private fun getException(exceptionHandle: Int): SentryException? {
        val exception: SentryException? = exceptionsByHandle.get()?.get(exceptionHandle)
        if (exception == null) {
            Log.e(TAG, "Internal Error -- SentryException not found: $exceptionHandle")
        }
        return exception
    }

    private fun getBreadcrumb(breadcrumbHandle: Int): Breadcrumb? {
        var crumb: Breadcrumb? = breadcrumbsByHandle.get()?.get(breadcrumbHandle)
        if (crumb == null) {
            Log.e(TAG, "Internal Error -- Breadcrumb not found: $breadcrumbHandle")
        }
        return crumb
    }

    private fun getLog(logHandle: Int): SentryLogEvent? {
        var logEvent: SentryLogEvent? = logsByHandle.get()?.get(logHandle)
        if (logEvent == null) {
            Log.e(TAG, "Internal Error -- SentryLogEvent not found: $logHandle")
        }
        return logEvent
    }

    private fun registerEvent(event: SentryEvent): Int {
        val eventsMap = eventsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- eventsByHandle is null")
            return 0
        }

        var handle = Random.nextInt()
        while (eventsMap.containsKey(handle)) {
            handle = Random.nextInt()
        }

        eventsMap[handle] = event
        return handle
    }

    private fun registerException(exception: SentryException): Int {
        val exceptionsMap = exceptionsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- exceptionsByHandle is null")
            return 0
        }

        var handle = Random.nextInt()
        while (exceptionsMap.containsKey(handle)) {
            handle = Random.nextInt()
        }

        exceptionsMap[handle] = exception
        return handle
    }

    private fun registerBreadcrumb(crumb: Breadcrumb): Int {
        val breadcrumbsMap = breadcrumbsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- breadcrumbsByHandle is null")
            return 0
        }

        var handle = Random.nextInt()
        while (breadcrumbsMap.containsKey(handle)) {
            handle = Random.nextInt()
        }

        breadcrumbsMap[handle] = crumb
        return handle
    }

    private fun registerLog(logEvent: SentryLogEvent): Int {
        var logsMap = logsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- logsMap is null")
            return 0
        }

        var handle = Random.nextInt()
        while (logsMap.containsKey(handle)) {
            handle = Random.nextInt()
        }

        logsMap[handle] = logEvent
        return handle
    }

    override fun getPluginName(): String {
        return "SentryAndroidGodotPlugin"
    }

    @UsedByGodot
    fun init(
        beforeSendHandlerId: Long,
        dsn: String,
        debug: Boolean,
        release: String,
        dist: String,
        environment: String,
        sampleRate: Float,
        maxBreadcrumbs: Int,
        enableLogs: Boolean,
        beforeSendLogHandlerId: Long
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
            options.sdkVersion?.name = "sentry.java.android.godot"
            options.nativeSdkName = "sentry.native.android.godot"
            options.logs.isEnabled = enableLogs
            options.beforeSend =
                SentryOptions.BeforeSendCallback { event: SentryEvent, hint: Hint ->
                    Log.v(TAG, "beforeSend: ${event.eventId} isCrashed: ${event.isCrashed}")
                    val handle: Int = registerEvent(event)
                    Callable.call(beforeSendHandlerId, "before_send", handle)
                    eventsByHandle.get()?.remove(handle) // Returns the event or null if it was discarded.
                }
            if (beforeSendLogHandlerId != 0L) {
                options.logs.beforeSend =
                    SentryOptions.Logs.BeforeSendLogCallback { logEvent ->
                        val handle: Int = registerLog(logEvent)
                        Callable.call(beforeSendLogHandlerId, "before_send_log", handle)
                        logsByHandle.get()?.remove(handle) // Returns the log or null if it was discarded.
                    }
            }

        }
    }

    @UsedByGodot
    fun close() {
        Sentry.close()
    }

    @UsedByGodot
    fun isEnabled(): Boolean {
      return Sentry.isEnabled()
    }

    @UsedByGodot
    fun addFileAttachment(path: String, filename: String, contentType: String, attachmentType: String) {
        val attachment = Attachment(
            path,
            filename.ifEmpty { File(path).name },
            contentType.ifEmpty { null },
            attachmentType.ifEmpty { null },
            false
        )
        Sentry.getGlobalScope().addAttachment(attachment)
    }

    @UsedByGodot
    fun addBytesAttachment(bytes: ByteArray, filename: String, contentType: String, attachmentType: String) {
        val attachment = Attachment(
            bytes,
            filename,
            contentType.ifEmpty { null },
            attachmentType.ifEmpty { null },
            false
        )
        Sentry.getGlobalScope().addAttachment(attachment)
    }

    @UsedByGodot
    fun setContext(key: String, value: Dictionary) {
        Sentry.getGlobalScope().setContexts(key, value)
    }

    @UsedByGodot
    fun removeContext(key: String) {
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
    fun setUser(id: String, userName: String, email: String, ipAddress: String) {
        val user = User()
        if (id.isNotEmpty()) {
            user.id = id
        }
        if (userName.isNotEmpty()) {
            user.username = userName
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
    fun addBreadcrumb(handle: Int) {
        val crumb = getBreadcrumb(handle) ?: return
        Sentry.addBreadcrumb(crumb)
    }

    @UsedByGodot
    fun log(level: Int, body: String, attributes: Dictionary) {
        if (attributes.isEmpty()) {
            Sentry.logger().log(level.toSentryLogLevel(), body)
        } else {
            val sentryAttributes = SentryAttributes.fromMap(attributes)
            Sentry.logger().log(
                level.toSentryLogLevel(),
                SentryLogParameters.create(sentryAttributes),
                body
            )
        }
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
    fun createEvent(): Int {
        val event = SentryEvent()
        val handle = registerEvent(event)
        return handle
    }

    @UsedByGodot
    fun releaseEvent(eventHandle: Int) {
        val eventsMap = eventsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- eventsByHandle is null")
            return
        }

        eventsMap.remove(eventHandle)
    }

    @UsedByGodot
    fun captureEvent(eventHandle: Int): String {
        val event: SentryEvent = getEvent(eventHandle) ?: run {
            Log.e(TAG, "Failed to capture event: $eventHandle")
            return ""
        }
        val id = Sentry.captureEvent(event)
        return id.toString()
    }

    @UsedByGodot
    fun captureFeedback(message: String, contactEmail: String, name: String, associatedEventId: String) {
        val feedback = Feedback(message)
        feedback.contactEmail = contactEmail.ifEmpty { null }
        feedback.name = name.ifEmpty { null }
        if (associatedEventId.isNotEmpty()) {
            feedback.setAssociatedEventId(SentryId(associatedEventId))
        }
        Sentry.captureFeedback(feedback)
    }

    @UsedByGodot
    fun eventGetId(eventHandle: Int): String {
        val id = getEvent(eventHandle)?.eventId ?: return ""
        return id.toString()
    }

    @UsedByGodot
    fun eventSetMessage(eventHandle: Int, message: String) {
        val event: SentryEvent = getEvent(eventHandle) ?: return
        val sentryMessage = Message()
        sentryMessage.formatted = message
        event.message = sentryMessage
    }

    @UsedByGodot
    fun eventGetMessage(eventHandle: Int): String {
        return getEvent(eventHandle)?.message?.formatted ?: return ""
    }

    @UsedByGodot
    fun eventSetTimestamp(eventHandle: Int, microsSinceUnixEpoch: Long) {
        val event: SentryEvent = getEvent(eventHandle) ?: return
        event.timestamp = microsSinceUnixEpoch.microsecondsToTimestamp()
    }

    @UsedByGodot
    fun eventGetTimestamp(eventHandle: Int): Long {
        val event: SentryEvent = getEvent(eventHandle) ?: return 0
        return event.timestamp?.toMicros() ?: 0
    }

    @UsedByGodot
    fun eventGetPlatform(eventHandle: Int): String {
        return getEvent(eventHandle)?.platform ?: return ""
    }

    @UsedByGodot
    fun eventSetLevel(eventHandle: Int, level: Int) {
        getEvent(eventHandle)?.level = level.toSentryLevel()
    }

    @UsedByGodot
    fun eventGetLevel(eventHandle: Int): Int {
        return getEvent(eventHandle)?.level?.toInt() ?: return SentryLevel.ERROR.toInt()
    }

    @UsedByGodot
    fun eventSetLogger(eventHandle: Int, logger: String) {
        getEvent(eventHandle)?.logger = logger
    }

    @UsedByGodot
    fun eventGetLogger(eventHandle: Int): String {
        return getEvent(eventHandle)?.logger ?: return ""
    }

    @UsedByGodot
    fun eventSetRelease(eventHandle: Int, release: String) {
        getEvent(eventHandle)?.release = release
    }

    @UsedByGodot
    fun eventGetRelease(eventHandle: Int): String {
        return getEvent(eventHandle)?.release ?: ""
    }

    @UsedByGodot
    fun eventSetDist(eventHandle: Int, dist: String) {
        getEvent(eventHandle)?.dist = dist
    }

    @UsedByGodot
    fun eventGetDist(eventHandle: Int): String {
        return getEvent(eventHandle)?.dist ?: ""
    }

    @UsedByGodot
    fun eventSetEnvironment(eventHandle: Int, environment: String) {
        getEvent(eventHandle)?.environment = environment
    }

    @UsedByGodot
    fun eventGetEnvironment(eventHandle: Int): String {
        return getEvent(eventHandle)?.environment ?: ""
    }

    @UsedByGodot
    fun eventSetTag(eventHandle: Int, key: String, value: String) {
        getEvent(eventHandle)?.setTag(key, value)
    }

    @UsedByGodot
    fun eventGetTag(eventHandle: Int, key: String): String {
        return getEvent(eventHandle)?.getTag(key) ?: ""
    }

    @UsedByGodot
    fun eventRemoveTag(eventHandle: Int, key: String) {
        getEvent(eventHandle)?.removeTag(key)
    }

    @UsedByGodot
    fun eventMergeContext(eventHandle: Int, key: String, value: Dictionary) {
        val event = getEvent(eventHandle) ?: return

        val existingContext: Any? = event.contexts[key]

        if (existingContext is Dictionary) {
            // Fast path: merge Dictionary directly.
            existingContext.putAll(value)
        } else {
            val existingMap = existingContext as? Map<*, *>
            existingMap?.let {
                // Merge elements from existing context into new context, but only for keys
                // that don't already exist in the new context.
                for ((k, v) in it) {
                    val kStr: String = k as? String ?: continue
                    if (!value.containsKey(kStr)) {
                        value[kStr] = v
                    }
                }
            }
            event.contexts[key] = value
        }

    }

    @UsedByGodot
    fun eventIsCrash(eventHandle: Int): Boolean {
        return getEvent(eventHandle)?.isCrashed == true
    }

    @UsedByGodot
    fun eventToJson(eventHandle: Int): String {
        val event = getEvent(eventHandle) ?: return ""
        val serializer: ISerializer = Sentry.getCurrentScopes().options.serializer
        val writer = StringWriter()
        serializer.serialize(event, writer)
        return writer.toString()
    }

    @UsedByGodot
    fun createException(type: String, value: String): Int {
        val exception = SentryException()
        exception.type = type
        exception.value = value
        exception.stacktrace = SentryStackTrace()
        exception.stacktrace?.frames = mutableListOf()
        return registerException(exception)
    }

    @UsedByGodot
    fun releaseException(exceptionHandle: Int) {
        val exceptionsMap = exceptionsByHandle.get()
        if (exceptionsMap == null) {
            Log.e(TAG, "Internal Error -- exceptionsByHandle is null")
            return
        }

        exceptionsMap.remove(exceptionHandle)
    }

    @UsedByGodot
    fun exceptionAppendStackFrame(exceptionHandle: Int, frameData: Dictionary) {
        val exception = getException(exceptionHandle) ?: return
        val frame = SentryStackFrame().apply {
            filename = frameData["filename"] as? String
            function = frameData["function"] as? String
            lineno = frameData["lineno"] as? Int
            isInApp = frameData["in_app"] as? Boolean
            platform = frameData["platform"] as? String

            if (frameData.containsKey("context_line")) {
                contextLine = frameData["context_line"] as? String
                preContext = (frameData["pre_context"] as? Array<*>)?.map { it as String }
                postContext = (frameData["post_context"] as? Array<*>)?.map { it as String }
            }

            if (frameData.containsKey("vars")) {
                vars = frameData["vars"] as? Dictionary
            }
        }

        exception.stacktrace?.frames?.add(frame)
    }

    @UsedByGodot
    fun eventAddException(eventHandle: Int, exceptionHandle: Int) {
        val event = getEvent(eventHandle) ?: return
        val exception = getException(exceptionHandle) ?: return
        if (event.exceptions == null) {
            event.exceptions = mutableListOf()
        }
        event.exceptions?.add(exception)
    }

    @UsedByGodot
    fun eventGetExceptionCount(eventHandle: Int): Int {
        return getEvent(eventHandle)?.exceptions?.size ?: 0
    }

    @UsedByGodot
    fun eventSetExceptionValue(eventHandle: Int, index: Int, value: String) {
        getEvent(eventHandle)?.exceptions?.getOrNull(index)?.value = value
    }

    @UsedByGodot
    fun eventGetExceptionValue(eventHandle: Int, index: Int): String {
        return getEvent(eventHandle)?.exceptions?.getOrNull(index)?.value ?: ""
    }

    @UsedByGodot
    fun createBreadcrumb(): Int {
        val crumb = Breadcrumb()
        val handle = registerBreadcrumb(crumb)
        return handle
    }

    @UsedByGodot
    fun releaseBreadcrumb(handle: Int) {
        val breadcrumbsMap = breadcrumbsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- breadcrumbsByHandle is null")
            return
        }

        breadcrumbsMap.remove(handle)
    }

    @UsedByGodot
    fun breadcrumbSetMessage(handle: Int, message: String) {
        getBreadcrumb(handle)?.message = message
    }

    @UsedByGodot
    fun breadcrumbGetMessage(handle: Int): String {
        return getBreadcrumb(handle)?.message ?: ""
    }

    @UsedByGodot
    fun breadcrumbSetCategory(handle: Int, category: String) {
        getBreadcrumb(handle)?.category = category
    }

    @UsedByGodot
    fun breadcrumbGetCategory(handle: Int): String {
        return getBreadcrumb(handle)?.category ?: ""
    }

    @UsedByGodot
    fun breadcrumbSetLevel(handle: Int, level: Int) {
        getBreadcrumb(handle)?.level = level.toSentryLevel()
    }

    @UsedByGodot
    fun breadcrumbGetLevel(handle: Int): Int {
        return getBreadcrumb(handle)?.level?.toInt() ?: SentryLevel.INFO.toInt()
    }

    @UsedByGodot
    fun breadcrumbSetType(handle: Int, type: String) {
       getBreadcrumb(handle)?.type = type
    }

    @UsedByGodot
    fun breadcrumbGetType(handle: Int): String {
        return getBreadcrumb(handle)?.type ?: ""
    }

    @UsedByGodot
    fun breadcrumbSetData(handle: Int, data: Dictionary) {
        val crumb = getBreadcrumb(handle) ?: return

        crumb.data.clear()

        for ((k, v) in data) {
            crumb.data[k] = v
        }
    }

    @UsedByGodot
    fun breadcrumbGetTimestamp(handle: Int): Long {
        val crumb = getBreadcrumb(handle) ?: return 0
        return crumb.timestamp.toMicros()
    }

    @UsedByGodot
    fun releaseLog(handle: Int) {
        val logsMap = logsByHandle.get() ?: run {
            Log.e(TAG, "Internal Error -- logsByHandle is null")
            return
        }

        logsMap.remove(handle)
    }

    @UsedByGodot
    fun logSetLevel(handle: Int, level: Int) {
        getLog(handle)?.level = level.toSentryLogLevel()
    }

    @UsedByGodot
    fun logGetLevel(handle: Int): Int {
        return getLog(handle)?.level?.toInt() ?: SentryLogLevel.INFO.toInt()
    }

    @UsedByGodot
    fun logSetBody(handle: Int, body: String) {
        getLog(handle)?.body = body
    }

    @UsedByGodot
    fun logGetBody(handle: Int): String {
        return getLog(handle)?.body ?: ""
    }

    @UsedByGodot
    fun logGetAttribute(handle: Int, name: String): Dictionary {
        // NOTE: Use Dictionary container for the value to avoid object wrapper creation.
        var attr = getLog(handle)?.attributes?.get(name) ?: return Dictionary()
        val result = Dictionary()
        result["type"] = attr.type
        result["value"] = attr.value
        return result
    }

    @UsedByGodot
    fun logSetAttribute(handle: Int, name: String, type: String, value: Any) {
        val log = getLog(handle) ?: return
        val logAttributes = log.attributes ?: HashMap<String, SentryLogEventAttributeValue>().also { log.attributes = it }
        logAttributes[name] = SentryLogEventAttributeValue(type, value)
    }

    @UsedByGodot
    fun logAddAttributes(handle: Int, attributes: Dictionary) {
        val log = getLog(handle) ?: return
        val logAttributes = log.attributes ?: HashMap<String, SentryLogEventAttributeValue>().also { log.attributes = it }
        for ((key, value) in attributes) {
            val attrValue = when(value) {
                is Boolean -> SentryLogEventAttributeValue("boolean", value)
                is Int, is Long -> SentryLogEventAttributeValue("integer", value)
                is Float, is Double -> SentryLogEventAttributeValue("double", value)
                is String -> SentryLogEventAttributeValue("string", value)
                else -> SentryLogEventAttributeValue("string", value.toString())
            }
            logAttributes[key.toString()] = attrValue
        }
    }

    @UsedByGodot
    fun logRemoveAttribute(handle: Int, name: String) {
        getLog(handle)?.attributes?.remove(name)
    }

}
