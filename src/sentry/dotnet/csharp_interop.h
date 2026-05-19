#pragma once

#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::dotnet {

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

} // namespace sentry::dotnet
