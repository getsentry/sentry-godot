#ifdef TESTS_ENABLED

/// Exercises SentryHTTPRequest's internal request and span lifecycle.
/// Uses synthetic completions to assert state transitions and span data directly.
/// Complements project/test/isolated/test_http_request.gd, which covers public API and wire behavior.

#include "cpp_test_helpers.h"
#include "sentry/sentry_http_request.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/tcp_server.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <memory>
#include <vector>

using namespace godot;
using namespace sentry;

namespace {

struct SpanRecord {
	String name;
	Dictionary attributes;
	SpanStatus status = SPAN_STATUS_OK;
	int end_count = 0;
	std::vector<std::shared_ptr<SpanRecord>> children;
};

class RecordingSpan : public SentrySpanImpl {
	std::shared_ptr<SpanRecord> _record;

public:
	explicit RecordingSpan(const std::shared_ptr<SpanRecord> &p_record) :
			_record(p_record) {}

	void set_attribute(const String &p_key, const Variant &p_value) override { _record->attributes[p_key] = p_value; }
	void set_status(SpanStatus p_status) override { _record->status = p_status; }
	void end() override { ++_record->end_count; }
	PackedStringArray get_trace_headers() override { return {}; }

	SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override {
		auto child = std::make_shared<SpanRecord>();
		child->name = p_name;
		child->attributes = p_attributes.duplicate();
		_record->children.push_back(child);
		return memnew(RecordingSpan(child));
	}
};

void _restart_request(int64_t, int64_t, const PackedStringArray &, const PackedByteArray &, SentryHTTPRequest *p_request, const String &p_url) {
	p_request->set_meta("restart_error", p_request->request(p_url));
}

// Gives tests deterministic control over request completion. The server listens without
// accepting or responding, so requests remain pending instead of failing.
// Tests then cancel the inner HTTPRequest and emit request_completed themselves.
struct RequestFixture {
	SentryHTTPRequest *request = memnew(SentryHTTPRequest);
	Ref<TCPServer> server;

	RequestFixture() {
		server.instantiate();
	}

	~RequestFixture() {
		request->cancel_request();
		memdelete(request);
	}

	void add_to_tree() {
		SceneTree::get_singleton()->get_root()->add_child(request);
	}

	String url(const String &p_path = "/") const {
		return "http://127.0.0.1:" + itos(server->get_local_port()) + p_path;
	}

	HTTPRequest *inner_request() const {
		return Object::cast_to<HTTPRequest>(request->get_child(0, true));
	}

	bool complete_request(int64_t p_result = HTTPRequest::RESULT_SUCCESS, int64_t p_response_code = 200) {
		HTTPRequest *inner = inner_request();
		if (inner == nullptr) {
			return false;
		}
		inner->cancel_request();
		inner->emit_signal("request_completed", p_result, p_response_code, PackedStringArray(), PackedByteArray());
		return true;
	}
};

// Installs a recording parent span for direct assertions on child-span state and data.
struct InstrumentedRequestFixture : RequestFixture {
	const std::shared_ptr<SpanRecord> parent_record = std::make_shared<SpanRecord>();
	Ref<SentrySpan> parent_span;

	InstrumentedRequestFixture() {
		parent_span = Ref<SentrySpan>(memnew(SentrySpan(memnew(RecordingSpan(parent_record)))));
		SentrySDK::get_singleton()->get_current_scope()->set_span(parent_span);
		add_to_tree();
	}

	~InstrumentedRequestFixture() {
		request->cancel_request();
		SentrySDK::get_singleton()->get_current_scope()->clear();
		parent_span->end();
	}
};

} // unnamed namespace

TEST_SUITE("HTTP request lifecycle") {
	TEST_CASE("Overlapping requests do not replace the in-flight span") {
		InstrumentedRequestFixture fixture;
		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String url = fixture.url("/first");
		REQUIRED_CHECK(fixture.request->request_raw(url) == OK);
		REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
		const std::shared_ptr<SpanRecord> span_record = fixture.parent_record->children.front();

		CHECK(fixture.request->request(url + String("/second")) == ERR_BUSY);
		CHECK(fixture.request->request_raw("http://example.com:invalid/") == ERR_BUSY);
		CHECK(fixture.parent_record->children.size() == 1);
		CHECK(span_record->end_count == 0);

		fixture.request->cancel_request();
		CHECK(span_record->end_count == 1);
		CHECK(fixture.request->request(url) == OK);
		CHECK(fixture.parent_record->children.size() == 2);
	}

	TEST_CASE("Completion finishes the old span before a signal handler starts another request") {
		for (int64_t result : { HTTPRequest::RESULT_SUCCESS, HTTPRequest::RESULT_CANT_CONNECT }) {
			CAPTURE(result);
			InstrumentedRequestFixture fixture;
			REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
			const String url = fixture.url();
			REQUIRED_CHECK(fixture.request->request(url) == OK);
			REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
			const std::shared_ptr<SpanRecord> span_record = fixture.parent_record->children.front();
			fixture.request->connect("request_completed", callable_mp_static(_restart_request).bind(fixture.request, url), Object::CONNECT_ONE_SHOT);

			REQUIRED_CHECK(fixture.complete_request(result, result == HTTPRequest::RESULT_SUCCESS ? 200 : 0));

			CHECK(span_record->end_count == 1);
			CHECK(int64_t(fixture.request->get_meta("restart_error")) == OK);
			CHECK(fixture.parent_record->children.size() == 2);
		}
	}

	TEST_CASE("Rejected startup does not leave the request busy") {
		RequestFixture fixture;
		CHECK(fixture.request->request("http://127.0.0.1/") == ERR_UNCONFIGURED);

		fixture.add_to_tree();
		CHECK(fixture.request->request("http://example.com:invalid/") == ERR_INVALID_DATA);
		CHECK(fixture.request->request("ftp://127.0.0.1/") == ERR_INVALID_PARAMETER);

		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String url = fixture.url();
		CHECK(fixture.request->request(url) == OK);
	}
}

TEST_SUITE("HTTP request instrumentation") {
	TEST_CASE("HTTP span records request and response facts with a redacted URL") {
		InstrumentedRequestFixture fixture;
		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String base_url = fixture.url("");
		const String request_url = base_url + String("/scores?token=secret#private");
		const String redacted_url = base_url + String("/scores");
		const String request_body = String::utf8("Hello 世界! 👋");

		REQUIRED_CHECK(fixture.request->request(request_url, {}, HTTPClient::METHOD_POST, request_body) == OK);
		REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
		const std::shared_ptr<SpanRecord> span = fixture.parent_record->children.front();
		CHECK(span->name == "POST " + redacted_url);
		CHECK(String(span->attributes["sentry.op"]) == "http.client");
		CHECK(String(span->attributes["sentry.origin"]) == "auto.http.godot");
		CHECK(String(span->attributes["sentry.kind"]) == "client");
		CHECK(String(span->attributes["http.request.method"]) == "POST");
		CHECK(int64_t(span->attributes["http.request.body.size"]) == request_body.to_utf8_buffer().size());
		CHECK(String(span->attributes["url.full"]) == redacted_url);
		CHECK(String(span->attributes["url.domain"]) == "127.0.0.1");
		CHECK(String(span->attributes["server.address"]) == "127.0.0.1");
		CHECK(int64_t(span->attributes["server.port"]) == fixture.server->get_local_port());
		CHECK(span->end_count == 0);
		CHECK(SentrySDK::get_singleton()->get_active_span() == fixture.parent_span);

		REQUIRED_CHECK(fixture.complete_request());
		CHECK(span->status == SPAN_STATUS_OK);
		CHECK(int64_t(span->attributes["http.response.status_code"]) == 200);
		CHECK(int64_t(span->attributes["http.response.body.size"]) == 0);
		CHECK(span->end_count == 1);
		CHECK(SentrySDK::get_singleton()->get_active_span() == fixture.parent_span);
	}

	TEST_CASE("HTTP error response records its status code without an error type") {
		InstrumentedRequestFixture fixture;
		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String url = fixture.url("/status");

		REQUIRED_CHECK(fixture.request->request(url) == OK);
		REQUIRED_CHECK(fixture.complete_request(HTTPRequest::RESULT_SUCCESS, 404));
		REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
		const std::shared_ptr<SpanRecord> http_error = fixture.parent_record->children.front();
		CHECK(http_error->status == SPAN_STATUS_ERROR);
		CHECK(int64_t(http_error->attributes["http.response.status_code"]) == 404);
		CHECK_FALSE(http_error->attributes.has("error.type"));
	}

	TEST_CASE("Transport failure records its error type without response facts") {
		InstrumentedRequestFixture fixture;
		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String url = fixture.url("/status");

		REQUIRED_CHECK(fixture.request->request(url) == OK);
		REQUIRED_CHECK(fixture.complete_request(HTTPRequest::RESULT_TIMEOUT, 0));
		REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
		const std::shared_ptr<SpanRecord> transport_error = fixture.parent_record->children.front();
		CHECK(transport_error->status == SPAN_STATUS_ERROR);
		CHECK(String(transport_error->attributes["error.type"]) == "timeout");
		CHECK_FALSE(transport_error->attributes.has("http.response.status_code"));
		CHECK_FALSE(transport_error->attributes.has("http.response.body.size"));
	}

	TEST_CASE("Cancellation finalizes each span only once") {
		InstrumentedRequestFixture fixture;
		REQUIRED_CHECK(fixture.server->listen(0, "127.0.0.1") == OK);
		const String url = fixture.url("/pending");

		REQUIRED_CHECK(fixture.request->request(url) == OK);
		fixture.request->cancel_request();
		fixture.request->cancel_request();
		REQUIRED_CHECK(fixture.parent_record->children.size() == 1);
		const std::shared_ptr<SpanRecord> cancelled = fixture.parent_record->children[0];
		CHECK(cancelled->status == SPAN_STATUS_ERROR);
		CHECK(String(cancelled->attributes["error.type"]) == "cancelled");
		CHECK(cancelled->end_count == 1);
	}
}

#endif // TESTS_ENABLED
