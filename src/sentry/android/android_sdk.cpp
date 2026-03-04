#include "android_sdk.h"

#include "android_breadcrumb.h"
#include "android_event.h"
#include "android_log.h"
#include "android_metric.h"
#include "android_string_names.h"
#include "android_util.h"
#include "sentry/common_defs.h"
#include "sentry/logging/print.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_log.h"
#include "sentry/processing/process_metric.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace {

inline Variant _as_attribute(const Variant &p_value) {
	Variant::Type type = p_value.get_type();
	return (type < Variant::BOOL || type > Variant::STRING) ? (Variant)p_value.stringify() : p_value;
}

Dictionary _sanitize_attributes(const Dictionary &p_attributes) {
	Dictionary attributes;
	if (!p_attributes.is_empty()) {
		const Array &keys = p_attributes.keys();
		for (int i = 0; i < keys.size(); i++) {
			const String &key = keys[i];
			attributes[key] = _as_attribute(p_attributes[key]);
		}
	}
	return attributes;
}

} // unnamed namespace

namespace sentry::android {

// *** SentryAndroidBeforeSendHandler

void SentryAndroidBeforeSendHandler::_initialize(Object *p_android_plugin) {
	android_plugin = p_android_plugin;
}

void SentryAndroidBeforeSendHandler::_before_send(int32_t p_event_handle) {
	sentry::logging::print_debug("handling before_send: ", p_event_handle);

	Ref<AndroidEvent> event_obj = memnew(AndroidEvent(android_plugin, p_event_handle));
	event_obj->set_as_borrowed();

	Ref<AndroidEvent> processed = sentry::process_event(event_obj);

	if (processed.is_null()) {
		// Discard event.
		android_plugin->call(ANDROID_SN(releaseEvent), p_event_handle);
	}
}

void SentryAndroidBeforeSendHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send"), &SentryAndroidBeforeSendHandler::_before_send);
}

// *** SentryAndroidBeforeSendLogHandler

void SentryAndroidBeforeSendLogHandler::_initialize(Object *p_android_plugin) {
	android_plugin = p_android_plugin;
}

void SentryAndroidBeforeSendLogHandler::_before_send_log(int32_t p_handle) {
	Ref<AndroidLog> log_obj = memnew(AndroidLog(android_plugin, p_handle));
	log_obj->set_as_borrowed();

	Ref<AndroidLog> processed = sentry::process_log(log_obj);

	if (processed.is_null()) {
		// Discard log.
		android_plugin->call(ANDROID_SN(releaseLog), p_handle);
	}
}

void SentryAndroidBeforeSendLogHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_log"), &SentryAndroidBeforeSendLogHandler::_before_send_log);
}

// ** SentryAndroidBeforeSendMetricHandler

void SentryAndroidBeforeSendMetricHandler::_before_send_metric(int32_t p_metric_handle) {
	Ref<AndroidMetric> metric_obj = memnew(AndroidMetric(android_plugin, p_metric_handle));
	metric_obj->set_as_borrowed();

	Ref<AndroidMetric> processed = sentry::process_metric(metric_obj);

	if (processed.is_null()) {
		// Discard metric.
		android_plugin->call(ANDROID_SN(releaseMetric), p_metric_handle);
	}
}

void SentryAndroidBeforeSendMetricHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_metric"), &SentryAndroidBeforeSendMetricHandler::_before_send_metric);
}

// *** AndroidSDK

void AndroidSDK::set_context(const String &p_key, const Dictionary &p_value) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(setContext), p_key, sanitize_variant(p_value));
}

void AndroidSDK::remove_context(const String &p_key) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeContext), p_key);
}

void AndroidSDK::set_tag(const String &p_key, const String &p_value) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(setTag), p_key, p_value);
}

void AndroidSDK::remove_tag(const String &p_key) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeTag), p_key);
}

void AndroidSDK::set_user(const Ref<SentryUser> &p_user) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	if (p_user.is_valid()) {
		android_plugin->call(ANDROID_SN(setUser),
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		remove_user();
	}
}

void AndroidSDK::remove_user() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeUser));
}

Ref<SentryBreadcrumb> AndroidSDK::create_breadcrumb() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	int32_t handle = android_plugin->call(ANDROID_SN(createBreadcrumb));
	Ref<AndroidBreadcrumb> crumb = memnew(AndroidBreadcrumb(android_plugin, handle));
	return crumb;
}

void AndroidSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	Ref<AndroidBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	android_plugin->call(ANDROID_SN(addBreadcrumb), crumb->get_handle());
}

void AndroidSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	if (p_body.is_empty()) {
		return;
	}

	android_plugin->call(ANDROID_SN(log), p_level, p_body, _sanitize_attributes(p_attributes));
}

String AndroidSDK::capture_message(const String &p_message, Level p_level) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(captureMessage), p_message, p_level);
}

String AndroidSDK::get_last_event_id() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(getLastEventId));
}

Ref<SentryEvent> AndroidSDK::create_event() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	int32_t event_handle = android_plugin->call(ANDROID_SN(createEvent));
	Ref<AndroidEvent> event = memnew(AndroidEvent(android_plugin, event_handle));
	return event;
}

String AndroidSDK::capture_event(const Ref<SentryEvent> &p_event) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL_V(android_plugin, String());
	ERR_FAIL_COND_V(p_event.is_null(), String());
	Ref<AndroidEvent> android_event = p_event;
	ERR_FAIL_COND_V(android_event.is_null(), String());
	int32_t handle = android_event->get_handle();
	android_plugin->call(ANDROID_SN(captureEvent), handle);
	return android_event->get_id();
}

void AndroidSDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");
	android_plugin->call(ANDROID_SN(captureFeedback),
			p_feedback->get_message(),
			p_feedback->get_contact_email(),
			p_feedback->get_name(),
			p_feedback->get_associated_event_id());
}

void AndroidSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);
	ERR_FAIL_COND(p_attachment.is_null());

	if (p_attachment->get_path().is_empty()) {
		sentry::logging::print_debug("attaching bytes with filename: ", p_attachment->get_filename());
		android_plugin->call(ANDROID_SN(addBytesAttachment),
				p_attachment->get_bytes(),
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				String());
	} else {
		String absolute_path = p_attachment->get_globalized_path();
		sentry::logging::print_debug("attaching file: ", absolute_path);
		android_plugin->call(ANDROID_SN(addFileAttachment),
				absolute_path,
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				String());
	}
}

void AndroidSDK::metrics_add_count(const String &p_name, int64_t p_value, const Dictionary &p_attributes) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	android_plugin->call(ANDROID_SN(metricsAddCount),
			p_name, p_value, _sanitize_attributes(p_attributes));
}

void AndroidSDK::metrics_add_gauge(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	android_plugin->call(ANDROID_SN(metricsAddGauge),
			p_name, p_value, p_unit, _sanitize_attributes(p_attributes));
}

void AndroidSDK::metrics_add_distribution(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	android_plugin->call(ANDROID_SN(metricsAddDistribution),
			p_name, p_value, p_unit, _sanitize_attributes(p_attributes));
}

void AndroidSDK::init() {
void AndroidSDK::_add_default_attachments() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	for (const Ref<SentryAttachment> &att : SENTRY_OPTIONS()->get_file_attachments()) {
		android_plugin->call(ANDROID_SN(addFileAttachment),
				att->get_globalized_path(),
				String(), // filename
				att->get_content_type(),
				att->get_attachment_type());
	}
}

void AndroidSDK::clear_attachments() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	android_plugin->call(ANDROID_SN(clearAttachments));
	_add_default_attachments();
}

void AndroidSDK::init() {
	Object *android_plugin = _get_android_plugin();
	ERR_FAIL_NULL(android_plugin);

	_add_default_attachments();

	Dictionary optionsData;
	optionsData["dsn"] = SENTRY_OPTIONS()->get_dsn();
	optionsData["debug"] = SENTRY_OPTIONS()->is_debug_enabled();
	optionsData["release"] = SENTRY_OPTIONS()->get_release();
	optionsData["dist"] = SENTRY_OPTIONS()->get_dist();
	optionsData["environment"] = SENTRY_OPTIONS()->get_environment();
	optionsData["sample_rate"] = SENTRY_OPTIONS()->get_sample_rate();
	optionsData["max_breadcrumbs"] = SENTRY_OPTIONS()->get_max_breadcrumbs();
	optionsData["enable_logs"] = SENTRY_OPTIONS()->get_enable_logs();
	optionsData["enable_metrics"] = SENTRY_OPTIONS()->get_experimental()->get_enable_metrics();
	optionsData["app_hang_tracking"] = SENTRY_OPTIONS()->is_app_hang_tracking_enabled();
	optionsData["app_hang_timeout_sec"] = SENTRY_OPTIONS()->get_app_hang_timeout_sec();
	optionsData["shutdown_timeout_ms"] = SENTRY_OPTIONS()->get_shutdown_timeout_ms();

	android_plugin->call(ANDROID_SN(init),
			optionsData,
			before_send_handler->get_instance_id(),
			SENTRY_OPTIONS()->get_before_send_log().is_valid() ? before_send_log_handler->get_instance_id() : 0,
			SENTRY_OPTIONS()->get_experimental()->get_before_send_metric().is_valid() ? before_send_metric_handler->get_instance_id() : 0);

	if (is_enabled()) {
		set_user(SentryUser::create_default());
	} else {
		ERR_PRINT("Sentry: Failed to initialize Android SDK.");
	}
}

void AndroidSDK::close() {
	Object *android_plugin = _get_android_plugin();
	if (android_plugin != nullptr) {
		android_plugin->call(ANDROID_SN(close));
	}
}

bool AndroidSDK::is_enabled() const {
	Object *android_plugin = _get_android_plugin();
	return android_plugin && android_plugin->call(ANDROID_SN(isEnabled));
}

AndroidSDK::AndroidSDK() {
	AndroidStringNames::create_singleton();

	Object *android_plugin = Engine::get_singleton()->get_singleton("SentryAndroidGodotPlugin");
	ERR_FAIL_NULL_MSG(android_plugin, "Sentry: Unable to locate SentryAndroidGodotPlugin singleton.");
	android_plugin_instance_id = android_plugin->get_instance_id();

	before_send_handler = memnew(SentryAndroidBeforeSendHandler);
	before_send_handler->_initialize(android_plugin);

	before_send_log_handler = memnew(SentryAndroidBeforeSendLogHandler);
	before_send_log_handler->_initialize(android_plugin);

	before_send_metric_handler = memnew(SentryAndroidBeforeSendMetricHandler);
	before_send_metric_handler->_initialize(android_plugin);
}

AndroidSDK::~AndroidSDK() {
	AndroidStringNames::destroy_singleton();
	if (before_send_handler) {
		memdelete(before_send_handler);
	}
	if (before_send_log_handler) {
		memdelete(before_send_log_handler);
	}
	if (before_send_metric_handler) {
		memdelete(before_send_metric_handler);
	}
}

} //namespace sentry::android
