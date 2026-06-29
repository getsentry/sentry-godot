#pragma once

#include "godot_cpp/core/defs.hpp"

namespace sentry::util {

// RAII guard for tracking and limiting recursive re-entry.
// Uses caller-provided depth storage, which must be initialized to 0.
// The depth counter must be externally synchronized (e.g. thread_local).
class RecursionGuard {
private:
	uint32_t *_depth_ptr;
	uint32_t _max_depth = 1;

public:
	_FORCE_INLINE_ bool should_proceed() const { return (*_depth_ptr) <= _max_depth; }

	RecursionGuard(uint32_t *p_depth_storage, uint32_t p_max_depth = 1) :
			_depth_ptr(p_depth_storage), _max_depth(p_max_depth) {
		++(*_depth_ptr);
	}
	~RecursionGuard() { --(*_depth_ptr); }

	RecursionGuard(const RecursionGuard &) = delete;
	RecursionGuard &operator=(const RecursionGuard &) = delete;
	RecursionGuard(RecursionGuard &&) = delete;
	RecursionGuard &operator=(RecursionGuard &&) = delete;
};

} //namespace sentry::util
