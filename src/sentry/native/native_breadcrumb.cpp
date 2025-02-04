#include "native_breadcrumb.h"
#include "godot_cpp/core/error_macros.hpp"
#include "sentry/native/native_util.h"

#include <sentry.h>

void NativeBreadcrumb::set_message(const String &p_message) {
	sentry::native::sentry_value_set_or_remove_string_by_key(native_crumb, "message", p_message);
}

String NativeBreadcrumb::get_message() const {
	return sentry_value_as_string(
			sentry_value_get_by_key(native_crumb, "message"));
}

void NativeBreadcrumb::set_category(const String &p_category) {
	sentry::native::sentry_value_set_or_remove_string_by_key(native_crumb, "category", p_category);
}

String NativeBreadcrumb::get_category() const {
	return sentry_value_as_string(
			sentry_value_get_by_key(native_crumb, "category"));
}

void NativeBreadcrumb::set_level(sentry::Level p_level) {
	sentry_value_set_by_key(native_crumb, "level",
			sentry_value_new_string(sentry::native::level_to_cstring(p_level)));
}

sentry::Level NativeBreadcrumb::get_level() const {
	sentry_value_t value = sentry_value_get_by_key(native_crumb, "level");
	if (sentry_value_is_null(value)) {
		return sentry::Level::LEVEL_ERROR;
	}
	return sentry::native::cstring_to_level(sentry_value_as_string(value));
}

void NativeBreadcrumb::set_type(const String &p_type) {
	sentry::native::sentry_value_set_or_remove_string_by_key(native_crumb, "type", p_type);
}

String NativeBreadcrumb::get_type() const {
	return sentry_value_as_string(
			sentry_value_get_by_key(native_crumb, "type"));
}

void NativeBreadcrumb::set_data(const Dictionary &p_data) {
	sentry_value_t native_data = sentry::native::variant_to_sentry_value(p_data);
	sentry_value_set_by_key(native_crumb, "data", native_data);
}

Dictionary NativeBreadcrumb::get_data() const {
	return sentry::native::sentry_value_to_variant(sentry_value_get_by_key(native_crumb, "data"));
}

NativeBreadcrumb::NativeBreadcrumb(const String &p_message, const String &p_category, sentry::Level p_level, const String &p_type, const Dictionary &p_data) {
	native_crumb = sentry_value_new_object();

	if (!p_message.is_empty()) {
		sentry_value_set_by_key(native_crumb, "message",
				sentry_value_new_string(p_message.utf8()));
	}

	if (!p_category.is_empty()) {
		sentry_value_set_by_key(native_crumb, "category",
				sentry_value_new_string(p_category.utf8()));
	}

	sentry_value_set_by_key(native_crumb, "level",
			sentry_value_new_string(sentry::native::level_to_cstring(p_level)));

	if (!p_type.is_empty()) {
		sentry_value_set_by_key(native_crumb, "type",
				sentry_value_new_string(p_type.utf8()));
	}

	if (!p_data.is_empty()) {
		sentry_value_set_by_key(native_crumb, "data",
				sentry::native::variant_to_sentry_value(p_data));
	}
}

NativeBreadcrumb::NativeBreadcrumb(sentry_value_t p_native_crumb) {
	if (sentry_value_refcount(p_native_crumb) > 0) {
		sentry_value_incref(p_native_crumb); // acquire ownership
		native_crumb = p_native_crumb;
	} else {
		// Shouldn't happen in healthy code.
		native_crumb = sentry_value_new_object();
		ERR_PRINT("Sentry: Internal error: Breadcrumb refcount is zero.");
	}
	native_crumb = p_native_crumb;
}

NativeBreadcrumb::NativeBreadcrumb() {
	native_crumb = sentry_value_new_object();
}

NativeBreadcrumb::~NativeBreadcrumb() {
	sentry_value_decref(native_crumb); // release ownership
}
