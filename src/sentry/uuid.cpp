#include "uuid.h"

#include <random>

#include <godot_cpp/variant/packed_byte_array.hpp>

namespace {

inline PackedByteArray _generate_uuid_v4() {
	PackedByteArray data;
	data.resize(16);
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<int> dist{ 0, 256 }; // Limits
	for (int i = 0; i < 16; i++) {
		data[i] = ((unsigned char)dist(gen));
	}
	data[6] = (data[6] & 0x0F) | 0x40; // Version 4
	data[8] = (data[8] & 0x3F) | 0x80; // Variant 10xx
	return data;
}

inline String _uuid_to_string(const PackedByteArray &p_uuid) {
	char buffer[37];
	std::snprintf(buffer, sizeof(buffer),
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			p_uuid[0], p_uuid[1], p_uuid[2], p_uuid[3],
			p_uuid[4], p_uuid[5],
			p_uuid[6], p_uuid[7],
			p_uuid[8], p_uuid[9],
			p_uuid[10], p_uuid[11], p_uuid[12], p_uuid[13], p_uuid[14], p_uuid[15]);
	return String(buffer);
}

} // unnamed namespace

namespace sentry::uuid {

String make_uuid() {
	return _uuid_to_string(_generate_uuid_v4());
}

} // namespace sentry::uuid
