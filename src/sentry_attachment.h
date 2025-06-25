#ifndef SENTRY_ATTACHMENT_H
#define SENTRY_ATTACHMENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Base class for attachment objects in the public API.
class SentryAttachment : public RefCounted {
	GDCLASS(SentryAttachment, RefCounted);

private:
	String file_path;
	String content_type;

protected:
	static void _bind_methods();

public:
	void set_file_path(const String &p_path) { file_path = p_path; }
	String get_file_path() const { return file_path; }

	void set_content_type(const String &p_content_type) { content_type = p_content_type; }
	String get_content_type() const { return content_type; }

	static Ref<SentryAttachment> create_with_path(const String &p_path, const String &p_content_type = "");

	virtual ~SentryAttachment() = default;
};

#endif // SENTRY_ATTACHMENT_H
