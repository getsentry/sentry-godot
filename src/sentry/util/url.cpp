#include "sentry/util/url.h"
#include "sentry/util/text.h"

using namespace sentry::util;

namespace {

// https://www.rfc-editor.org/info/rfc3986/#section-3.1
bool _has_scheme(Span<char32_t> p_url, int64_t p_scheme_end) {
	if (p_scheme_end <= 0 || !is_alpha_ascii(p_url[0])) {
		return false;
	}
	for (int64_t i = 1; i < p_scheme_end; i++) {
		const char32_t c = p_url[i];
		if (!is_alpha_ascii(c) && !is_numeric_ascii(c) && c != '+' && c != '-' && c != '.') {
			return false;
		}
	}
	return true;
}

// https://www.rfc-editor.org/info/rfc3986/#section-3.2.3
Error _parse_port(Span<char32_t> p_port, int32_t &r_port) {
	if (p_port.is_empty()) {
		r_port = -1;
		return OK;
	}

	int32_t port = 0;
	for (const char32_t c : p_port) {
		if (!is_numeric_ascii(c)) {
			return ERR_INVALID_DATA;
		}
		port = port * 10 + (c - '0');
		if (port > 65535) {
			return ERR_INVALID_DATA;
		}
	}
	r_port = port;
	return OK;
}

// Splits "host:port", "[::1]:port", or a bare host, and discards any "user:password@" prefix.
// https://www.rfc-editor.org/info/rfc3986/#section-3.2
Error _split_authority(Span<char32_t> p_authority, Span<char32_t> &r_host, int32_t &r_port) {
	Span<char32_t> authority = p_authority;
	const int64_t credentials_end = authority.is_empty() ? -1 : authority.rfind(U'@');
	if (credentials_end >= 0) {
		authority = Span<char32_t>(authority.ptr() + credentials_end + 1, authority.size() - credentials_end - 1);
	}
	if (authority.is_empty()) {
		r_host = authority;
		r_port = -1;
		return OK;
	}

	int64_t port_start = -1;
	if (authority[0] == U'[') {
		const int64_t bracket_end = authority.find(U']');
		if (bracket_end < 0 || authority.find(U'[', 1) >= 0 || authority.find(U']', bracket_end + 1) >= 0) {
			return ERR_INVALID_DATA;
		}
		const uint64_t host_end = bracket_end + 1;
		if (host_end < authority.size()) {
			if (authority[host_end] != U':') {
				return ERR_INVALID_DATA;
			}
			port_start = host_end;
		}
	} else {
		if (authority.find(U'[') >= 0 || authority.find(U']') >= 0) {
			return ERR_INVALID_DATA;
		}
		port_start = authority.rfind(U':');
		if (port_start >= 0 && authority.find(U':') != port_start) {
			return ERR_INVALID_DATA;
		}
	}

	int32_t port = -1;
	if (port_start >= 0) {
		const Span<char32_t> port_text(authority.ptr() + port_start + 1, authority.size() - port_start - 1);
		const Error error = _parse_port(port_text, port);
		if (error != OK) {
			return error;
		}
		authority = Span<char32_t>(authority.ptr(), port_start);
	}
	r_host = authority;
	r_port = port;
	return OK;
}

} // unnamed namespace

namespace sentry::util {

String URLParts::redacted() const {
	String result;
	if (!scheme.is_empty()) {
		result = scheme + "://";
	} else if (has_authority) {
		result = "//";
	}
	result += host;
	if (port >= 0) {
		result += ":" + itos(port);
	}
	return result + path;
}

Error parse_url(const String &p_url, URLParts &r_parts) {
	URLParts parts;
	Span<char32_t> rest(p_url.ptr(), p_url.length());
	Span<char32_t> scheme;
	Span<char32_t> host;
	Span<char32_t> path;
	Span<char32_t> query;
	Span<char32_t> fragment;

	const int64_t scheme_end = rest.size() >= 3 ? rest.find_sequence(Span<char32_t>(U"://")) : -1;
	if (_has_scheme(rest, scheme_end)) {
		parts.has_authority = true;
		scheme = Span<char32_t>(rest.ptr(), scheme_end);
		rest = Span<char32_t>(rest.ptr() + scheme_end + 3, rest.size() - scheme_end - 3);
	} else if (rest.size() >= 2 && rest[0] == U'/' && rest[1] == U'/') {
		parts.has_authority = true;
		rest = Span<char32_t>(rest.ptr() + 2, rest.size() - 2);
	}

	const int64_t fragment_start = rest.find(U'#');
	if (fragment_start >= 0) {
		fragment = Span<char32_t>(rest.ptr() + fragment_start + 1, rest.size() - fragment_start - 1);
		rest = Span<char32_t>(rest.ptr(), fragment_start);
	}

	const int64_t query_start = rest.find(U'?');
	if (query_start >= 0) {
		query = Span<char32_t>(rest.ptr() + query_start + 1, rest.size() - query_start - 1);
		rest = Span<char32_t>(rest.ptr(), query_start);
	}

	if (!parts.has_authority) {
		path = rest;
	} else {
		const int64_t path_start = rest.find(U'/');
		if (path_start >= 0) {
			path = Span<char32_t>(rest.ptr() + path_start, rest.size() - path_start);
			rest = Span<char32_t>(rest.ptr(), path_start);
		}
		const Error error = _split_authority(rest, host, parts.port);
		if (error != OK) {
			return error;
		}
	}

	parts.scheme = string_from_span(scheme).to_lower();
	parts.host = string_from_span(host);
	parts.path = string_from_span(path);
	parts.query = string_from_span(query);
	parts.fragment = string_from_span(fragment);
	r_parts = parts;
	return OK;
}

} // namespace sentry::util
