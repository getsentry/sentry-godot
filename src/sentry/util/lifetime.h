#pragma once

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/safe_refcount.hpp>

#include <utility>

namespace sentry::util {

namespace detail {
template <typename T, typename M>
class CallableCpp;
}

template <typename T>
class WeakRef;

// Embedded in the target to share lifetime state with weak references.
// Destruction or explicit invalidation makes those weak references return nullptr.
class Lifetime {
	template <typename T>
	friend class WeakRef;
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

// Observes the target's lifetime without keeping it alive.
// The target class must declare LIFETIME_OBSERVABLE().
template <typename T>
class WeakRef {
	T *_target = nullptr;
	Lifetime::State *_state = nullptr;

public:
	WeakRef() = default;

	explicit WeakRef(T *p_target) :
			_target(p_target),
			_state(p_target ? p_target->_lifetime._retain_state() : nullptr) {}

	WeakRef(const WeakRef &p_other) :
			_target(p_other._target), _state(p_other._state) {
		if (_state) {
			_state->refcount.ref();
		}
	}

	WeakRef(WeakRef &&p_other) noexcept :
			_target(std::exchange(p_other._target, nullptr)),
			_state(std::exchange(p_other._state, nullptr)) {}

	WeakRef &operator=(WeakRef p_other) noexcept {
		std::swap(_target, p_other._target);
		std::swap(_state, p_other._state);
		return *this;
	}

	_FORCE_INLINE_ bool is_alive() const { return _state && _state->alive; }
	_FORCE_INLINE_ T *get() const { return is_alive() ? _target : nullptr; }

	~WeakRef() { Lifetime::_release_state(_state); }
};

} //namespace sentry::util

// Enables weak references that become invalid during object destruction.
// Provides protected _invalidate_lifetime() for earlier invalidation during teardown.
#define LIFETIME_OBSERVABLE()                               \
private:                                                    \
	::sentry::util::Lifetime _lifetime;                     \
	template <typename>                                     \
	friend class ::sentry::util::WeakRef;                   \
	template <typename, typename>                           \
	friend class ::sentry::util::detail::CallableCpp;       \
                                                            \
protected:                                                  \
	void _invalidate_lifetime() { _lifetime.invalidate(); } \
                                                            \
private:
