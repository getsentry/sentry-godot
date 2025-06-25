#include "sentry_attachment.h"

#include "sentry/simple_bind.h"

#include <godot_cpp/core/class_db.hpp>

Ref<SentryAttachment> SentryAttachment::create_with_path(const String &p_path, const String &p_filename, const String &p_content_type) {
	ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<SentryAttachment>(), "Sentry: Can't create attachment with an empty file path.");

	Ref<SentryAttachment> attachment = memnew(SentryAttachment);
	attachment->path = p_path;
	attachment->filename = p_filename;
	attachment->content_type = p_content_type;
	return attachment;
}

Ref<SentryAttachment> SentryAttachment::create_with_bytes(const PackedByteArray &p_bytes, const String &p_filename, const String &p_content_type) {
	ERR_FAIL_COND_V_MSG(p_filename.is_empty(), Ref<SentryAttachment>(), "Sentry: Can't create attachment with an empty filename.");

	Ref<SentryAttachment> attachment = memnew(SentryAttachment);
	attachment->bytes = p_bytes;
	attachment->filename = p_filename;
	attachment->content_type = p_content_type;
	return attachment;
}

void SentryAttachment::_bind_methods() {
	ClassDB::bind_static_method("SentryAttachment", D_METHOD("create_with_path", "path", "filename", "content_type"), &SentryAttachment::create_with_path, DEFVAL(""), DEFVAL(""));
	ClassDB::bind_static_method("SentryAttachment", D_METHOD("create_with_bytes", "bytes", "filename", "content_type"), &SentryAttachment::create_with_bytes, DEFVAL(""));

	BIND_PROPERTY_READONLY(SentryAttachment, PropertyInfo(Variant::PACKED_BYTE_ARRAY, "bytes"), get_bytes);
	BIND_PROPERTY_READONLY(SentryAttachment, PropertyInfo(Variant::STRING, "path"), get_path);
	BIND_PROPERTY_READONLY(SentryAttachment, PropertyInfo(Variant::STRING, "filename"), get_filename);
	BIND_PROPERTY_READONLY(SentryAttachment, PropertyInfo(Variant::STRING, "content_type"), get_content_type);
}
