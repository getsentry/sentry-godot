#include "sentry_http_request.h"

#include "sentry/level.h"
#include "sentry/sentry_sdk.h"
#include "sentry/util/module_instance.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace sentry;

namespace {

struct HTTPRequestStringData {
	const String sentry_op{ "sentry.op" };
	const String sentry_origin{ "sentry.origin" };
	const String sentry_kind{ "sentry.kind" };
	const String http_request_method{ "http.request.method" };
	const String http_request_body_size{ "http.request.body.size" };
	const String http_response_status_code{ "http.response.status_code" };
	const String http_response_body_size{ "http.response.body.size" };
	const String http_response_body_decoded_size{ "http.response.body.decoded_size" };
	const String url_full{ "url.full" };
	const String url_domain{ "url.domain" };
	const String server_address{ "server.address" };
	const String server_port{ "server.port" };
	const String error_type{ "error.type" };
	const String url{ "url" };
	const String status_code{ "status_code" };
	const String http_client{ "http.client" };
	const String auto_http_godot{ "auto.http.godot" };
	const String client{ "client" };
	const String http{ "http" };
	const String cancelled{ "cancelled" };
	const String open_bracket{ "[" };
	const String close_bracket{ "]" };
	const StringName request_completed{ "request_completed" };

	const String method_get{ "GET" };
	const String method_head{ "HEAD" };
	const String method_post{ "POST" };
	const String method_put{ "PUT" };
	const String method_delete{ "DELETE" };
	const String method_options{ "OPTIONS" };
	const String method_trace{ "TRACE" };
	const String method_connect{ "CONNECT" };
	const String method_patch{ "PATCH" };
	const String method_unknown{ "UNKNOWN" };
};

using HTTPRequestStrings = util::ModuleInstance<HTTPRequestStringData>;

const String &_http_method(HTTPClient::Method p_method) {
	const auto &strings = HTTPRequestStrings::get();
	switch (p_method) {
		case HTTPClient::METHOD_GET:
			return strings.method_get;
		case HTTPClient::METHOD_HEAD:
			return strings.method_head;
		case HTTPClient::METHOD_POST:
			return strings.method_post;
		case HTTPClient::METHOD_PUT:
			return strings.method_put;
		case HTTPClient::METHOD_DELETE:
			return strings.method_delete;
		case HTTPClient::METHOD_OPTIONS:
			return strings.method_options;
		case HTTPClient::METHOD_TRACE:
			return strings.method_trace;
		case HTTPClient::METHOD_CONNECT:
			return strings.method_connect;
		case HTTPClient::METHOD_PATCH:
			return strings.method_patch;
		default:
			return strings.method_unknown;
	}
}

const char *_http_request_error(int64_t p_result) {
	switch (p_result) {
		case HTTPRequest::RESULT_CHUNKED_BODY_SIZE_MISMATCH:
			return "chunked_body_size_mismatch";
		case HTTPRequest::RESULT_CANT_CONNECT:
			return "cant_connect";
		case HTTPRequest::RESULT_CANT_RESOLVE:
			return "cant_resolve";
		case HTTPRequest::RESULT_CONNECTION_ERROR:
			return "connection_error";
		case HTTPRequest::RESULT_TLS_HANDSHAKE_ERROR:
			return "tls_handshake_error";
		case HTTPRequest::RESULT_NO_RESPONSE:
			return "no_response";
		case HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED:
			return "body_size_limit_exceeded";
		case HTTPRequest::RESULT_BODY_DECOMPRESS_FAILED:
			return "body_decompress_failed";
		case HTTPRequest::RESULT_REQUEST_FAILED:
			return "request_failed";
		case HTTPRequest::RESULT_DOWNLOAD_FILE_CANT_OPEN:
			return "download_file_cant_open";
		case HTTPRequest::RESULT_DOWNLOAD_FILE_WRITE_ERROR:
			return "download_file_write_error";
		case HTTPRequest::RESULT_REDIRECT_LIMIT_REACHED:
			return "redirect_limit_reached";
		case HTTPRequest::RESULT_TIMEOUT:
			return "timeout";
		default:
			return "unknown";
	}
}

Ref<SentrySpan> _start_http_span(const util::URLParts &p_url, HTTPClient::Method p_method, int64_t p_request_body_size) {
	const auto &strings = HTTPRequestStrings::get();
	const String redacted_url{ p_url.redacted() };
	const String &method_name = _http_method(p_method);

	// IPv6 addresses are enclosed in square brackets
	const String server_address = p_url.host.begins_with(strings.open_bracket) && p_url.host.ends_with(strings.close_bracket)
			? p_url.host.substr(1, p_url.host.length() - 2)
			: p_url.host;

	// https://github.com/getsentry/sentry-conventions/tree/main/model/attributes
	// https://develop.sentry.dev/sdk/telemetry/traces/span-data-conventions/#http
	// https://opentelemetry.io/docs/specs/semconv/registry/attributes/url/
	Dictionary attributes;
	attributes[strings.sentry_op] = strings.http_client;
	attributes[strings.sentry_origin] = strings.auto_http_godot;
	attributes[strings.sentry_kind] = strings.client;
	attributes[strings.http_request_method] = method_name;
	attributes[strings.http_request_body_size] = p_request_body_size;
	// TODO: Add `url.query`, `url.fragment`, and unredacted `url.full` once data
	//       collection options are implemented.
	//       Omit them for now to avoid including potentially sensitive URL components.
	attributes[strings.url_full] = redacted_url;
	attributes[strings.url_domain] = p_url.host; // with IPv6 brackets?
	attributes[strings.server_address] = server_address; // without IPv6 brackets?
	if (p_url.port >= 0) {
		attributes[strings.server_port] = p_url.port;
	}

	String span_name{ method_name };
	span_name += U' ';
	span_name += redacted_url;

	return SentrySDK::get_singleton()->start_span(span_name, attributes,
			SentrySDK::get_singleton()->get_active_span());
}

PackedStringArray _apply_headers(const Ref<SentrySpan> &p_span, const String &p_redacted_url, const PackedStringArray &p_custom_headers) {
	PackedStringArray headers = p_span->get_trace_headers(p_redacted_url);
	for (const String &header : p_custom_headers) {
		const String header_name = header.get_slicec(U':', 0).strip_edges().to_lower();
		bool already_present = false;
		for (const String &existing_header : headers) {
			const String existing_header_name = existing_header.get_slicec(U':', 0).strip_edges().to_lower();
			if (existing_header_name == header_name) {
				already_present = true;
				break;
			}
		}
		if (!already_present) {
			headers.push_back(header);
		}
	}
	return headers;
}

void _add_http_breadcrumb(const sentry::Level p_level, const Dictionary &p_data) {
	const auto &strings = HTTPRequestStrings::get();
	Ref<SentryBreadcrumb> crumb = SentryBreadcrumb::create();
	crumb->set_type(strings.http);
	crumb->set_category(strings.http);
	crumb->set_level(p_level);
	crumb->set_data(p_data);
	SentrySDK::get_singleton()->add_breadcrumb(crumb);
}

} // unnamed namespace

namespace sentry {

Dictionary SentryHTTPRequest::RequestData::as_breadcrumb_data() const {
	const auto &strings = HTTPRequestStrings::get();
	Dictionary data;
	data[strings.url] = parsed_url.redacted();
	data[strings.http_request_method] = _http_method(method);
	// TODO: Add `http.query` and `http.fragment` once data collection options are implemented.
	//       Omit them for now to avoid including potentially sensitive URL components.
	if (request_body_size > 0) {
		data[strings.http_request_body_size] = request_body_size;
	}
	return data;
}

Error SentryHTTPRequest::request(const String &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, const String &p_request_data) {
	const PackedByteArray raw_data = p_request_data.to_utf8_buffer();
	return request_raw(p_url, p_custom_headers, p_method, raw_data);
}

Error SentryHTTPRequest::request_raw(const String &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, const PackedByteArray &p_request_data_raw) {
	ERR_FAIL_COND_V(!_http_request->is_inside_tree(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(_request_in_progress, ERR_BUSY, "SentryHTTPRequest is processing a request. Wait for completion or cancel it before attempting a new one.");

	util::URLParts parsed_url;
	Error err = util::parse_url(p_url, parsed_url);
	if (err != OK) {
		return err;
	}

	_request_in_progress = true;
	const PackedStringArray headers = _instrument_request(parsed_url, p_custom_headers, p_method, p_request_data_raw.size());
	err = _http_request->request_raw(p_url, headers, p_method, p_request_data_raw);
	// ERR_CANT_CONNECT still schedules request_completed in Godot.
	if (err != OK && err != ERR_CANT_CONNECT) {
		_finalize_request(RequestOutcome::startup_failure(err));
	}
	return err;
}

void SentryHTTPRequest::cancel_request() {
	_http_request->cancel_request();
	_finalize_request(RequestOutcome::cancelled());
}

void SentryHTTPRequest::_request_completed(int64_t p_result, int64_t p_response_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	_finalize_request(RequestOutcome::completed(p_result, p_response_code, _http_request->get_downloaded_bytes()));

	emit_signal(HTTPRequestStrings::get().request_completed, p_result, p_response_code, p_headers, p_body);
}

PackedStringArray SentryHTTPRequest::_instrument_request(const util::URLParts &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, int64_t p_request_body_size) {
	_span = _start_http_span(p_url, p_method, p_request_body_size);

	_request_data.request_body_size = p_request_body_size;
	_request_data.method = p_method;
	_request_data.parsed_url = p_url;

	const PackedStringArray headers = _apply_headers(_span, p_url.redacted(), p_custom_headers);
	return headers;
}

void SentryHTTPRequest::_finalize_request(const RequestOutcome &p_outcome) {
	if (!_request_in_progress) {
		return;
	}

	const auto &strings = HTTPRequestStrings::get();
	String error_type;
	SpanStatus status = SPAN_STATUS_OK;
	Level breadcrumb_level = LEVEL_INFO;
	switch (p_outcome.kind) {
		case RequestOutcome::Kind::CANCELLED: {
			error_type = strings.cancelled;
			status = SPAN_STATUS_ERROR;
			breadcrumb_level = LEVEL_WARNING;
		} break;
		case RequestOutcome::Kind::STARTUP_FAILURE: {
			error_type = UtilityFunctions::error_string(p_outcome.startup_error);
			status = SPAN_STATUS_ERROR;
			breadcrumb_level = LEVEL_ERROR;
		} break;
		case RequestOutcome::Kind::COMPLETED: {
			if (p_outcome.result != RESULT_SUCCESS) {
				error_type = _http_request_error(p_outcome.result);
			}
			if (p_outcome.result != RESULT_SUCCESS || p_outcome.response_code >= 400) {
				status = SPAN_STATUS_ERROR;
				breadcrumb_level = LEVEL_ERROR;
			}
		} break;
	}

	if (_span.is_valid()) {
		_span->set_status(status);
		if (!error_type.is_empty()) {
			_span->set_attribute(strings.error_type, error_type);
		}
		if (p_outcome.response_code >= 0) {
			_span->set_attribute(strings.http_response_status_code, p_outcome.response_code);
		}
		if (p_outcome.response_body_size >= 0) {
#ifdef WEB_ENABLED
			// On Web, fetch exposes decoded response chunks, so Godot reports decoded bytes.
			_span->set_attribute(strings.http_response_body_decoded_size, p_outcome.response_body_size);
#else
			// On other platforms, Godot reports bytes downloaded before decompression.
			_span->set_attribute(strings.http_response_body_size, p_outcome.response_body_size);
#endif
		}
		_span->end();
	}

	Dictionary breadcrumb_data = _request_data.as_breadcrumb_data();
	if (!error_type.is_empty()) {
		breadcrumb_data[strings.error_type] = error_type;
	}
	if (p_outcome.response_code > 0) {
		breadcrumb_data[strings.status_code] = p_outcome.response_code;
	}
	_add_http_breadcrumb(breadcrumb_level, breadcrumb_data);

	_span.unref();
	_request_data = {};
	_request_in_progress = false;
}

void SentryHTTPRequest::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_http_request->connect(HTTPRequestStrings::get().request_completed, callable_mp(this, &SentryHTTPRequest::_request_completed));
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		_finalize_request(RequestOutcome::cancelled());
	}
}

void SentryHTTPRequest::_bind_methods() {
	HTTPRequestStrings::create_once();

	ClassDB::bind_method(D_METHOD("request", "url", "custom_headers", "method", "request_data"), &SentryHTTPRequest::request, DEFVAL(PackedStringArray()), DEFVAL(HTTPClient::METHOD_GET), DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("request_raw", "url", "custom_headers", "method", "request_data_raw"), &SentryHTTPRequest::request_raw, DEFVAL(PackedStringArray()), DEFVAL(HTTPClient::METHOD_GET), DEFVAL(PackedByteArray()));
	ClassDB::bind_method(D_METHOD("cancel_request"), &SentryHTTPRequest::cancel_request);
	ClassDB::bind_method(D_METHOD("set_tls_options", "client_options"), &SentryHTTPRequest::set_tls_options);
	ClassDB::bind_method(D_METHOD("get_http_client_status"), &SentryHTTPRequest::get_http_client_status);
	ClassDB::bind_method(D_METHOD("set_use_threads", "enable"), &SentryHTTPRequest::set_use_threads);
	ClassDB::bind_method(D_METHOD("is_using_threads"), &SentryHTTPRequest::is_using_threads);
	ClassDB::bind_method(D_METHOD("set_accept_gzip", "enable"), &SentryHTTPRequest::set_accept_gzip);
	ClassDB::bind_method(D_METHOD("is_accepting_gzip"), &SentryHTTPRequest::is_accepting_gzip);
	ClassDB::bind_method(D_METHOD("set_body_size_limit", "bytes"), &SentryHTTPRequest::set_body_size_limit);
	ClassDB::bind_method(D_METHOD("get_body_size_limit"), &SentryHTTPRequest::get_body_size_limit);
	ClassDB::bind_method(D_METHOD("set_max_redirects", "amount"), &SentryHTTPRequest::set_max_redirects);
	ClassDB::bind_method(D_METHOD("get_max_redirects"), &SentryHTTPRequest::get_max_redirects);
	ClassDB::bind_method(D_METHOD("set_download_file", "path"), &SentryHTTPRequest::set_download_file);
	ClassDB::bind_method(D_METHOD("get_download_file"), &SentryHTTPRequest::get_download_file);
	ClassDB::bind_method(D_METHOD("get_downloaded_bytes"), &SentryHTTPRequest::get_downloaded_bytes);
	ClassDB::bind_method(D_METHOD("get_body_size"), &SentryHTTPRequest::get_body_size);
	ClassDB::bind_method(D_METHOD("set_timeout", "timeout"), &SentryHTTPRequest::set_timeout);
	ClassDB::bind_method(D_METHOD("get_timeout"), &SentryHTTPRequest::get_timeout);
	ClassDB::bind_method(D_METHOD("set_download_chunk_size", "chunk_size"), &SentryHTTPRequest::set_download_chunk_size);
	ClassDB::bind_method(D_METHOD("get_download_chunk_size"), &SentryHTTPRequest::get_download_chunk_size);
	ClassDB::bind_method(D_METHOD("set_http_proxy", "host", "port"), &SentryHTTPRequest::set_http_proxy);
	ClassDB::bind_method(D_METHOD("set_https_proxy", "host", "port"), &SentryHTTPRequest::set_https_proxy);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "download_file", PROPERTY_HINT_FILE_PATH), "set_download_file", "get_download_file");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "download_chunk_size", PROPERTY_HINT_RANGE, "256,16777216,suffix:B"), "set_download_chunk_size", "get_download_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_threads"), "set_use_threads", "is_using_threads");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "accept_gzip"), "set_accept_gzip", "is_accepting_gzip");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "body_size_limit", PROPERTY_HINT_RANGE, "-1,2000000000,suffix:B"), "set_body_size_limit", "get_body_size_limit");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_redirects", PROPERTY_HINT_RANGE, "-1,64"), "set_max_redirects", "get_max_redirects");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "timeout", PROPERTY_HINT_RANGE, "0,3600,0.1,or_greater,suffix:s"), "set_timeout", "get_timeout");

	ADD_SIGNAL(MethodInfo(HTTPRequestStrings::get().request_completed, PropertyInfo(Variant::INT, "result"), PropertyInfo(Variant::INT, "response_code"), PropertyInfo(Variant::PACKED_STRING_ARRAY, "headers"), PropertyInfo(Variant::PACKED_BYTE_ARRAY, "body")));

	BIND_ENUM_CONSTANT(RESULT_SUCCESS);
	BIND_ENUM_CONSTANT(RESULT_CHUNKED_BODY_SIZE_MISMATCH);
	BIND_ENUM_CONSTANT(RESULT_CANT_CONNECT);
	BIND_ENUM_CONSTANT(RESULT_CANT_RESOLVE);
	BIND_ENUM_CONSTANT(RESULT_CONNECTION_ERROR);
	BIND_ENUM_CONSTANT(RESULT_TLS_HANDSHAKE_ERROR);
	BIND_ENUM_CONSTANT(RESULT_NO_RESPONSE);
	BIND_ENUM_CONSTANT(RESULT_BODY_SIZE_LIMIT_EXCEEDED);
	BIND_ENUM_CONSTANT(RESULT_BODY_DECOMPRESS_FAILED);
	BIND_ENUM_CONSTANT(RESULT_REQUEST_FAILED);
	BIND_ENUM_CONSTANT(RESULT_DOWNLOAD_FILE_CANT_OPEN);
	BIND_ENUM_CONSTANT(RESULT_DOWNLOAD_FILE_WRITE_ERROR);
	BIND_ENUM_CONSTANT(RESULT_REDIRECT_LIMIT_REACHED);
	BIND_ENUM_CONSTANT(RESULT_TIMEOUT);
}

SentryHTTPRequest::SentryHTTPRequest() {
	_http_request = memnew(HTTPRequest);
	add_child(_http_request, false, INTERNAL_MODE_BACK);
}

} //namespace sentry
