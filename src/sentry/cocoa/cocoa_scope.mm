#include "cocoa_scope.h"

#include "cocoa_breadcrumb.h"
#include "cocoa_util.h"

namespace sentry::cocoa {

// NOTE: Input validations are performed by Godot-facing SentryScope.

void CocoaScope::set_context(const String &p_key, const Dictionary &p_value) {
	[_scope setContextValue:dictionary_to_objc(p_value) forKey:string_to_objc(p_key)];
}

void CocoaScope::set_tag(const String &p_key, const String &p_value) {
	[_scope setTagValue:string_to_objc(p_value) forKey:string_to_objc(p_key)];
}

void CocoaScope::set_user(const Ref<SentryUser> &p_user) {
	[_scope setUser:user_to_objc(p_user)];
}

void CocoaScope::set_level(sentry::Level p_level) {
	[_scope setLevel:sentry_level_to_objc(p_level)];
}

void CocoaScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	[_scope setFingerprint:string_array_to_objc(p_fingerprint)];
}

void CocoaScope::set_attribute(const String &p_name, const Variant &p_value) {
	[_scope setAttributeValue:variant_to_scope_attribute(p_value) forKey:string_to_objc(p_name)];
}

void CocoaScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	Ref<CocoaBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	[_scope addBreadcrumb:crumb->get_cocoa_breadcrumb()];
}

void CocoaScope::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	SentryObjCAttachment *attachment = attachment_to_objc(p_attachment);
	ERR_FAIL_NULL(attachment);
	[_scope addAttachment:attachment];
}

void CocoaScope::clear() {
	[_scope clear];
}

SentryScopeImpl *CocoaScope::clone() const {
	SentryObjCScope *copy = [SentryObjCSDK.internal.scope cloneScope:_scope];
	return memnew(CocoaScope(copy));
}

CocoaScope::CocoaScope(SentryObjCScope *p_scope) :
		_scope(p_scope) {
}

CocoaScope::~CocoaScope() {
}

} //namespace sentry::cocoa
