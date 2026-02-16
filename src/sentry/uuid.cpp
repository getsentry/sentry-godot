#include "uuid.h"

#include <random>

#include <godot_cpp/variant/packed_byte_array.hpp>

namespace {

inline PackedByteArray _generate_uuid_v4() {
	PackedByteArray data;
	data.resize(16);
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<int> dist{ 0, 255 }; // Limits
	for (int i = 0; i < 16; i++) {
		data[i] = ((unsigned char)dist(gen));
	}
	data[6] = (data[6] & 0x0F) | 0x40; // Version 4
	data[8] = (data[8] & 0x3F) | 0x80; // Variant 10xx
	return data;
}

constexpr const char *FORMAT_WITH_DASHES = "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x";
constexpr const char *FORMAT_NO_DASHES = "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x";

inline String _uuid_to_string(const PackedByteArray &p_uuid, const char *p_format) {
	char buffer[37];
	std::snprintf(buffer, sizeof(buffer), p_format,
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
	return _uuid_to_string(_generate_uuid_v4(), FORMAT_WITH_DASHES);
}

String make_uuid_no_dashes() {
	return _uuid_to_string(_generate_uuid_v4(), FORMAT_NO_DASHES);
}

} // namespace sentry::uuid
