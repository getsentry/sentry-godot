#include "javascript_sdk.h"

#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_event.h"
#include "sentry/javascript/javascript_log.h"
#include "sentry/javascript/javascript_string_names.h"
#include "sentry/javascript/javascript_util.h"
#include "sentry/logging/print.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_log.h"
#include "sentry/sentry_sdk.h"

#include "gen/sdk_version.gen.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/json.hpp>

#include <emscripten.h>

// Defines JS functions inline inside C++ unit
namespace em_js {

// Emscripten JS function to add bytes attachment directly from WASM memory.
// This avoids the overhead of base64 encoding/decoding.
EM_JS(void, sentry_add_bytes_attachment, (const char *filename, const uint8_t *data, int size, const char *content_type), {
	try {
		var filenameStr = UTF8ToString(filename);
		var contentTypeStr = UTF8ToString(content_type);

		// Copy bytes from WASM memory into a new Uint8Array
		var bytes = new Uint8Array(size);
		bytes.set(HEAPU8.subarray(data, data + size));

		window.SentryBridge.addBytesAttachment(filenameStr, bytes, contentTypeStr);
	} catch (e) {
		console.error("Failed to add bytes attachment:", e);
	}
});

// Emscripten JS function to push bytes directly to bridge layer and return id for later retrieval.
EM_JS(uint32_t, store_bytes, (const uint8_t *data, int size), {
	try {
		var bytes = new Uint8Array(size);
		bytes.set(HEAPU8.subarray(data, data + size));

		var id = window.SentryBridge.storeBytes(bytes);
		return id;
	} catch (e) {
		console.error("Failed to transfer bytes to JS bridge layer:", e);
		return 0;
	}
});

} //namespace em_js

namespace sentry::javascript {

// *** JavaScriptBeforeSendHandler

void JavaScriptBeforeSendHandler::initialize(const FileAttachmentsGetter &p_file_attachments_getter) {
	_file_attachments_getter = p_file_attachments_getter;
}

void JavaScriptBeforeSendHandler::handle_before_send(const Array &p_args) {
	ERR_FAIL_COND(p_args.size() != 2);

	Ref<JavaScriptObject> event_obj = p_args[0];
	ERR_FAIL_COND(event_obj.is_null());

	Ref<JavaScriptObject> out_attachments = p_args[1];
	ERR_FAIL_COND(out_attachments.is_null());

	Ref<JavaScriptEvent> event = memnew(JavaScriptEvent(event_obj));
	Ref<JavaScriptEvent> processed = sentry::process_event(event);

	// NOTE: We cannot return a value from a callback, so we use the same
	//       event object to communicate the result back.
	if (unlikely(processed.is_null())) {
		// Discard event.
		event_obj->set(JAVASCRIPT_SN(shouldDiscard), true);
	} else {
		event_obj->set(JAVASCRIPT_SN(shouldDiscard), false);

		// Read file-based attachments and include them with the event.
		Vector<Ref<SentryAttachment>> file_attachments = _file_attachments_getter.call();
		for (const Ref<SentryAttachment> &att : file_attachments) {
			if (att->get_path().is_empty()) {
				// Skip attachments with empty path.
				// NOTE: Byte attachments are not processed here - they are added immediately.
				continue;
			}

			Ref<FileAccess> file = FileAccess::open(att->get_path(), FileAccess::READ);
			if (file.is_null()) {
				// NOTE: Some attachments may legitimately be missing (e.g. screenshots not created on non-main threads).
				sentry::logging::print_debug("Skipping attachment - file not found: " + att->get_path());
				continue;
			}

			PackedByteArray bytes = file->get_buffer(file->get_length());
			if (bytes.is_empty()) {
				sentry::logging::print_debug("Skipping attachment - empty file: " + att->get_path());
				continue;
			}

			sentry::logging::print_debug("Adding attachment: " + att->get_path());

			uint32_t bytes_id = em_js::store_bytes(bytes.ptr(), bytes.size());
			if (bytes_id == 0) {
				sentry::logging::print_warning("Failed to push attachment bytes to JS: " + att->get_path());
				continue;
			}

			Ref<JavaScriptObject> attachment_data = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));
			attachment_data->set(JAVASCRIPT_SN(id), bytes_id);
			attachment_data->set(JAVASCRIPT_SN(filename), att->get_path().get_file());
			if (!att->get_attachment_type().is_empty()) {
				attachment_data->set(JAVASCRIPT_SN(attachmentType), att->get_attachment_type());
			}
			if (!att->get_content_type().is_empty()) {
				attachment_data->set(JAVASCRIPT_SN(contentType), att->get_content_type());
			}
			out_attachments->call(JAVASCRIPT_SN(push), attachment_data);
		}
	}
}

// *** JavaScriptBeforeSendLogHandler

void JavaScriptBeforeSendLogHandler::handle_before_send_log(const Array &p_args) {
	ERR_FAIL_COND(p_args.size() != 1);

	Ref<JavaScriptObject> log_obj = p_args[0];
	ERR_FAIL_COND(log_obj.is_null());

	Ref<JavaScriptLog> log = memnew(JavaScriptLog(log_obj));
	Ref<JavaScriptLog> processed = sentry::process_log(log);

	// NOTE: We cannot return a value from a callback, so we use the same
	//       log object to communicate the result back.
	log_obj->set(JAVASCRIPT_SN(shouldDiscard), processed.is_null());
}

// *** JavaScriptSDK

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call(JAVASCRIPT_SN(setContext), p_key, JSON::stringify(p_value));
}

void JavaScriptSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call(JAVASCRIPT_SN(removeContext), p_key);
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call(JAVASCRIPT_SN(setTag), p_key, p_value);
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call(JAVASCRIPT_SN(removeTag), p_key);
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	if (p_user.is_null()) {
		js_sentry_bridge()->call(JAVASCRIPT_SN(removeUser));
	} else {
		js_sentry_bridge()->call(JAVASCRIPT_SN(setUser),
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	}
}

void JavaScriptSDK::remove_user() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call(JAVASCRIPT_SN(removeUser));
}

Ref<SentryBreadcrumb> JavaScriptSDK::create_breadcrumb() {
	return memnew(JavaScriptBreadcrumb);
}

void JavaScriptSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	if (p_breadcrumb.is_null()) {
		return;
	}
	JavaScriptBreadcrumb *crumb = Object::cast_to<JavaScriptBreadcrumb>(p_breadcrumb.ptr());
	ERR_FAIL_NULL(crumb);
	js_sentry_bridge()->call(JAVASCRIPT_SN(addBreadcrumb), crumb->get_js_object());
}

void JavaScriptSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	String attr_value = attributes_to_json(p_attributes);

	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logTrace), p_body, attr_value);
		} break;
		case LOG_LEVEL_DEBUG: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logDebug), p_body, attr_value);
		} break;
		case LOG_LEVEL_INFO: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logInfo), p_body, attr_value);
		} break;
		case LOG_LEVEL_WARN: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logWarn), p_body, attr_value);
		} break;
		case LOG_LEVEL_ERROR: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logError), p_body, attr_value);
		} break;
		case LOG_LEVEL_FATAL: {
			js_sentry_bridge()->call(JAVASCRIPT_SN(logFatal), p_body, attr_value);
		} break;
	}
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call(JAVASCRIPT_SN(captureMessage), p_message, level_as_string(p_level));
}

String JavaScriptSDK::get_last_event_id() {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call(JAVASCRIPT_SN(lastEventId));
}

Ref<SentryEvent> JavaScriptSDK::create_event() {
	return memnew(JavaScriptEvent);
}

String JavaScriptSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	ERR_FAIL_COND_V_MSG(p_event.is_null(), String(), "Sentry: Can't capture event - event object is null.");
	JavaScriptEvent *ev = Object::cast_to<JavaScriptEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(ev, String());
	return js_sentry_bridge()->call(JAVASCRIPT_SN(captureEvent), ev->get_js_object());
}

void JavaScriptSDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");

	js_sentry_bridge()->call(JAVASCRIPT_SN(captureFeedback),
			p_feedback->get_message(),
			p_feedback->get_name(),
			p_feedback->get_contact_email(),
			p_feedback->get_associated_event_id());
}

void JavaScriptSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");

	if (!p_attachment->get_path().is_empty()) {
		// Add attachment to list for on-demand loading during event processing.
		file_attachments.append(p_attachment);
	} else {
		// Bytes attachment - added immediately
		ERR_FAIL_COND_MSG(p_attachment->get_bytes().is_empty(), "Sentry: Can't add attachment with empty bytes and no file path.");
		ERR_FAIL_COND_MSG(p_attachment->get_filename().is_empty(), "Sentry: Can't add attachment without filename.");

		em_js::sentry_add_bytes_attachment(
				p_attachment->get_filename().utf8(),
				p_attachment->get_bytes().ptr(),
				p_attachment->get_bytes().size(),
				p_attachment->get_content_type_or_default().utf8());
	}
}

void JavaScriptSDK::init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	if (p_configuration_callback.is_valid()) {
		p_configuration_callback.call(SENTRY_OPTIONS());
	}

	file_attachments.clear();
	if (!p_global_attachments.is_empty()) {
		for (const String &path : p_global_attachments) {
			Ref<SentryAttachment> att = SentryAttachment::create_with_path(path);
			if (path.ends_with("view-hierarchy.json")) {
				att->set_content_type("application/json");
				att->set_attachment_type("event.view_hierarchy");
			}
			file_attachments.append(att);
		}
	}

	_before_send_js_callback = JavaScriptBridge::get_singleton()->create_callback(callable_mp(_before_send_handler, &JavaScriptBeforeSendHandler::handle_before_send));

	// Only create the before_send_log callback if user has set a callback
	Variant before_send_log_callback;
	if (SENTRY_OPTIONS()->get_before_send_log().is_valid()) {
		_before_send_log_js_callback = JavaScriptBridge::get_singleton()->create_callback(callable_mp(_before_send_log_handler, &JavaScriptBeforeSendLogHandler::handle_before_send_log));
		before_send_log_callback = _before_send_log_js_callback;
	}

	js_sentry_bridge()->call(JAVASCRIPT_SN(init),
			_before_send_js_callback,
			before_send_log_callback,
			SENTRY_OPTIONS()->get_dsn(),
			SENTRY_OPTIONS()->is_debug_enabled(),
			SENTRY_OPTIONS()->get_release(),
			SENTRY_OPTIONS()->get_dist(),
			SENTRY_OPTIONS()->get_environment(),
			SENTRY_OPTIONS()->get_sample_rate(),
			SENTRY_OPTIONS()->get_max_breadcrumbs(),
			SENTRY_OPTIONS()->get_enable_logs(),
			String(SENTRY_GODOT_SDK_VERSION));

	if (is_enabled()) {
		set_user(SentryUser::create_default());
	} else {
		ERR_PRINT("Sentry: Failed to initialize JavaScript SDK.");
	}
}

void JavaScriptSDK::close() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	js_sentry_bridge()->call(JAVASCRIPT_SN(close));

	_before_send_js_callback.unref();
	_before_send_log_js_callback.unref();
	file_attachments.clear();
}

bool JavaScriptSDK::is_enabled() const {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), false);
	return js_sentry_bridge()->call(JAVASCRIPT_SN(isEnabled));
}

JavaScriptSDK::JavaScriptSDK() {
	JavaScriptStringNames::create_singleton();

	auto getter_func = [](void *ctx) -> Vector<Ref<SentryAttachment>> {
		return static_cast<JavaScriptSDK *>(ctx)->_get_file_attachments();
	};
	_before_send_handler = memnew(JavaScriptBeforeSendHandler);
	_before_send_handler->initialize(
			JavaScriptBeforeSendHandler::FileAttachmentsGetter{ getter_func, this });

	_before_send_log_handler = memnew(JavaScriptBeforeSendLogHandler);
}

JavaScriptSDK::~JavaScriptSDK() {
	JavaScriptStringNames::destroy_singleton();

	memdelete(_before_send_handler);
	_before_send_handler = nullptr;

	memdelete(_before_send_log_handler);
	_before_send_log_handler = nullptr;
}

} //namespace sentry::javascript
