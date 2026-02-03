#pragma once

#include <godot_cpp/core/defs.hpp>
#include <string_view>

namespace sentry::util {

// FNV-1a hash - portable hash implementation that doesn't rely on std::hash<*>.
// See https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
_FORCE_INLINE_ size_t fnv1a_hash(const char *p_data, size_t p_len) {
	// FNV-1a constants for 64-bit or 32-bit depending on platform
	// See table: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV_hash_parameters
	constexpr size_t FNV_OFFSET_BASIS = sizeof(size_t) == 8 ? 14695981039346656037ULL : 2166136261U;
	constexpr size_t FNV_PRIME = sizeof(size_t) == 8 ? 1099511628211ULL : 16777619U;

	size_t hash = FNV_OFFSET_BASIS;
	for (size_t i = 0; i < p_len; i++) {
		hash ^= static_cast<uint64_t>(static_cast<unsigned char>(p_data[i]));
		hash *= FNV_PRIME;
	}
	return hash;
}

_FORCE_INLINE_ size_t hash(std::string_view p_value) {
	return fnv1a_hash(p_value.data(), p_value.size());
}

_FORCE_INLINE_ size_t hash(int p_value) {
	return p_value;
}

template <class T>
_FORCE_INLINE_ void hash_combine(std::size_t &p_hash, const T &p_value) {
	// NOTE: Hash combining technique, originally from boost.
	p_hash ^= hash(p_value) + 0x9e3779b9 + (p_hash << 6) + (p_hash >> 2);
}

} //namespace sentry::util
