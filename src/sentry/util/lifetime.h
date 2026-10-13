#pragma once

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/safe_refcount.hpp>

namespace sentry::util {

namespace detail {
template <typename T, typename M>
class CallableCpp;
}

// Embedded in the target to let callables observe its lifetime.
// Destruction or explicit invalidation prevents further calls.
class Lifetime {
	template <typename T, typename M>
	friend class detail::CallableCpp;

	struct State {
		::godot::SafeRefCount refcount;
		bool alive = true;

		State() { refcount.init(); }
	};

	State *_state = nullptr;

	State *_retain_state() const {
		_state->refcount.ref();
		return _state;
	}

	static void _release_state(State *p_state) {
		if (p_state && p_state->refcount.unref()) {
			::godot::memdelete(p_state);
		}
	}

public:
	Lifetime() :
			_state(memnew(State)) {}

	Lifetime(const Lifetime &) = delete;
	Lifetime &operator=(const Lifetime &) = delete;
	Lifetime(Lifetime &&) = delete;
	Lifetime &operator=(Lifetime &&) = delete;

	void invalidate() { _state->alive = false; }

	~Lifetime() {
		invalidate();
		_release_state(_state);
	}
};

} //namespace sentry::util

// Enables callables that become invalid during object destruction.
// Provides protected _invalidate_lifetime() for earlier invalidation during teardown.
#define LIFETIME_OBSERVABLE()                               \
private:                                                    \
	::sentry::util::Lifetime _lifetime;                     \
	template <typename, typename>                           \
	friend class ::sentry::util::detail::CallableCpp;       \
                                                            \
protected:                                                  \
	void _invalidate_lifetime() { _lifetime.invalidate(); } \
                                                            \
private:
