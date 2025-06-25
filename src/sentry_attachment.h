#ifndef SENTRY_ATTACHMENT_H
#define SENTRY_ATTACHMENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

#ifdef NATIVE_SDK
#include <sentry.h>
#endif

using namespace godot;

// Base class for attachment objects in the public API.
class SentryAttachment : public RefCounted {
	GDCLASS(SentryAttachment, RefCounted);

private:
	String path;
	String filename;
	String content_type;

#ifdef NATIVE_SDK
	sentry_attachment_t *native_attachment = nullptr;
#endif

protected:
	static void _bind_methods();

public:
	static Ref<SentryAttachment> create_with_path(const String &p_path, const String &p_filename = "", const String &p_content_type = "");

	void set_path(const String &p_path) { path = p_path; }
	String get_path() const { return path; }

	void set_filename(const String &p_filename) { filename = p_filename; }
	String get_filename() const { return filename; }

	void set_content_type(const String &p_content_type) { content_type = p_content_type; }
	String get_content_type() const { return content_type; }

#ifdef NATIVE_SDK
	sentry_attachment_t *get_native_attachment() const { return native_attachment; }
	void set_native_attachment(sentry_attachment_t *p_native_attachment) { native_attachment = p_native_attachment; }
#endif

	virtual ~SentryAttachment() = default;
};

#endif // SENTRY_ATTACHMENT_H
