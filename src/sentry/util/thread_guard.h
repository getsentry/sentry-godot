#pragma once

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace sentry::util {

// Records the constructing thread so methods can reject calls from other threads.
// Declare with SENTRY_THREAD_OWNER and guard methods with ERR_SENTRY_THREAD_GUARD.
class ThreadOwner {
private:
	uint64_t _owner_tid = ::godot::OS::get_singleton()->get_thread_caller_id();

public:
	_FORCE_INLINE_ bool is_owner_thread() const {
		return _owner_tid == ::godot::OS::get_singleton()->get_thread_caller_id();
	}
};

} //namespace sentry::util

// Declares the thread-owner member. Add to the private section of the guarded class.
#define SENTRY_THREAD_OWNER ::sentry::util::ThreadOwner _thread_owner

// Rejects the call with m_msg if it comes from a thread other than the one that
// constructed the object. Requires SENTRY_THREAD_OWNER in the class.
#define ERR_SENTRY_THREAD_GUARD(m_msg) ERR_FAIL_COND_MSG(!_thread_owner.is_owner_thread(), m_msg)

// Same as ERR_SENTRY_THREAD_GUARD, but returns m_ret from methods that have a return value.
#define ERR_SENTRY_THREAD_GUARD_V(m_ret, m_msg) ERR_FAIL_COND_V_MSG(!_thread_owner.is_owner_thread(), m_ret, m_msg)
