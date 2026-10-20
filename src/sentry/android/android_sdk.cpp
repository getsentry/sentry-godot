#include "android_sdk.h"

#include "android_breadcrumb.h"
#include "android_event.h"
#include "android_log.h"
#include "android_metric.h"
#include "android_scope.h"
#include "android_span.h"
#include "android_string_names.h"
#include "android_util.h"
#include "sentry/common_defs.h"
#include "sentry/logging/print.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_feedback.h"
#include "sentry/processing/process_log.h"
#include "sentry/processing/process_metric.h"
#include "sentry/processing/process_transaction.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
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
			const Variant &key = keys[i];
			String name = key.stringify();
			ERR_CONTINUE_MSG(name.is_empty(), "Sentry: Can't set attribute with an empty name.");
			attributes[name] = _as_attribute(p_attributes[key]);
		}
	}
	return attributes;
}

PackedStringArray _trace_propagation_targets_to_strings(const Array &p_targets) {
	PackedStringArray targets;
	for (const Variant &target : p_targets) {
		if (target.get_type() == Variant::STRING) {
			targets.append(target);
			continue;
		}

		ERR_CONTINUE_MSG(target.get_type() != Variant::OBJECT, "Sentry: Ignoring an invalid trace propagation target.");
		Object *object = target;
		RegEx *regex = Object::cast_to<RegEx>(object);
		ERR_CONTINUE_MSG(regex == nullptr || !regex->is_valid(), "Sentry: Ignoring an invalid trace propagation target.");
		// Android takes each pattern as a string and matches it as both a substring and a regex.
		targets.append(regex->get_pattern());
	}
	return targets;
}

} // unnamed namespace

namespace sentry::android {

// *** SentryAndroidBeforeSendHandler

void SentryAndroidBeforeSendHandler::_initialize(util::ObjectHandle<Object> p_android_plugin) {
	_android_plugin = p_android_plugin;
}

void SentryAndroidBeforeSendHandler::_before_send(int32_t p_event_handle) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	sentry::logging::print_debug("handling before_send: ", p_event_handle);

	Ref<AndroidEvent> event_obj = memnew(AndroidEvent(_android_plugin, p_event_handle));
	event_obj->set_as_borrowed();

	Ref<AndroidEvent> processed = sentry::process_event(event_obj);

	if (processed.is_null()) {
		ENSURE_ANDROID_PLUGIN(release_plugin, _android_plugin);
		// Discard event.
		release_plugin->call(ANDROID_SN(releaseEvent), p_event_handle);
	}
}

void SentryAndroidBeforeSendHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send"), &SentryAndroidBeforeSendHandler::_before_send);
}

// *** SentryAndroidBeforeSendTransactionHandler

void SentryAndroidBeforeSendTransactionHandler::_before_send_transaction(int32_t p_event_handle) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	Ref<AndroidEvent> transaction_obj = memnew(AndroidEvent(_android_plugin, p_event_handle));
	transaction_obj->set_as_borrowed();

	Ref<AndroidEvent> processed = sentry::process_transaction(transaction_obj);

	if (processed.is_null()) {
		ENSURE_ANDROID_PLUGIN(release_plugin, _android_plugin);
		release_plugin->call(ANDROID_SN(releaseEvent), p_event_handle);
	}
}

void SentryAndroidBeforeSendTransactionHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_transaction"), &SentryAndroidBeforeSendTransactionHandler::_before_send_transaction);
}

// *** SentryAndroidBeforeSendFeedbackHandler

void SentryAndroidBeforeSendFeedbackHandler::_before_send_feedback(int32_t p_event_handle) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	Ref<AndroidEvent> event_obj = memnew(AndroidEvent(_android_plugin, p_event_handle));
	event_obj->set_as_borrowed();

	Ref<AndroidEvent> processed = sentry::process_feedback(event_obj);

	if (processed.is_null()) {
		ENSURE_ANDROID_PLUGIN(release_plugin, _android_plugin);
		// Discard feedback.
		release_plugin->call(ANDROID_SN(releaseEvent), p_event_handle);
	}
}

void SentryAndroidBeforeSendFeedbackHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_feedback"), &SentryAndroidBeforeSendFeedbackHandler::_before_send_feedback);
}

// *** SentryAndroidBeforeSendLogHandler

void SentryAndroidBeforeSendLogHandler::_initialize(util::ObjectHandle<Object> p_android_plugin) {
	_android_plugin = p_android_plugin;
}

void SentryAndroidBeforeSendLogHandler::_before_send_log(int32_t p_handle) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	Ref<AndroidLog> log_obj = memnew(AndroidLog(_android_plugin, p_handle));
	log_obj->set_as_borrowed();

	Ref<AndroidLog> processed = sentry::process_log(log_obj);

	if (processed.is_null()) {
		ENSURE_ANDROID_PLUGIN(release_plugin, _android_plugin);
		// Discard log.
		release_plugin->call(ANDROID_SN(releaseLog), p_handle);
	}
}

void SentryAndroidBeforeSendLogHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_log"), &SentryAndroidBeforeSendLogHandler::_before_send_log);
}

// ** SentryAndroidBeforeSendMetricHandler

void SentryAndroidBeforeSendMetricHandler::_before_send_metric(int32_t p_metric_handle) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	Ref<AndroidMetric> metric_obj = memnew(AndroidMetric(_android_plugin, p_metric_handle));
	metric_obj->set_as_borrowed();

	Ref<AndroidMetric> processed = sentry::process_metric(metric_obj);

	if (processed.is_null()) {
		ENSURE_ANDROID_PLUGIN(release_plugin, _android_plugin);
		// Discard metric.
		release_plugin->call(ANDROID_SN(releaseMetric), p_metric_handle);
	}
}

void SentryAndroidBeforeSendMetricHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send_metric"), &SentryAndroidBeforeSendMetricHandler::_before_send_metric);
}

// *** AndroidSDK

void AndroidSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(setContext), p_key, sanitize_variant(p_value));
}

void AndroidSDK::remove_context(const String &p_key) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(removeContext), p_key);
}

void AndroidSDK::set_tag(const String &p_key, const String &p_value) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(setTag), p_key, p_value);
}

void AndroidSDK::remove_tag(const String &p_key) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(removeTag), p_key);
}

void AndroidSDK::set_user(const Ref<SentryUser> &p_user) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	if (p_user.is_valid()) {
		plugin->call(ANDROID_SN(setUser),
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		remove_user();
	}
}

void AndroidSDK::remove_user() {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(removeUser));
}

Ref<SentryBreadcrumb> AndroidSDK::create_breadcrumb() {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, nullptr);
	int32_t handle = plugin->call(ANDROID_SN(createBreadcrumb));
	Ref<AndroidBreadcrumb> crumb = memnew(AndroidBreadcrumb(_android_plugin, handle));
	return crumb;
}

void AndroidSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	Ref<AndroidBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	plugin->call(ANDROID_SN(addBreadcrumb), crumb->get_handle());
}

void AndroidSDK::capture_log(const Ref<SentryScope> &p_scope, LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	if (p_body.is_empty()) {
		return;
	}

	ERR_FAIL_COND(p_scope.is_null());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	plugin->call(ANDROID_SN(log),
			android_scope->get_handle(),
			p_level, p_body, _sanitize_attributes(p_attributes));
}

String AndroidSDK::get_last_event_id() {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(getLastEventId));
}

Ref<SentryEvent> AndroidSDK::create_event() {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, nullptr);
	int32_t event_handle = plugin->call(ANDROID_SN(createEvent));
	Ref<AndroidEvent> event = memnew(AndroidEvent(_android_plugin, event_handle));
	return event;
}

String AndroidSDK::capture_event(const Ref<SentryScope> &p_scope, const Ref<SentryEvent> &p_event) {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());

	Ref<AndroidEvent> android_event = p_event;
	ERR_FAIL_COND_V(android_event.is_null(), String());

	ERR_FAIL_COND_V(p_scope.is_null(), String());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	return plugin->call(ANDROID_SN(captureEvent),
			android_scope->get_handle(),
			android_event->get_handle());
}

void AndroidSDK::capture_feedback(const Ref<SentryScope> &p_scope, const Ref<SentryFeedback> &p_feedback) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");

	ERR_FAIL_COND(p_scope.is_null());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	plugin->call(ANDROID_SN(captureFeedback),
			android_scope->get_handle(),
			p_feedback->get_message(),
			p_feedback->get_contact_email(),
			p_feedback->get_name(),
			p_feedback->get_associated_event_id());
}

void AndroidSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	ERR_FAIL_COND(p_attachment.is_null());

	if (!p_attachment->get_path().is_empty()) {
		// File attachment
		String absolute_path = p_attachment->get_globalized_path();
		sentry::logging::print_debug("attaching file: ", absolute_path);
		plugin->call(ANDROID_SN(addFileAttachment),
				absolute_path,
				p_attachment->get_effective_filename(),
				p_attachment->get_content_type(),
				p_attachment->get_attachment_type());
	} else {
		// Bytes attachment
		ERR_FAIL_COND_MSG(p_attachment->get_filename().is_empty(), "Sentry: Can't add bytes attachment without filename.");
		sentry::logging::print_debug("attaching bytes with filename: ", p_attachment->get_filename());
		plugin->call(ANDROID_SN(addBytesAttachment),
				p_attachment->get_bytes(),
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				p_attachment->get_attachment_type());
	}
}

void AndroidSDK::metrics_add_count(const Ref<SentryScope> &p_scope, const String &p_name, int64_t p_value, const Dictionary &p_attributes) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	ERR_FAIL_COND(p_scope.is_null());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	plugin->call(ANDROID_SN(metricsAddCount),
			android_scope->get_handle(),
			p_name, p_value, _sanitize_attributes(p_attributes));
}

void AndroidSDK::metrics_add_gauge(const Ref<SentryScope> &p_scope, const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	ERR_FAIL_COND(p_scope.is_null());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	plugin->call(ANDROID_SN(metricsAddGauge),
			android_scope->get_handle(),
			p_name, p_value, p_unit, _sanitize_attributes(p_attributes));
}

void AndroidSDK::metrics_add_distribution(const Ref<SentryScope> &p_scope, const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	ERR_FAIL_COND(p_scope.is_null());
	AndroidScope *android_scope = static_cast<AndroidScope *>(p_scope->get_implementation());

	plugin->call(ANDROID_SN(metricsAddDistribution),
			android_scope->get_handle(),
			p_name, p_value, p_unit, _sanitize_attributes(p_attributes));
}

void AndroidSDK::set_attribute(const String &p_name, const Variant &p_value) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			plugin->call(ANDROID_SN(setAttributeBool), p_name, p_value);
		} break;
		case Variant::Type::INT: {
			plugin->call(ANDROID_SN(setAttributeLong), p_name, p_value);
		} break;
		case Variant::Type::FLOAT: {
			plugin->call(ANDROID_SN(setAttributeDouble), p_name, p_value);
		} break;
		case Variant::Type::STRING: {
			plugin->call(ANDROID_SN(setAttributeString), p_name, p_value);
		} break;
		default: {
			plugin->call(ANDROID_SN(setAttributeString), p_name, p_value.stringify());
		} break;
	}
}

void AndroidSDK::remove_attribute(const String &p_name) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	plugin->call(ANDROID_SN(removeAttribute), p_name);
}

SentryScopeImpl *AndroidSDK::create_scope() {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, memnew(AndroidScope()));
	int32_t handle = plugin->call(ANDROID_SN(createScope));
	return memnew(AndroidScope(_android_plugin, handle));
}

SentrySpanImpl *AndroidSDK::create_span(const String &p_name, const Dictionary &p_attributes) {
	return AndroidSpan::start_root(_android_plugin, p_name, p_attributes);
}

void AndroidSDK::set_trace(const String &p_trace_id, const String &p_parent_span_id) {
	ERR_FAIL_COND(p_trace_id.is_empty());

	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	plugin->call(ANDROID_SN(setTrace), p_trace_id, p_parent_span_id);
}

void AndroidSDK::_add_default_attachments() {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	for (const Ref<SentryAttachment> &att : SENTRY_OPTIONS()->get_default_attachments()) {
		plugin->call(ANDROID_SN(addFileAttachment),
				att->get_globalized_path(),
				att->get_effective_filename(),
				att->get_content_type(),
				att->get_attachment_type());
	}
}

void AndroidSDK::clear_attachments() {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	plugin->call(ANDROID_SN(clearAttachments));
	_add_default_attachments();
}

void AndroidSDK::init() {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	_add_default_attachments();

	Dictionary optionsData;
	optionsData["dsn"] = SENTRY_OPTIONS()->get_dsn();
	optionsData["debug"] = SENTRY_OPTIONS()->is_debug_enabled();
	optionsData["release"] = SENTRY_OPTIONS()->get_release();
	optionsData["dist"] = SENTRY_OPTIONS()->get_dist();
	optionsData["environment"] = SENTRY_OPTIONS()->get_environment();
	optionsData["sample_rate"] = SENTRY_OPTIONS()->get_sample_rate();
	optionsData["traces_sample_rate"] = SENTRY_OPTIONS()->get_traces_sample_rate();
	optionsData["trace_propagation_targets"] = _trace_propagation_targets_to_strings(
			SENTRY_OPTIONS()->get_trace_propagation_targets());
	optionsData["propagate_traceparent"] = SENTRY_OPTIONS()->is_propagate_traceparent_enabled();
	optionsData["org_id"] = SENTRY_OPTIONS()->get_org_id();
	optionsData["max_breadcrumbs"] = SENTRY_OPTIONS()->get_max_breadcrumbs();
	optionsData["enable_anr_detection"] = SENTRY_OPTIONS()->get_android()->get_enable_anr_detection();
	optionsData["anr_timeout_interval_ms"] = SENTRY_OPTIONS()->get_android()->get_anr_timeout_interval_ms();
	optionsData["attach_anr_thread_dump"] = SENTRY_OPTIONS()->get_android()->get_attach_anr_thread_dump();
	optionsData["shutdown_timeout_ms"] = SENTRY_OPTIONS()->get_shutdown_timeout_ms();

	plugin->call(ANDROID_SN(init),
			optionsData,
			before_send_handler->get_instance_id(),
			before_send_transaction_handler->get_instance_id(),
			before_send_feedback_handler->get_instance_id(),
			SENTRY_OPTIONS()->get_before_send_log().is_valid() ? before_send_log_handler->get_instance_id() : 0,
			SENTRY_OPTIONS()->get_before_send_metric().is_valid() ? before_send_metric_handler->get_instance_id() : 0);

	if (is_enabled()) {
		set_user(SentryUser::create_default());
	} else {
		ERR_PRINT("Sentry: Failed to initialize Android SDK.");
	}
}

void AndroidSDK::close() {
	Object *plugin = _android_plugin.get();
	if (plugin != nullptr) {
		plugin->call(ANDROID_SN(close));
	}
}

bool AndroidSDK::is_enabled() const {
	Object *plugin = _android_plugin.get();
	return plugin && plugin->call(ANDROID_SN(isEnabled));
}

AndroidSDK::AndroidSDK() {
	AndroidStringNames::create_once();

	Object *plugin = Engine::get_singleton()->get_singleton("SentryAndroidGodotPlugin");
	ERR_FAIL_NULL_MSG(plugin, "Sentry: Unable to locate SentryAndroidGodotPlugin singleton.");
	_android_plugin = util::ObjectHandle<Object>(plugin);

	before_send_handler = memnew(SentryAndroidBeforeSendHandler);
	before_send_handler->_initialize(_android_plugin);

	before_send_transaction_handler = memnew(SentryAndroidBeforeSendTransactionHandler);
	before_send_transaction_handler->_initialize(_android_plugin);

	before_send_feedback_handler = memnew(SentryAndroidBeforeSendFeedbackHandler);
	before_send_feedback_handler->_initialize(_android_plugin);

	before_send_log_handler = memnew(SentryAndroidBeforeSendLogHandler);
	before_send_log_handler->_initialize(_android_plugin);

	before_send_metric_handler = memnew(SentryAndroidBeforeSendMetricHandler);
	before_send_metric_handler->_initialize(_android_plugin);
}

AndroidSDK::~AndroidSDK() {
	if (before_send_handler) {
		memdelete(before_send_handler);
	}
	if (before_send_transaction_handler) {
		memdelete(before_send_transaction_handler);
	}
	if (before_send_feedback_handler) {
		memdelete(before_send_feedback_handler);
	}
	if (before_send_log_handler) {
		memdelete(before_send_log_handler);
	}
	if (before_send_metric_handler) {
		memdelete(before_send_metric_handler);
	}
}

} //namespace sentry::android
