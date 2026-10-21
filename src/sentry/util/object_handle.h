#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <type_traits>

namespace sentry::util {

// Stores Godot objects by instance ID.
// Non-owning: resolving an ID does not prevent concurrent destruction.
template <typename T>
class ObjectHandle {
	static_assert(std::is_base_of_v<godot::Object, T>);

	godot::ObjectID _instance_id;

public:
	ObjectHandle() = default;
	explicit ObjectHandle(T *p_object) :
			_instance_id(p_object ? p_object->get_instance_id() : uint64_t(0)) {}

	_FORCE_INLINE_ T *get() const {
		return godot::Object::cast_to<T>(godot::ObjectDB::get_instance(_instance_id));
	}
};

} // namespace sentry::util
