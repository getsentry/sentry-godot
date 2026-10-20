#if defined(TESTS_ENABLED) && defined(SDK_ANDROID)

#include "cpp_test_helpers.h"
#include "sentry/android/android_breadcrumb.h"
#include "sentry/android/android_event.h"
#include "sentry/android/android_log.h"
#include "sentry/android/android_metric.h"
#include "sentry/android/android_scope.h"
#include "sentry/android/android_span.h"

#include <godot_cpp/classes/gd_script.hpp>

using namespace godot;
using namespace sentry;
using namespace sentry::android;
using sentry::util::ObjectHandle;

namespace {

Object *_create_mock_plugin(const Array &p_calls) {
	Ref<GDScript> script;
	script.instantiate();
	script->set_source_code(R"(extends Object
var calls: Array = []

func releaseScope(handle: int) -> void:
	calls.append("scope:%d" % handle)

func cloneScope(handle: int) -> int:
	return handle + 1

func startSpan(_name: String, _op: String) -> int:
	return 10

func spanStartChild(_handle: int, _name: String, _op: String) -> int:
	return 20

func releaseSpan(handle: int) -> void:
	calls.append("span:%d" % handle)

func releaseEvent(handle: int) -> void:
	calls.append("event:%d" % handle)

func releaseBreadcrumb(handle: int) -> void:
	calls.append("breadcrumb:%d" % handle)

func releaseLog(handle: int) -> void:
	calls.append("log:%d" % handle)

func releaseMetric(handle: int) -> void:
	calls.append("metric:%d" % handle)
)");
	if (!CHECK(script->reload() == OK)) {
		return nullptr;
	}
	Object *plugin = memnew(Object);
	plugin->set_script(script);
	plugin->set("calls", p_calls);
	return plugin;
}

} // namespace

TEST_SUITE("AndroidPluginLifetime") {
	TEST_CASE("Owned wrappers release their handles while the plugin is alive") {
		Array calls;
		Object *plugin = _create_mock_plugin(calls);
		REQUIRED_CHECK(plugin);
		ObjectHandle<Object> handle(plugin);
		{
			AndroidScope scope(handle, 1);
			SentryScopeImpl *clone = scope.clone();
			memdelete(clone);

			SentrySpanImpl *span = AndroidSpan::start_root(handle, "root", Dictionary());
			SentrySpanImpl *child = span->start_child("child", Dictionary());
			memdelete(child);
			memdelete(span);

			Ref<AndroidEvent> event = memnew(AndroidEvent(handle, 3));
			Ref<AndroidBreadcrumb> breadcrumb = memnew(AndroidBreadcrumb(handle, 4));
			Ref<AndroidLog> log = memnew(AndroidLog(handle, 5));
			Ref<AndroidMetric> metric = memnew(AndroidMetric(handle, 6));
		}

		CHECK(calls.size() == 8);
		for (const char *released : { "scope:1", "scope:2", "span:10", "span:20", "event:3", "breadcrumb:4", "log:5", "metric:6" }) {
			CHECK(calls.count(released) == 1);
		}
		memdelete(plugin);
	}

	TEST_CASE("Borrowed wrappers do not release their handles") {
		Array calls;
		Object *plugin = _create_mock_plugin(calls);
		REQUIRED_CHECK(plugin);
		ObjectHandle<Object> handle(plugin);
		{
			Ref<AndroidEvent> event = memnew(AndroidEvent(handle, 1));
			event->set_as_borrowed();

			Ref<AndroidLog> log = memnew(AndroidLog(handle, 2));
			log->set_as_borrowed();

			Ref<AndroidMetric> metric = memnew(AndroidMetric(handle, 3));
			metric->set_as_borrowed();
		}

		CHECK(calls.is_empty());
		memdelete(plugin);
	}

	TEST_CASE("Wrappers, cloned scopes and child spans survive plugin destruction") {
		Array calls;
		Object *plugin = _create_mock_plugin(calls);
		REQUIRED_CHECK(plugin);
		ObjectHandle<Object> handle(plugin);
		{
			AndroidScope scope(handle, 1);
			SentryScopeImpl *clone = scope.clone();

			SentrySpanImpl *span = AndroidSpan::start_root(handle, "root", Dictionary());
			SentrySpanImpl *child = span->start_child("child", Dictionary());

			Ref<AndroidEvent> event = memnew(AndroidEvent(handle, 3));
			Ref<AndroidBreadcrumb> breadcrumb = memnew(AndroidBreadcrumb(handle, 4));
			Ref<AndroidLog> log = memnew(AndroidLog(handle, 5));
			Ref<AndroidMetric> metric = memnew(AndroidMetric(handle, 6));

			memdelete(plugin);
			CHECK(handle.get() == nullptr);

			scope.set_tag("after", "destruction");
			scope.clear();
			clone->clear();
			SentryScopeImpl *inert_clone = clone->clone();
			inert_clone->clear();
			memdelete(inert_clone);
			memdelete(clone);

			span->set_attribute("after", "destruction");
			span->set_status(SPAN_STATUS_OK);
			span->end();
			child->end();
			CHECK(span->get_trace_headers().is_empty());
			SentrySpanImpl *inert_child = span->start_child("inert", Dictionary());
			CHECK(Castable::cast_to<AndroidSpan>(inert_child) == nullptr);
			memdelete(inert_child);
			memdelete(child);
			memdelete(span);

			event->set_message("after destruction");
			event->merge_context("context", Dictionary());
			CHECK(event->get_message().is_empty());
			CHECK(event->get_level() == LEVEL_ERROR);
			CHECK_FALSE(event->is_crash());

			breadcrumb->set_message("after destruction");
			CHECK(breadcrumb->get_timestamp().is_null());

			log->set_body("after destruction");
			CHECK(log->get_body().is_empty());

			metric->set_value(1.0);
			CHECK(metric->get_value() == 0.0);
		}

		CHECK(calls.is_empty());
	}
}

#endif // TESTS_ENABLED && SDK_ANDROID
