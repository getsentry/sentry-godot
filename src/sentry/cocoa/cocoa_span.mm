#include "cocoa_span.h"

#include "cocoa_util.h"
#include "sentry/sentry_sdk.h"

namespace sentry::cocoa {

SentrySpanImpl *CocoaSpan::start_root(const String &p_name, const Dictionary &p_attributes) {
	if (!SentryObjCSDK.isEnabled) {
		return SentrySpanImpl::create_noop();
	}
	const SentrySDK::TraceContext trace = SentrySDK::get_singleton()->get_trace_context();
	SentryObjCTransactionContext *context = [[SentryObjCTransactionContext alloc]
				initWithName:string_to_objc(p_name)
				   operation:string_to_objc(p_attributes.get("sentry.op", String()))
					 traceId:[[SentryObjCId alloc] initWithUUIDString:string_to_objc(trace.trace_id)]
					  spanId:[[SentryObjCSpanId alloc] init]
				parentSpanId:trace.parent_span_id.is_empty()
					? nil
					: [[SentryObjCSpanId alloc] initWithValue:string_to_objc(trace.parent_span_id)]
			   parentSampled:SentryObjCSampleDecisionUndecided
			parentSampleRate:nil
			parentSampleRand:nil];
	// Cocoa returns a span even for unsampled transactions; its headers still carry the trace context and sampling decision.
	SentryObjCSpan *span = [SentryObjCSDK startTransactionWithContext:context bindToScope:NO];
	return memnew(CocoaSpan(span, p_attributes));
}

SentrySpanImpl *CocoaSpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	SentryObjCSpan *child = [_span startChildWithOperation:string_to_objc(p_attributes.get("sentry.op", String()))
											   description:string_to_objc(p_name)];
	return memnew(CocoaSpan(child, p_attributes));
}

void CocoaSpan::set_attribute(const String &p_key, const Variant &p_value) {
	[_span setDataValue:variant_to_scope_attribute(p_value) forKey:string_to_objc(p_key)];
}

void CocoaSpan::set_status(SpanStatus p_status) {
	_span.status = p_status == SPAN_STATUS_OK ? SentryObjCSpanStatusOk : SentryObjCSpanStatusInternalError;
}

void CocoaSpan::end() {
	[_span finishWithStatus:_span.status == SentryObjCSpanStatusUndefined ? SentryObjCSpanStatusOk : _span.status];
}

PackedStringArray CocoaSpan::get_trace_headers() {
	PackedStringArray headers;
	SentryObjCTraceHeader *trace = [_span toTraceHeader];
	headers.append("sentry-trace: " + string_from_objc([trace value]));
	NSString *baggage = [_span baggageHttpHeader];
	if (baggage.length > 0) {
		headers.append("baggage: " + string_from_objc(baggage));
	}
	if (SENTRY_OPTIONS()->is_propagate_traceparent_enabled()) {
		headers.append(vformat("traceparent: 00-%s-%s-%s",
				string_from_objc(trace.traceId.sentryIdString),
				string_from_objc(trace.spanId.sentrySpanIdString),
				trace.sampled == SentryObjCSampleDecisionYes ? "01" : "00"));
	}
	return headers;
}

void CocoaSpan::_apply_attributes(const Dictionary &p_attributes) {
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		String name = key;
		ERR_CONTINUE_MSG(name.is_empty(), "Sentry: Can't set attribute with an empty key.");
		set_attribute(name, p_attributes[key]);
	}
}

CocoaSpan::CocoaSpan(SentryObjCSpan *p_span, const Dictionary &p_attributes) :
		_span(p_span) {
	_apply_attributes(p_attributes);
}

CocoaSpan::~CocoaSpan() {
}

} //namespace sentry::cocoa
