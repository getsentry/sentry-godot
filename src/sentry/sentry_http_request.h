#pragma once

#include "sentry/sentry_span.h"
#include "sentry/util/url.h"

#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

// This node mirrors HTTPRequest's public API but is not an HTTPRequest subtype.
class SentryHTTPRequest : public Node {
	GDCLASS(SentryHTTPRequest, Node);

public:
	enum Result {
		RESULT_SUCCESS = HTTPRequest::RESULT_SUCCESS,
		RESULT_CHUNKED_BODY_SIZE_MISMATCH = HTTPRequest::RESULT_CHUNKED_BODY_SIZE_MISMATCH,
		RESULT_CANT_CONNECT = HTTPRequest::RESULT_CANT_CONNECT,
		RESULT_CANT_RESOLVE = HTTPRequest::RESULT_CANT_RESOLVE,
		RESULT_CONNECTION_ERROR = HTTPRequest::RESULT_CONNECTION_ERROR,
		RESULT_TLS_HANDSHAKE_ERROR = HTTPRequest::RESULT_TLS_HANDSHAKE_ERROR,
		RESULT_NO_RESPONSE = HTTPRequest::RESULT_NO_RESPONSE,
		RESULT_BODY_SIZE_LIMIT_EXCEEDED = HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED,
		RESULT_BODY_DECOMPRESS_FAILED = HTTPRequest::RESULT_BODY_DECOMPRESS_FAILED,
		RESULT_REQUEST_FAILED = HTTPRequest::RESULT_REQUEST_FAILED,
		RESULT_DOWNLOAD_FILE_CANT_OPEN = HTTPRequest::RESULT_DOWNLOAD_FILE_CANT_OPEN,
		RESULT_DOWNLOAD_FILE_WRITE_ERROR = HTTPRequest::RESULT_DOWNLOAD_FILE_WRITE_ERROR,
		RESULT_REDIRECT_LIMIT_REACHED = HTTPRequest::RESULT_REDIRECT_LIMIT_REACHED,
		RESULT_TIMEOUT = HTTPRequest::RESULT_TIMEOUT,
	};

private:
	struct RequestData {
		util::URLParts parsed_url;
		HTTPClient::Method method = HTTPClient::METHOD_GET;
		int64_t request_body_size = 0;

		Dictionary as_breadcrumb_data() const;
	};

	struct RequestOutcome {
		enum class Kind {
			CANCELLED,
			STARTUP_FAILURE,
			COMPLETED,
		};

		Kind kind;
		Error startup_error = OK;
		int64_t result = RESULT_SUCCESS;
		int64_t response_code = -1;
		int64_t response_body_size = -1;

		static RequestOutcome cancelled() { return { Kind::CANCELLED }; }
		static RequestOutcome startup_failure(Error p_error) { return { Kind::STARTUP_FAILURE, p_error }; }
		static RequestOutcome completed(int64_t p_result, int64_t p_response_code, int64_t p_response_body_size) {
			return { Kind::COMPLETED, OK, p_result,
				p_response_code > 0 ? p_response_code : -1,
				p_result == RESULT_SUCCESS ? p_response_body_size : -1 };
		}
	};

	HTTPRequest *_http_request = nullptr;
	Ref<SentrySpan> _span;
	bool _request_in_progress = false;
	RequestData _request_data;

	PackedStringArray _instrument_request(const util::URLParts &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, int64_t p_request_body_size);
	void _finalize_request(const RequestOutcome &p_outcome);
	void _request_completed(int64_t p_result, int64_t p_response_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	Error request(const String &p_url, const PackedStringArray &p_custom_headers = PackedStringArray(), HTTPClient::Method p_method = HTTPClient::METHOD_GET, const String &p_request_data = String());
	Error request_raw(const String &p_url, const PackedStringArray &p_custom_headers = PackedStringArray(), HTTPClient::Method p_method = HTTPClient::METHOD_GET, const PackedByteArray &p_request_data_raw = PackedByteArray());
	void cancel_request();

	void set_tls_options(const Ref<TLSOptions> &p_client_options) { _http_request->set_tls_options(p_client_options); }
	HTTPClient::Status get_http_client_status() const { return _http_request->get_http_client_status(); }
	void set_use_threads(bool p_enable) { _http_request->set_use_threads(p_enable); }
	bool is_using_threads() const { return _http_request->is_using_threads(); }
	void set_accept_gzip(bool p_enable) { _http_request->set_accept_gzip(p_enable); }
	bool is_accepting_gzip() const { return _http_request->is_accepting_gzip(); }
	void set_body_size_limit(int32_t p_bytes) { _http_request->set_body_size_limit(p_bytes); }
	int32_t get_body_size_limit() const { return _http_request->get_body_size_limit(); }
	void set_max_redirects(int32_t p_amount) { _http_request->set_max_redirects(p_amount); }
	int32_t get_max_redirects() const { return _http_request->get_max_redirects(); }
	void set_download_file(const String &p_path) { _http_request->set_download_file(p_path); }
	String get_download_file() const { return _http_request->get_download_file(); }
	int32_t get_downloaded_bytes() const { return _http_request->get_downloaded_bytes(); }
	int32_t get_body_size() const { return _http_request->get_body_size(); }
	void set_timeout(double p_timeout) { _http_request->set_timeout(p_timeout); }
	double get_timeout() { return _http_request->get_timeout(); }
	void set_download_chunk_size(int32_t p_chunk_size) { _http_request->set_download_chunk_size(p_chunk_size); }
	int32_t get_download_chunk_size() const { return _http_request->get_download_chunk_size(); }
	void set_http_proxy(const String &p_host, int32_t p_port) { _http_request->set_http_proxy(p_host, p_port); }
	void set_https_proxy(const String &p_host, int32_t p_port) { _http_request->set_https_proxy(p_host, p_port); }

	SentryHTTPRequest();
};

} //namespace sentry

VARIANT_ENUM_CAST(sentry::SentryHTTPRequest::Result);
