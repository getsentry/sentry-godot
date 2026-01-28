#include "javascript_sdk.h"

#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_event.h"
#include "sentry/javascript/javascript_util.h"
#include "sentry/logging/print.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_options.h"

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
EM_JS(uint8_t, add_bytes, (const uint8_t *data, int size), {
	try {
		var bytes = new Uint8Array(size);
		bytes.set(HEAPU8.subarray(data, data + size));

		var id = window.SentryBridge.addBytes(bytes);
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

	// Get file attachments via the getter.
	PackedStringArray file_attachments = _file_attachments_getter.call();

	Ref<sentry::javascript::JavaScriptEvent> event = memnew(sentry::javascript::JavaScriptEvent(event_obj));
	Ref<sentry::javascript::JavaScriptEvent> processed = sentry::process_event(event);

	// NOTE: We cannot return a value from a callback, so we use the same
	//       event object to communicate the result back.
	if (unlikely(processed.is_null())) {
		// Discard event.
		event_obj->set("shouldDiscard", true);
	} else {
		event_obj->set("shouldDiscard", false);

		// Add file-based attachments
		for (const String &path : file_attachments) {
			Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
			if (file.is_null()) {
				// Some attachments may be missing. Skip...
				sentry::logging::print_debug("Skipping missing file attachment: " + path);
				continue;
			}

			PackedByteArray bytes = file->get_buffer(file->get_length());
			if (bytes.is_empty()) {
				sentry::logging::print_debug("Skipping empty file attachment: " + path);
				continue;
			}

			sentry::logging::print_debug("Adding attachment: " + path);

			uint8_t bytes_id = em_js::add_bytes(bytes.ptr(), bytes.size());
			if (bytes_id == 0) {
				sentry::logging::print_warning("Failed to push attachment bytes to JS: " + path);
				continue;
			}

			// Create attachment data object and push to outAttachments array
			Ref<JavaScriptObject> attachment_data = JavaScriptBridge::get_singleton()->create_object("Object");
			attachment_data->set("id", bytes_id);
			attachment_data->set("filename", path.get_file());

			out_attachments->call("push", attachment_data);
		}
	}
}

// *** JavaScriptSDK

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("setContext", p_key, JSON::stringify(p_value), String(), false);
}

void JavaScriptSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeContext", p_key);
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("setTag", p_key, p_value);
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeTag", p_key);
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	if (p_user.is_null()) {
		js_sentry_bridge()->call("removeUser");
	} else {
		js_sentry_bridge()->call("setUser",
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	}
}

void JavaScriptSDK::remove_user() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeUser");
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
	js_sentry_bridge()->call("addBreadcrumb", crumb->get_js_object());
}

void JavaScriptSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	String attr_value = attributes_to_json(p_attributes);

	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			js_sentry_bridge()->call("logTrace", p_body, attr_value);
		} break;
		case LOG_LEVEL_DEBUG: {
			js_sentry_bridge()->call("logDebug", p_body, attr_value);
		} break;
		case LOG_LEVEL_INFO: {
			js_sentry_bridge()->call("logInfo", p_body, attr_value);
		} break;
		case LOG_LEVEL_WARN: {
			js_sentry_bridge()->call("logWarn", p_body, attr_value);
		} break;
		case LOG_LEVEL_ERROR: {
			js_sentry_bridge()->call("logError", p_body, attr_value);
		} break;
		case LOG_LEVEL_FATAL: {
			js_sentry_bridge()->call("logFatal", p_body, attr_value);
		} break;
	}
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call("captureMessage", p_message, level_as_string(p_level));
}

String JavaScriptSDK::get_last_event_id() {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call("lastEventId");
}

Ref<SentryEvent> JavaScriptSDK::create_event() {
	return memnew(JavaScriptEvent);
}

String JavaScriptSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	JavaScriptEvent *ev = Object::cast_to<JavaScriptEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(ev, String());
	return js_sentry_bridge()->call("captureEvent", ev->get_js_object());
}

void JavaScriptSDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");

	js_sentry_bridge()->call("captureFeedback",
			p_feedback->get_message(),
			p_feedback->get_name(),
			p_feedback->get_contact_email(),
			p_feedback->get_associated_event_id());
}

void JavaScriptSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");

	if (!p_attachment->get_path().is_empty()) {
		file_attachments.append(p_attachment->get_path());
	} else {
		// Bytes attachment
		PackedByteArray bytes = p_attachment->get_bytes();
		ERR_FAIL_COND_MSG(bytes.is_empty(), "Sentry: Can't add attachment with empty bytes and no file path.");
		String filename = p_attachment->get_filename();
		ERR_FAIL_COND_MSG(filename.is_empty(), "Sentry: Can't add attachment without filename.");

		em_js::sentry_add_bytes_attachment(
				filename.utf8().get_data(),
				bytes.ptr(),
				bytes.size(),
				p_attachment->get_content_type_or_default().utf8().get_data());
	}
}

void JavaScriptSDK::init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	file_attachments.clear();
	if (!p_global_attachments.is_empty()) {
		for (const String path : p_global_attachments) {
			file_attachments.append(path);
		}
	}

	_before_send_callback = JavaScriptBridge::get_singleton()->create_callback(callable_mp(_before_send_handler, &JavaScriptBeforeSendHandler::handle_before_send));

	js_sentry_bridge()->call("init",
			_before_send_callback,
			SentryOptions::get_singleton()->get_dsn(),
			SentryOptions::get_singleton()->is_debug_enabled(),
			SentryOptions::get_singleton()->get_release(),
			SentryOptions::get_singleton()->get_dist(),
			SentryOptions::get_singleton()->get_environment(),
			SentryOptions::get_singleton()->get_sample_rate(),
			SentryOptions::get_singleton()->get_max_breadcrumbs(),
			SentryOptions::get_singleton()->get_enable_logs());
}

void JavaScriptSDK::close() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	js_sentry_bridge()->call("close");

	_before_send_callback.unref();
	file_attachments.clear();
}

bool JavaScriptSDK::is_enabled() const {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), false);
	return js_sentry_bridge()->call("isEnabled");
}

JavaScriptSDK::JavaScriptSDK() {
	auto getter_func = [](void *ctx) -> PackedStringArray {
		return static_cast<JavaScriptSDK *>(ctx)->_get_file_attachments();
	};
	_before_send_handler = memnew(JavaScriptBeforeSendHandler);
	_before_send_handler->initialize(
			JavaScriptBeforeSendHandler::FileAttachmentsGetter{ getter_func, this });
}

JavaScriptSDK::~JavaScriptSDK() {
	memdelete(_before_send_handler);
	_before_send_handler = nullptr;
}

} //namespace sentry::javascript
