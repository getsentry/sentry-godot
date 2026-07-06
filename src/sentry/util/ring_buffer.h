#pragma once

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/cowdata.hpp>

namespace sentry::util {

// Fixed-capacity buffer that stores elements in insertion order.
// When full, pushing a new element overwrites the oldest one.
// Index 0 is the oldest element.
template <typename T>
class RingBuffer {
private:
	godot::CowData<T> _cowdata;
	int _capacity = 0;
	int _next = 0;

public:
	_FORCE_INLINE_ int size() const { return _cowdata.size(); }
	_FORCE_INLINE_ bool is_empty() const { return _cowdata.is_empty(); }

	void push(const T &p_element) {
		if (_capacity == 0) {
			return;
		}
		if (_cowdata.size() < _capacity) {
			_cowdata.resize(_cowdata.size() + 1);
		}
		_cowdata.set(_next, p_element);
		_next = (_next + 1) % _capacity;
	}

	const T &operator[](int p_index) const {
		return _cowdata.get((_next - _cowdata.size() + p_index + _capacity) % _capacity);
	}

	void clear() {
		_cowdata.clear();
		_next = 0;
	}

	RingBuffer() = delete;

	explicit RingBuffer(int p_capacity) {
		if (p_capacity > 0) {
			_capacity = p_capacity;
		}
	}
};

} //namespace sentry::util
