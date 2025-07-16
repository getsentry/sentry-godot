#ifndef SENTRY_OBJC_WRAPPER_H
#define SENTRY_OBJC_WRAPPER_H

#include "sentry/common_defs.h"
#include "sentry/level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::objc_wrapper {

void initialize_sentry_sdk(const String &dsn, bool debug_enabled);
void shutdown_sentry_sdk();

void set_user_objc(const String &user_id, const String &username, const String &email, const String &ip_address);
void remove_user_objc();

void add_breadcrumb_objc(const String &message, const String &category, Level level, const String &type);

String capture_message_objc(const String &message, Level level);
String get_last_event_id_objc();

void set_context_objc(const String &key, const Dictionary &value);
void remove_context_objc(const String &key);

void set_tag_objc(const String &key, const String &value);
void remove_tag_objc(const String &key);

} // namespace sentry::objc_wrapper

#endif // SENTRY_OBJC_WRAPPER_H
