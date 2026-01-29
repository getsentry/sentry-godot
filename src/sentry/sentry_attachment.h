#ifndef SENTRY_ATTACHMENT_H
#define SENTRY_ATTACHMENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#ifdef SDK_NATIVE
#include <sentry.h>
#endif

using namespace godot;

namespace sentry {

// Represents attachments in the public API.
class SentryAttachment : public RefCounted {
	GDCLASS(SentryAttachment, RefCounted);

private:
	PackedByteArray bytes;
	String path;
	String filename;
	String content_type;
	String attachment_type;

#ifdef SDK_NATIVE
	sentry_attachment_t *native_attachment = nullptr;
#endif

protected:
	static void _bind_methods();

public:
	static Ref<SentryAttachment> create_with_path(const String &p_path);
	static Ref<SentryAttachment> create_with_bytes(const PackedByteArray &p_bytes, const String &p_filename);

	PackedByteArray get_bytes() const { return bytes; }
	void set_bytes(const PackedByteArray &p_bytes) { bytes = p_bytes; }

	String get_path() const { return path; }
	void set_path(const String &p_path) { path = p_path; }

	String get_filename() const { return filename; }
	void set_filename(const String &p_filename) { filename = p_filename; }

	String get_content_type() const { return content_type; }
	void set_content_type(const String &p_content_type) { content_type = p_content_type; }

	// NOTE: "attachment_type" property is not exposed in the API
	String get_attachment_type() const { return attachment_type; }
	void set_attachment_type(const String &p_attachment_type) { attachment_type = p_attachment_type; }

	String get_content_type_or_default() const { return content_type.is_empty() ? "application/octet-stream" : content_type; }

#ifdef SDK_NATIVE
	sentry_attachment_t *get_native_attachment() const { return native_attachment; }
	void set_native_attachment(sentry_attachment_t *p_native_attachment) { native_attachment = p_native_attachment; }
#endif

	~SentryAttachment() = default;
};

} // namespace sentry

#endif // SENTRY_ATTACHMENT_H
