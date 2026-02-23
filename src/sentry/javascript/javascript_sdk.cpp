#include "javascript_sdk.h"

#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_event.h"
#include "sentry/javascript/javascript_log.h"
#include "sentry/javascript/javascript_util.h"
#include "sentry/logging/print.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_log.h"
#include "sentry/sentry_sdk.h"

#include "gen/sdk_version.gen.h"

#include <godot_cpp/classes/file_access.hpp>
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

// *** WASM callbacks

extern "C" {

static void before_send_wasm_callback(int *p_ids, int p_len) {
	ERR_FAIL_COND(p_len != 2);

	JSObjectPtr event_obj = JSObject::from_id(p_ids[0]);
	JSObjectPtr out_attachments = JSObject::from_id(p_ids[1]);
	ERR_FAIL_COND(!event_obj);
	ERR_FAIL_COND(!out_attachments);

	Ref<JavaScriptEvent> event = memnew(JavaScriptEvent(event_obj));
	Ref<JavaScriptEvent> processed = sentry::process_event(event);

	// NOTE: We cannot return a value from a callback, so we use the same
	//       event object to communicate the result back.
	if (unlikely(processed.is_null())) {
		// Discard event.
		event_obj->set("shouldDiscard", true);
	} else {
		event_obj->set("shouldDiscard", false);

		// Read file-based attachments and include them with the event.
		for (const Ref<SentryAttachment> &att : SENTRY_OPTIONS()->get_file_attachments()) {
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

			JSObjectPtr attachment_data = JSObject::create("Object");
			attachment_data->set("id", bytes_id);
			attachment_data->set("filename", att->get_path().get_file().utf8());
			if (!att->get_attachment_type().is_empty()) {
				attachment_data->set("attachmentType", att->get_attachment_type().ascii());
			}
			if (!att->get_content_type().is_empty()) {
				attachment_data->set("contentType", att->get_content_type().ascii());
			}
			out_attachments->call("push", attachment_data);
		}
	}
}

static void before_send_log_wasm_callback(int *p_ids, int p_len) {
	ERR_FAIL_COND(p_len != 1);

	JSObjectPtr log_jso = JSObject::from_id(p_ids[0]);
	ERR_FAIL_COND(!log_jso);

	Ref<JavaScriptLog> log = memnew(JavaScriptLog(log_jso));
	Ref<JavaScriptLog> processed = sentry::process_log(log);

	// NOTE: We cannot return a value from a callback, so we use the same
	//       log object to communicate the result back.
	log_jso->set("shouldDiscard", processed.is_null());
}

} // extern "C"

// *** JavaScriptSDK

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(!js_bridge());
	js_bridge()->call("setContext", p_key.utf8(), JSON::stringify(p_value).utf8());
}

void JavaScriptSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(!js_bridge());
	js_bridge()->call("removeContext", p_key.utf8());
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(!js_bridge());
	js_bridge()->call("setTag", p_key.utf8(), p_value.utf8());
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(!js_bridge());
	js_bridge()->call("removeTag", p_key.utf8());
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(!js_bridge());
	if (p_user.is_null()) {
		js_bridge()->call("removeUser");
	} else {
		js_bridge()->call("setUser",
				p_user->get_id().utf8(),
				p_user->get_username().utf8(),
				p_user->get_email().utf8(),
				p_user->get_ip_address().ascii());
	}
}

void JavaScriptSDK::remove_user() {
	ERR_FAIL_COND(!js_bridge());
	js_bridge()->call("removeUser");
}

Ref<SentryBreadcrumb> JavaScriptSDK::create_breadcrumb() {
	return memnew(JavaScriptBreadcrumb);
}

void JavaScriptSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND(!js_bridge());
	if (p_breadcrumb.is_null()) {
		return;
	}
	JavaScriptBreadcrumb *crumb = Object::cast_to<JavaScriptBreadcrumb>(p_breadcrumb.ptr());
	ERR_FAIL_NULL(crumb);
	js_bridge()->call("addBreadcrumb", crumb->get_js_object());
}

void JavaScriptSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ERR_FAIL_COND(!js_bridge());

	String attr_value = attributes_to_json(p_attributes);

	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			js_bridge()->call("logTrace", p_body.utf8(), attr_value.utf8());
		} break;
		case LOG_LEVEL_DEBUG: {
			js_bridge()->call("logDebug", p_body.utf8(), attr_value.utf8());
		} break;
		case LOG_LEVEL_INFO: {
			js_bridge()->call("logInfo", p_body.utf8(), attr_value.utf8());
		} break;
		case LOG_LEVEL_WARN: {
			js_bridge()->call("logWarn", p_body.utf8(), attr_value.utf8());
		} break;
		case LOG_LEVEL_ERROR: {
			js_bridge()->call("logError", p_body.utf8(), attr_value.utf8());
		} break;
		case LOG_LEVEL_FATAL: {
			js_bridge()->call("logFatal", p_body.utf8(), attr_value.utf8());
		} break;
	}
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_COND_V(!js_bridge(), String());
	String uuid = js_bridge()->call("captureMessage", p_message.utf8(), level_as_string(p_level).utf8()).as_string();
	return uuid;
}

String JavaScriptSDK::get_last_event_id() {
	ERR_FAIL_COND_V(!js_bridge(), String());
	return js_bridge()->call("lastEventId").as_string();
}

Ref<SentryEvent> JavaScriptSDK::create_event() {
	return memnew(JavaScriptEvent);
}

String JavaScriptSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V(!js_bridge(), String());
	ERR_FAIL_COND_V_MSG(p_event.is_null(), String(), "Sentry: Can't capture event - event object is null.");
	JavaScriptEvent *ev = Object::cast_to<JavaScriptEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(ev, String());
	return js_bridge()->call("captureEvent", ev->get_js_object()).as_string();
}

void JavaScriptSDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	ERR_FAIL_COND(!js_bridge());
	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");

	js_bridge()->call("captureFeedback",
			p_feedback->get_message().utf8(),
			p_feedback->get_name().utf8(),
			p_feedback->get_contact_email().utf8(),
			p_feedback->get_associated_event_id().ascii());
}

void JavaScriptSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND(!js_bridge());
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");

	if (!p_attachment->get_path().is_empty()) {
		// Add attachment to list for on-demand loading during event processing.
		SENTRY_OPTIONS()->add_file_attachment(p_attachment);
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

void JavaScriptSDK::init() {
	ERR_FAIL_COND(!js_bridge());

	_before_send_js_callback = JSObject::create_callback(before_send_wasm_callback);

	// Only create the before_send_log callback if user has set a callback
	JSObjectPtr before_send_log_callback;
	if (SENTRY_OPTIONS()->get_before_send_log().is_valid()) {
		_before_send_log_js_callback = JSObject::create_callback(before_send_log_wasm_callback);
		before_send_log_callback = _before_send_log_js_callback;
	}

	js_bridge()->call("init",
			_before_send_js_callback,
			before_send_log_callback,
			SENTRY_OPTIONS()->get_dsn().utf8(),
			SENTRY_OPTIONS()->is_debug_enabled(),
			SENTRY_OPTIONS()->get_release().utf8(),
			SENTRY_OPTIONS()->get_dist().utf8(),
			SENTRY_OPTIONS()->get_environment().utf8(),
			SENTRY_OPTIONS()->get_sample_rate(),
			SENTRY_OPTIONS()->get_max_breadcrumbs(),
			SENTRY_OPTIONS()->get_enable_logs(),
			SENTRY_GODOT_SDK_VERSION);

	if (is_enabled()) {
		set_user(SentryUser::create_default());
	} else {
		ERR_PRINT("Sentry: Failed to initialize JavaScript SDK.");
	}
}

void JavaScriptSDK::close() {
	ERR_FAIL_COND(!js_bridge());

	js_bridge()->call("close");

	_before_send_js_callback.reset();
	_before_send_log_js_callback.reset();
}

bool JavaScriptSDK::is_enabled() const {
	ERR_FAIL_COND_V(!js_bridge(), false);
	return js_bridge()->call("isEnabled").as_bool();
}

JavaScriptSDK::JavaScriptSDK() {
}

JavaScriptSDK::~JavaScriptSDK() {
}

} //namespace sentry::javascript
