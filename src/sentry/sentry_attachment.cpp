#include "sentry_attachment.h"

#include "sentry/util/simple_bind.h"

#include <godot_cpp/core/class_db.hpp>

namespace sentry {

Ref<SentryAttachment> SentryAttachment::create_with_path(const String &p_path) {
	ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<SentryAttachment>(), "Sentry: Can't create attachment with an empty file path.");

	Ref<SentryAttachment> attachment = memnew(SentryAttachment);
	attachment->path = p_path;
	return attachment;
}

Ref<SentryAttachment> SentryAttachment::create_with_bytes(const PackedByteArray &p_bytes, const String &p_filename) {
	ERR_FAIL_COND_V_MSG(p_filename.is_empty(), Ref<SentryAttachment>(), "Sentry: Can't create attachment with an empty filename.");

	Ref<SentryAttachment> attachment = memnew(SentryAttachment);
	attachment->bytes = p_bytes;
	attachment->filename = p_filename;
	return attachment;
}

void SentryAttachment::_bind_methods() {
	ClassDB::bind_static_method("SentryAttachment", D_METHOD("create_with_path", "path"), &SentryAttachment::create_with_path);
	ClassDB::bind_static_method("SentryAttachment", D_METHOD("create_with_bytes", "bytes", "filename"), &SentryAttachment::create_with_bytes);

	BIND_PROPERTY(SentryAttachment, PropertyInfo(Variant::PACKED_BYTE_ARRAY, "bytes"), set_bytes, get_bytes);
	BIND_PROPERTY(SentryAttachment, PropertyInfo(Variant::STRING, "path"), set_path, get_path);
	BIND_PROPERTY(SentryAttachment, PropertyInfo(Variant::STRING, "filename"), set_filename, get_filename);
	BIND_PROPERTY(SentryAttachment, PropertyInfo(Variant::STRING, "content_type"), set_content_type, get_content_type);
}

} // namespace sentry
