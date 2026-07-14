#pragma once

#include <type_traits>
#include <utility>

namespace sentry::util {

// A callback bound to an instance, or to a free function.
template <typename... Args>
class Callback {
private:
	void (*_function)(void *p_context, Args... p_args) = nullptr;
	void *_context = nullptr; // instance ptr or nullptr for free functions

	template <auto Method, typename T>
	static void _call_member_func(void *p_context, Args... p_args) {
		(static_cast<T *>(p_context)->*Method)(std::forward<Args>(p_args)...);
	}

	template <auto Function>
	static void _call_free_func(void *, Args... p_args) {
		Function(std::forward<Args>(p_args)...);
	}

public:
	// Binds a member function to an instance.
	template <auto Method, typename T>
	static Callback bind(T *p_instance) {
		static_assert(!std::is_const_v<T>,
				"Callback::bind: instance must be mutable");
		static_assert(std::is_invocable_v<decltype(Method), T *, Args...>,
				"Callback::bind: Method must be a member function of T callable with Args...");
		return Callback(&_call_member_func<Method, T>, p_instance);
	}

	// Binds a free function.
	template <auto Function>
	static Callback bind() {
		static_assert(!std::is_member_function_pointer_v<decltype(Function)>,
				"Callback::bind: use bind<&T::method>(instance) to bind a member function");
		static_assert(std::is_invocable_v<decltype(Function), Args...>,
				"Callback::bind: Function must be callable with Args...");
		return Callback(&_call_free_func<Function>, nullptr);
	}

	bool is_valid() const {
		return _function != nullptr;
	}

	void call(Args... p_args) const {
		if (is_valid()) {
			_function(_context, std::forward<Args>(p_args)...);
		}
	}

	bool operator==(const Callback &p_other) const {
		return _function == p_other._function && _context == p_other._context;
	}

	Callback() = default;

	Callback(void (*p_function)(void *p_context, Args... p_args), void *p_context) :
			_function(p_function), _context(p_context) {}
};

} // namespace sentry::util
