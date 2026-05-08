#pragma once

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::uuid {

String make_uuid();
String make_uuid_no_dashes();
String make_rand_hex_16();

} // namespace sentry::uuid
