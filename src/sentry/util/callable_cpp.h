#pragma once

#include "lifetime.h"

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <cstring>
#include <type_traits>

namespace sentry::util {

namespace detail {

template <typename M>
struct CppMethod;

template <typename T, typename R, typename... P>
struct CppMethod<R (T::*)(P...)> {
	static constexpr int argument_count = sizeof...(P);

	static void call(T *p_target, R (T::*p_method)(P...), const ::godot::Variant **p_args, ::godot::Variant &r_return, GDExtensionCallError &r_error) {
		if constexpr (std::is_void_v<R>) {
			::godot::call_with_variant_args_helper(p_target, p_method, p_args, r_error, ::godot::BuildIndexSequence<sizeof...(P)>{});
		} else {
			::godot::call_with_variant_args_ret_helper(p_target, p_method, p_args, r_return, r_error, ::godot::BuildIndexSequence<sizeof...(P)>{});
		}
	}
};

template <typename T, typename R, typename... P>
struct CppMethod<R (T::*)(P...) const> {
	static constexpr int argument_count = sizeof...(P);

	static void call(const T *p_target, R (T::*p_method)(P...) const, const ::godot::Variant **p_args, ::godot::Variant &r_return, GDExtensionCallError &r_error) {
		// Godot's helpers take a mutable pointer even when invoking a const method.
		T *target = const_cast<T *>(p_target);
		if constexpr (std::is_void_v<R>) {
			::godot::call_with_variant_argsc_helper(target, p_method, p_args, r_error, ::godot::BuildIndexSequence<sizeof...(P)>{});
		} else {
			::godot::call_with_variant_args_retc_helper(target, p_method, p_args, r_return, r_error, ::godot::BuildIndexSequence<sizeof...(P)>{});
		}
	}
};

// Binds a C++ method and prevents calls after the target's lifetime ends.
// Requires the target class to declare LIFETIME_OBSERVABLE().
template <typename T, typename M>
class CallableCpp final : public ::godot::CallableCustom {
	using Method = CppMethod<M>;

	struct Data {
		T *target;
		Lifetime::State *state;
		M method;
	} _data;
	uint32_t _hash;

	static bool _compare_equal(const ::godot::CallableCustom *p_a, const ::godot::CallableCustom *p_b) {
		const auto *a = static_cast<const CallableCpp *>(p_a);
		const auto *b = static_cast<const CallableCpp *>(p_b);
		return memcmp(&a->_data, &b->_data, sizeof(Data)) == 0;
	}

	static bool _compare_less(const ::godot::CallableCustom *p_a, const ::godot::CallableCustom *p_b) {
		const auto *a = static_cast<const CallableCpp *>(p_a);
		const auto *b = static_cast<const CallableCpp *>(p_b);
		return memcmp(&a->_data, &b->_data, sizeof(Data)) < 0;
	}

public:
	CallableCpp(T *p_target, M p_method) {
		// Zero-pad so we can hash it reliably.
		memset(&_data, 0, sizeof(Data));
		_data.target = p_target;
		_data.state = p_target ? p_target->_lifetime._retain_state() : nullptr;
		_data.method = p_method;
		_hash = ::godot::hash_murmur3_buffer(&_data, sizeof(Data));
	}

	CallableCpp(const CallableCpp &) = delete;
	CallableCpp &operator=(const CallableCpp &) = delete;

	~CallableCpp() override { Lifetime::_release_state(_data.state); }

	uint32_t hash() const override { return _hash; }
	::godot::String get_as_text() const override { return "callable_cpp()"; }
	CompareEqualFunc get_compare_equal_func() const override { return &_compare_equal; }
	CompareLessFunc get_compare_less_func() const override { return &_compare_less; }
	bool is_valid() const override { return _data.state && _data.state->alive; }
	::godot::ObjectID get_object() const override { return {}; }

	int get_argument_count(bool &r_is_valid) const override {
		r_is_valid = true;
		return Method::argument_count;
	}

	void call(const ::godot::Variant **p_arguments, [[maybe_unused]] int p_argcount, ::godot::Variant &r_return_value, GDExtensionCallError &r_call_error) const override {
		r_return_value = ::godot::Variant();
		r_call_error = {};
		if (!is_valid()) {
			r_call_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
			return;
		}
#ifdef DEBUG_ENABLED
		if (p_argcount < Method::argument_count) {
			r_call_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_call_error.expected = Method::argument_count;
			return;
		}
		if (p_argcount > Method::argument_count) {
			r_call_error.error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_call_error.expected = Method::argument_count;
			return;
		}
#endif
		Method::call(_data.target, _data.method, p_arguments, r_return_value, r_call_error);
	}
};

} //namespace detail

template <typename T, typename R, typename... P>
::godot::Callable create_cpp_callable(T *p_target, R (T::*p_method)(P...)) {
	using Custom = detail::CallableCpp<T, decltype(p_method)>;
	return ::godot::Callable(memnew(Custom(p_target, p_method)));
}

template <typename T, typename R, typename... P>
::godot::Callable create_cpp_callable(const T *p_target, R (T::*p_method)(P...) const) {
	using Custom = detail::CallableCpp<const T, decltype(p_method)>;
	return ::godot::Callable(memnew(Custom(p_target, p_method)));
}

} //namespace sentry::util

// Binds a method on a LIFETIME_OBSERVABLE() C++ target. Invocation and teardown must share a thread.
#define callable_cpp(I, M) ::sentry::util::create_cpp_callable(I, M)
