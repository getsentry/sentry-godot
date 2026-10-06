#pragma once

#include <cstdint>

#include <godot_cpp/core/defs.hpp>

namespace sentry {

// Base for metrics collectors.
class MetricsCollector {
private:
	bool _enabled = false;

protected:
	virtual void _process(uint64_t p_now_usec) = 0;

public:
	_FORCE_INLINE_ void set_enabled(bool p_enabled) {
		_enabled = p_enabled;
	}

	_FORCE_INLINE_ void process(uint64_t p_now_usec) {
		if (_enabled) {
			_process(p_now_usec);
		}
	}

	virtual void reset() {}

	virtual ~MetricsCollector() {}
};

} //namespace sentry
