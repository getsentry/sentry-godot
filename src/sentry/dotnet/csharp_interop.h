#pragma once

#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::dotnet {

// Returns true on a mono (.NET) Godot build.
bool godot_supports_dotnet();

// Calls the .NET init, if available. No-op in standard non-.NET Godot builds.
void init();

// Calls the .NET close, if available. No-op in standard non-.NET Godot builds.
void close();

// Forwards a C# exception error to the .NET layer for capture.
void handle_logger_error(const String &p_file, const String &p_code);

void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb);

void set_tag(const String &p_key, const String &p_value);
void remove_tag(const String &p_key);

void set_user(const Ref<SentryUser> &p_user);
void remove_user();

// Forwards a native/engine event to the options.Native.SetBeforeSend callback in the .NET layer.
// Returns true to keep the event, false to discard it. Mutates the event in place.
// No-op returning true when the .NET layer or callback is unavailable.
bool process_event_in_managed_layer(const Ref<SentryEvent> &p_event);

// Returns true once the managed layer has loaded and registered its native callbacks.
bool is_managed_layer_registered();

#ifdef TESTS_ENABLED

bool is_before_send_defined();

#endif // TESTS_ENABLED

} // namespace sentry::dotnet
