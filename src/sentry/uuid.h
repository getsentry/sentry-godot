#ifndef UUID_H
#define UUID_H

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::uuid {

String make_uuid();

} // namespace sentry::uuid

#endif // UUID_H
