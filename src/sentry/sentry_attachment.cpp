#include "sentry_attachment.h"

#include "sentry/util/simple_bind.h"

#include <godot_cpp/classes/project_settings.hpp>
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

void SentryAttachment::set_bytes(const PackedByteArray &p_bytes) {
	if (!p_bytes.is_empty() && !path.is_empty()) {
		ERR_PRINT("Sentry: Setting bytes on an attachment that already has a path set. The path will take priority; bytes will be ignored.");
	}
	bytes = p_bytes;
}

void SentryAttachment::set_path(const String &p_path) {
	if (!p_path.is_empty() && !bytes.is_empty()) {
		ERR_PRINT("Sentry: Setting path on an attachment that already has bytes set. The path will take priority; bytes will be ignored.");
	}
	path = p_path;
}

String SentryAttachment::get_globalized_path() const {
	if (path.begins_with("res://") || path.begins_with("user://")) {
		return ProjectSettings::get_singleton()->globalize_path(path);
	}
	return path;
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
