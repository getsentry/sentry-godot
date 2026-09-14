#include "sentry_http_request.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace sentry {

void SentryHTTPRequest::_on_request_completed(int64_t p_result, int64_t p_response_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	emit_signal("request_completed", p_result, p_response_code, p_headers, p_body);
}

Error SentryHTTPRequest::request(const String &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, const String &p_request_data) {
	return _http_request->request(p_url, p_custom_headers, p_method, p_request_data);
}

Error SentryHTTPRequest::request_raw(const String &p_url, const PackedStringArray &p_custom_headers, HTTPClient::Method p_method, const PackedByteArray &p_request_data_raw) {
	return _http_request->request_raw(p_url, p_custom_headers, p_method, p_request_data_raw);
}

void SentryHTTPRequest::cancel_request() {
	_http_request->cancel_request();
}

void SentryHTTPRequest::set_tls_options(const Ref<TLSOptions> &p_client_options) {
	_http_request->set_tls_options(p_client_options);
}

HTTPClient::Status SentryHTTPRequest::get_http_client_status() const {
	return _http_request->get_http_client_status();
}

void SentryHTTPRequest::set_use_threads(bool p_enable) {
	_http_request->set_use_threads(p_enable);
}

bool SentryHTTPRequest::is_using_threads() const {
	return _http_request->is_using_threads();
}

void SentryHTTPRequest::set_accept_gzip(bool p_enable) {
	_http_request->set_accept_gzip(p_enable);
}

bool SentryHTTPRequest::is_accepting_gzip() const {
	return _http_request->is_accepting_gzip();
}

void SentryHTTPRequest::set_body_size_limit(int32_t p_bytes) {
	_http_request->set_body_size_limit(p_bytes);
}

int32_t SentryHTTPRequest::get_body_size_limit() const {
	return _http_request->get_body_size_limit();
}

void SentryHTTPRequest::set_max_redirects(int32_t p_amount) {
	_http_request->set_max_redirects(p_amount);
}

int32_t SentryHTTPRequest::get_max_redirects() const {
	return _http_request->get_max_redirects();
}

void SentryHTTPRequest::set_download_file(const String &p_path) {
	_http_request->set_download_file(p_path);
}

String SentryHTTPRequest::get_download_file() const {
	return _http_request->get_download_file();
}

int32_t SentryHTTPRequest::get_downloaded_bytes() const {
	return _http_request->get_downloaded_bytes();
}

int32_t SentryHTTPRequest::get_body_size() const {
	return _http_request->get_body_size();
}

void SentryHTTPRequest::set_timeout(double p_timeout) {
	_http_request->set_timeout(p_timeout);
}

double SentryHTTPRequest::get_timeout() {
	return _http_request->get_timeout();
}

void SentryHTTPRequest::set_download_chunk_size(int32_t p_chunk_size) {
	_http_request->set_download_chunk_size(p_chunk_size);
}

int32_t SentryHTTPRequest::get_download_chunk_size() const {
	return _http_request->get_download_chunk_size();
}

void SentryHTTPRequest::set_http_proxy(const String &p_host, int32_t p_port) {
	_http_request->set_http_proxy(p_host, p_port);
}

void SentryHTTPRequest::set_https_proxy(const String &p_host, int32_t p_port) {
	_http_request->set_https_proxy(p_host, p_port);
}

void SentryHTTPRequest::_ready() {
	_http_request->connect("request_completed", callable_mp(this, &SentryHTTPRequest::_on_request_completed));
}

void SentryHTTPRequest::_bind_methods() {
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

	ADD_SIGNAL(MethodInfo("request_completed", PropertyInfo(Variant::INT, "result"), PropertyInfo(Variant::INT, "response_code"), PropertyInfo(Variant::PACKED_STRING_ARRAY, "headers"), PropertyInfo(Variant::PACKED_BYTE_ARRAY, "body")));

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
