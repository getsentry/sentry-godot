#ifndef SIMPLE_BIND_H
#define SIMPLE_BIND_H

#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/string.hpp"
#include <godot_cpp/core/class_db.hpp>

// This file contains macros to simplify the binding of properties from a C++ class to the Godot ClassDB,
// which makes them accessible from GDScript and the Godot editor.

namespace sentry::bind {

// Registers the specified property and its accessors in the ClassDB.
// Note: This function is used in the macros defined below.
template <auto Setter, auto Getter>
_FORCE_INLINE_ void bind_property(const godot::StringName &p_class, const godot::PropertyInfo &p_info, const godot::StringName &p_setter_name, const godot::StringName &p_getter_name) {
	godot::ClassDB::bind_method(D_METHOD(p_setter_name, p_info.name), Setter);
	godot::ClassDB::bind_method(D_METHOD(p_getter_name), Getter);
	godot::ClassDB::add_property(p_class, p_info, p_setter_name, p_getter_name);
}

// Registers read-only property and its getter in the ClassDB.
template <auto Getter>
_FORCE_INLINE_ void bind_property_readonly(const godot::StringName &p_class, const godot::PropertyInfo &p_info, const godot::StringName &p_getter_name) {
	godot::ClassDB::bind_method(D_METHOD(p_getter_name), Getter);
	godot::ClassDB::add_property(p_class, p_info, "", p_getter_name);
}

} //namespace sentry::bind

// Macro to bind a property and its getter and setter.
#define BIND_PROPERTY(m_class, m_prop_info, m_setter, m_getter) \
	sentry::bind::bind_property<&m_class::m_setter, &m_class::m_getter>(m_class::get_class_static(), m_prop_info, #m_setter, #m_getter)

// A simplified version of the previous macro, for basic cases.
#define BIND_PROPERTY_SIMPLE(m_class, m_type, m_property) \
	sentry::bind::bind_property<&m_class::set_##m_property, &m_class::get_##m_property>(m_class::get_class_static(), PropertyInfo(m_type, #m_property), "set_" #m_property, "get_" #m_property)

// Macro to bind a read-only property and its getter.
#define BIND_PROPERTY_READONLY(m_class, m_prop_info, m_getter) \
	sentry::bind::bind_property_readonly<&m_class::m_getter>(m_class::get_class_static(), m_prop_info, #m_getter)

// Macro to define a simple property with accessors and a default value.
// Note: The property will be compatible with BIND_PROPERTY_SIMPLE macro.
#define SIMPLE_PROPERTY(m_type, m_property, m_default)              \
	m_type m_property = m_default;                                  \
	void set_##m_property(m_type p_value) { m_property = p_value; } \
	m_type get_##m_property() { return m_property; }

#endif // SIMPLE_BIND_H
