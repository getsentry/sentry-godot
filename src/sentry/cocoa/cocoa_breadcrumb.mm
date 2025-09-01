#include "cocoa_breadcrumb.h"

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/cocoa/cocoa_util.h"

namespace sentry::cocoa {

void CocoaBreadcrumb::set_message(const String &p_message) {
	cocoa_breadcrumb.message = string_to_objc_or_nil_if_empty(p_message);
}

String CocoaBreadcrumb::get_message() const {
	return string_from_objc(cocoa_breadcrumb.message);
}

void CocoaBreadcrumb::set_category(const String &p_category) {
	cocoa_breadcrumb.category = string_to_objc_or_nil_if_empty(p_category);
}

String CocoaBreadcrumb::get_category() const {
	return string_from_objc(cocoa_breadcrumb.category);
}

void CocoaBreadcrumb::set_level(sentry::Level p_level) {
	cocoa_breadcrumb.level = sentry_level_to_objc(p_level);
}

sentry::Level CocoaBreadcrumb::get_level() const {
	return sentry_level_from_objc(cocoa_breadcrumb.level);
}

void CocoaBreadcrumb::set_type(const String &p_type) {
	cocoa_breadcrumb.type = string_to_objc_or_nil_if_empty(p_type);
}

String CocoaBreadcrumb::get_type() const {
	return string_from_objc(cocoa_breadcrumb.type);
}

void CocoaBreadcrumb::set_data(const Dictionary &p_data) {
	cocoa_breadcrumb.data = dictionary_to_objc(p_data);
}

Ref<SentryTimestamp> CocoaBreadcrumb::get_timestamp() {
	if (cocoa_breadcrumb.timestamp == nil) {
		return Ref<SentryTimestamp>();
	}

	NSTimeInterval seconds = [cocoa_breadcrumb.timestamp timeIntervalSince1970];
	return SentryTimestamp::from_unix_time(seconds);
}

CocoaBreadcrumb::CocoaBreadcrumb() :
		cocoa_breadcrumb([[objc::SentryBreadcrumb alloc] init]) {
}

CocoaBreadcrumb::CocoaBreadcrumb(objc::SentryBreadcrumb *p_cocoa_breadcrumb) :
		cocoa_breadcrumb(p_cocoa_breadcrumb) {
	if (!p_cocoa_breadcrumb) {
		cocoa_breadcrumb = [[objc::SentryBreadcrumb alloc] init];
		ERR_PRINT_ONCE("Sentry: Internal error - cocoa breadcrumb instance is null");
	}
}

} //namespace sentry::cocoa
