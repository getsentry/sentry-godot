#pragma once

#include "sentry/internal_sdk.h"

#include <godot_cpp/classes/java_script_object.hpp>

using namespace godot;

namespace sentry::javascript {

// Handles event processing
class JavaScriptBeforeSendHandler : public Object {
	GDCLASS(JavaScriptBeforeSendHandler, Object);

public:
	struct FileAttachmentsGetter {
		Vector<Ref<SentryAttachment>> (*func)(void *p_context) = nullptr;
		void *context = nullptr;

		Vector<Ref<SentryAttachment>> call() const {
			if (func != nullptr) {
				return func(context);
			}
			return Vector<Ref<SentryAttachment>>();
		}
	};

private:
	FileAttachmentsGetter _file_attachments_getter;

protected:
	static void _bind_methods() {}

public:
	void handle_before_send(const Array &p_args);

	void initialize(const FileAttachmentsGetter &p_file_attachments_getter);
	JavaScriptBeforeSendHandler() = default;
};

// Handles log processing
class JavaScriptBeforeSendLogHandler : public Object {
	GDCLASS(JavaScriptBeforeSendLogHandler, Object);

protected:
	static void _bind_methods() {}

public:
	void handle_before_send_log(const Array &p_args);

	JavaScriptBeforeSendLogHandler() = default;
};

// Internal SDK utilizing Sentry for JavaScript.
class JavaScriptSDK : public InternalSDK {
private:
	// NOTE: Need to keep these refs alive for as long as they're needed.
	Ref<JavaScriptObject> _before_send_js_callback;
	Ref<JavaScriptObject> _before_send_log_js_callback;

	JavaScriptBeforeSendHandler *_before_send_handler;
	JavaScriptBeforeSendLogHandler *_before_send_log_handler;

	// Stores file-based attachment paths to be added in processing during before_send.
	Vector<Ref<SentryAttachment>> file_attachments;

	Vector<Ref<SentryAttachment>> _get_file_attachments() { return file_attachments; }

public:
	// NOTE: JS SDK can't be intialized early because it needs JavaScriptBridge engine singleton.
	virtual BitField<Capabilities> get_capabilities() const override { return SUPPORTS_ALL & ~SUPPORTS_EARLY_INIT; }

	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void remove_context(const String &p_key) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;

	virtual Ref<SentryBreadcrumb> create_breadcrumb() override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual void log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes = Dictionary()) override;

	virtual String capture_message(const String &p_message, Level p_level = sentry::LEVEL_INFO) override;
	virtual String get_last_event_id() override;

	virtual Ref<SentryEvent> create_event() override;
	virtual String capture_event(const Ref<SentryEvent> &p_event) override;

	virtual void capture_feedback(const Ref<SentryFeedback> &p_feedback) override;

	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;

	virtual void init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) override;
	virtual void close() override;
	virtual bool is_enabled() const override;

	JavaScriptSDK();
	virtual ~JavaScriptSDK() override;
};

} //namespace sentry::javascript
