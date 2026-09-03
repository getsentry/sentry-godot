#!/usr/bin/env node

// Mock DOM environment
global.window = {};

console.log("🔍 Testing Final Sentry Bridge Bundle...\n");

let failureCount = 0;
let passCount = 0;

function assert(condition, message) {
	if (!condition) {
		throw new Error(message || "Assertion failed");
	}
}

function assertEqual(actual, expected, message) {
	if (actual !== expected) {
		throw new Error(
				`${message || "Assertion failed"}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`,
		);
	}
}

function runTest(name, testFn) {
	try {
		testFn();
		console.log(`  ✅ ${name}`);
		passCount++;
	} catch (e) {
		console.log(`  ❌ ${name}: ${e.message}`);
		failureCount++;
	}
}

try {
	require("./dist/sentry-bundle.js");

	const { spanToStreamedSpanJSON, getCurrentScope, propagationContextFromHeaders } = require("@sentry/core");

	console.log("✅ Bundle loaded successfully\n");

	const bridge = global.window.SentryBridge;

	if (bridge) {
		console.log("🧪 Testing bridge methods...\n");

		// Check if all methods exist
		const expectedMethods = [
			"init",
			"close",
			"isEnabled",
			"setContext",
			"removeContext",
			"setTag",
			"removeTag",
			"setUser",
			"removeUser",
			"setTrace",
			"eventSetUser",
			"createScope",
			"scopeSetContext",
			"scopeSetFingerprint",
			"scopeSetUser",
			"scopeAddBytesAttachment",
			"scopeAddFileAttachment",
			"scopeClear",
			"scopeClone",
			"scopeSetSpan",
			"startSpan",
			"spanSetStatus",
			"spanGetTraceHeaders",
			"logTrace",
			"logDebug",
			"logInfo",
			"logWarn",
			"logError",
			"logFatal",
			"captureEvent",
			"captureFeedback",
			"lastEventId",
			"addBreadcrumb",
			"addBytesAttachment",
			"addFileAttachment",
			"clearAttachments",
			"storeBytes",
			"takeBytes",
			"releaseBytes",
			"storeObject",
			"getObject",
			"releaseObject",
			"metricsAddCount",
			"metricsAddGauge",
			"metricsAddDistribution",
			"setAttribute",
			"removeAttribute",
		];

		console.log("📋 Method availability check:");
		expectedMethods.forEach((method) => {
			runTest(method, () => {
				if (typeof bridge[method] !== "function") {
					throw new Error(`${method} is not a function`);
				}
			});
		});

		// Reverse check: detect unexpected methods on the bridge
		const bridgeMethods = Object.getOwnPropertyNames(Object.getPrototypeOf(bridge)).filter(
				(m) => m !== "constructor" && typeof bridge[m] === "function",
		);
		bridgeMethods.forEach((method) => {
			runTest(`bridge should not expose unexpected method: ${method}`, () => {
				if (!expectedMethods.includes(method)) {
					throw new Error(`unexpected method "${method}" found on bridge but not in test list`);
				}
			});
		});

		console.log("\n🧪 Functional tests:");

		// Stands in for the C++ layer: hands back fixed bytes, leaving "user://missing.txt" unread like a missing file.
		const readAttachmentPaths = [];
		const readAttachment = (request) => {
			readAttachmentPaths.push(request.path);
			if (request.path !== "user://missing.txt") {
				request.data = new Uint8Array([ 1, 2, 3, 4 ]);
			}
		};

		// Stands in for the C++ layer: records the feedback events the hook receives.
		const feedbackEvents = [];
		const beforeSendFeedback = (event) => {
			feedbackEvents.push(event);
		};
		const initBridge = (traceLifecycle, propagateTraceparent = false, orgId = "", tracePropagationTargets = [ ".*" ]) => {
			bridge.init(() => {}, beforeSendFeedback, null, null, readAttachment, "https://test@sentry.io/123", false,
					"1.0.0", "1", "production", 1.0, 1.0, traceLifecycle, JSON.stringify(tracePropagationTargets),
					propagateTraceparent, orgId, 100, false,
					"0.1.0");
		};

		runTest("init()", () => {
			initBridge(bridge.TraceLifecycle.Stream);
			const targets = bridge.createScope().getClient().getOptions().tracePropagationTargets;
			assert(targets[0] instanceof RegExp, '".*" should become a regular expression');
			assertEqual(targets[0].source, ".*", '".*" should match every URL');
		});

		// Observes what actually goes out with an event. The bridge registers its own handler during
		// init() and handlers run in registration order, so this one sees the resolved and filtered list.
		const sentAttachments = [];
		bridge.createScope().getClient().on("beforeSendEvent", (_event, hint) => {
			sentAttachments.push(hint?.attachments ?? []);
		});

		runTest("isEnabled()", () => {
			const result = bridge.isEnabled();
			assertEqual(typeof result, "boolean", "isEnabled should return a boolean");
		});

		runTest("setTag()", () => {
			bridge.setTag("test-tag", "test-value");
		});

		runTest("removeTag()", () => {
			bridge.removeTag("test-tag");
		});

		runTest("setContext()", () => {
			bridge.setContext("test-context", '{"key": "value"}');
		});

		runTest("removeContext()", () => {
			bridge.removeContext("test-context");
		});

		runTest("setUser()", () => {
			bridge.setUser("user123", "testuser", "test@example.com", "127.0.0.1");
		});

		runTest("removeUser()", () => {
			bridge.removeUser();
		});

		runTest("setTrace()", () => {
			// Scopes hold the propagation context by reference, so one made before the call has to move
			// onto the new trace too, and values have to be read out before the next call changes them.
			const existing = bridge.createScope();

			bridge.setTrace("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbb");
			const withParent = bridge.createScope().getPropagationContext();
			assertEqual(withParent.traceId, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
					"setTrace should adopt the given trace id");
			assertEqual(withParent.parentSpanId, "bbbbbbbbbbbbbbbb",
					"setTrace should adopt the given parent span id");
			assert(withParent.propagationSpanId !== undefined,
					"setTrace should mint a span id for captures outside a span");
			assertEqual(existing.getPropagationContext().traceId, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
					"setTrace should move a scope that already exists onto the new trace");
			const firstSpanId = withParent.propagationSpanId;

			bridge.setTrace("cccccccccccccccccccccccccccccccc", "");
			const noParent = bridge.createScope().getPropagationContext();
			assertEqual(noParent.traceId, "cccccccccccccccccccccccccccccccc",
					"setTrace should replace the trace id");
			assertEqual(noParent.parentSpanId, undefined,
					"setTrace should leave the parent unset when none is given");
			assert(noParent.propagationSpanId !== firstSpanId,
					"setTrace should mint a fresh span id on every call");
			assertEqual(existing.getPropagationContext().parentSpanId, undefined,
					"setTrace should clear the parent on a scope that already exists");
		});

		runTest("setTrace() over a continued trace", () => {
			getCurrentScope().setPropagationContext(propagationContextFromHeaders(
					"12345678123456781234567812345678-1234567812345678-1",
					"sentry-trace_id=12345678123456781234567812345678,sentry-sample_rate=0.5"));
			const continued = bridge.createScope();

			bridge.setTrace("dddddddddddddddddddddddddddddddd", "");
			assertEqual(continued.getPropagationContext().traceId, "dddddddddddddddddddddddddddddddd",
					"setTrace should replace a continued trace");
			assertEqual(continued.getPropagationContext().dsc, undefined,
					"setTrace should drop the frozen baggage of the trace it replaces");
			assertEqual(continued.getPropagationContext().sampled, undefined,
					"setTrace should drop the sampling decision of the trace it replaces");
		});

		runTest("scopeClone()", () => {
			const base = bridge.createScope();
			const fork = bridge.scopeClone(base);

			bridge.setTrace("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", "");
			assertEqual(fork.getPropagationContext().traceId, "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
					"a forked scope should follow a trace change");
			assertEqual(fork.getPropagationContext().traceId, base.getPropagationContext().traceId,
					"a forked scope should stay on the trace of the scope it was forked from");
		});

		runTest("eventSetUser()", () => {
			const event = {};
			bridge.eventSetUser(event, "user123", "testuser", "test@example.com", "127.0.0.1");
			assertEqual(event.user.id, "user123", "eventSetUser should set user id on event");
			assertEqual(event.user.username, "testuser", "eventSetUser should set username on event");
			assertEqual(event.user.email, "test@example.com", "eventSetUser should set email on event");
			assertEqual(event.user.ip_address, "127.0.0.1", "eventSetUser should set ip_address on event");
			const event2 = {};
			bridge.eventSetUser(event2, "", "", "", "");
			assertEqual(Object.keys(event2.user).length, 0, "eventSetUser should skip empty fields");
		});

		runTest("createScope()", () => {
			const scope = bridge.createScope();
			assert(scope.getClient() !== undefined, "createScope should bind the current client");
			assertEqual(bridge.createScope().getPropagationContext().traceId,
					scope.getPropagationContext().traceId,
					"createScope should inherit the current trace");
		});

		runTest("events outside a span share one span id", () => {
			const scope = bridge.createScope();
			const spanId = scope.getPropagationContext().propagationSpanId;
			assert(spanId, "init should pin a span id for events captured outside a span");
			assertEqual(bridge.createScope().getPropagationContext().propagationSpanId, spanId,
					"every scope should carry the same pinned span id");
			assertEqual(bridge.scopeClear(scope).getPropagationContext().propagationSpanId, spanId,
					"scopeClear should preserve the pinned span id");
		});

		runTest("scope setters", () => {
			const scope = bridge.createScope();
			bridge.scopeSetContext(scope, "test-context", '{"key": "value"}');
			bridge.scopeSetFingerprint(scope, '["a","b"]');
			bridge.scopeSetUser(scope, "user123", "testuser", "test@example.com", "127.0.0.1");
			scope.setTag("subsystem", "savegame");
			scope.setLevel("warning");
			const data = scope.getScopeData();
			assertEqual(data.contexts["test-context"].key, "value", "scopeSetContext should set the context");
			assertEqual(data.fingerprint.join(","), "a,b", "scopeSetFingerprint should set the fingerprint");
			assertEqual(data.user.id, "user123", "scopeSetUser should set the user");
			assertEqual(data.tags.subsystem, "savegame", "setTag should set a tag");
			assertEqual(data.level, "warning", "setLevel should set the level");
			scope.setUser(null);
			assertEqual(scope.getScopeData().user.id, undefined, "setUser(null) should clear the user");
		});

		runTest("scope.addBreadcrumb()", () => {
			const scope = bridge.createScope();
			// Second argument is the breadcrumb cap the C++ layer reads from SentryOptions.
			scope.addBreadcrumb({ message : "first" }, 2);
			scope.addBreadcrumb({ message : "second" }, 2);
			scope.addBreadcrumb({ message : "third" }, 2);
			const data = scope.getScopeData();
			assertEqual(data.breadcrumbs.length, 2, "addBreadcrumb should honor the max breadcrumbs argument");
			assertEqual(data.breadcrumbs[0].message, "second", "addBreadcrumb should drop the oldest past the cap");
			assertEqual(data.breadcrumbs[1].message, "third", "addBreadcrumb should keep the newest breadcrumb");
		});

		runTest("scope.setAttribute()", () => {
			const scope = bridge.createScope();
			scope.setAttribute("level", "forest");
			scope.setAttribute("enemy_id", 42);
			scope.setAttribute("health", 10.5);
			scope.setAttribute("elite", false);
			const data = scope.getScopeData();
			assertEqual(data.attributes.level, "forest", "setAttribute should set a string attribute");
			assertEqual(data.attributes.enemy_id, 42, "setAttribute should set an int attribute");
			assertEqual(data.attributes.health, 10.5, "setAttribute should set a float attribute");
			assertEqual(data.attributes.elite, false, "setAttribute should set a bool attribute");
		});

		runTest("scope.clone()", () => {
			const scope = bridge.createScope();
			scope.setTag("subsystem", "savegame");
			const fork = scope.clone();
			assert(fork.getClient() !== undefined, "clone should carry the client over");
			assertEqual(fork.getPropagationContext().traceId, scope.getPropagationContext().traceId,
					"clone should stay on the parent's trace");
			assertEqual(fork.getScopeData().tags.subsystem, "savegame",
					"clone should inherit the parent's data");
			fork.setTag("subsystem", "worldgen");
			assertEqual(scope.getScopeData().tags.subsystem, "savegame",
					"writes to the clone should not reach the parent");
		});

		runTest("scopeAddBytesAttachment()", () => {
			const scope = bridge.createScope();
			bridge.scopeAddBytesAttachment(scope, "save.txt", new Uint8Array([ 1, 2, 3 ]), "text/plain", "event.attachment");
			const attachments = scope.getScopeData().attachments;
			assertEqual(attachments.length, 1, "scopeAddBytesAttachment should add the attachment to the scope");
			assertEqual(attachments[0].filename, "save.txt", "scopeAddBytesAttachment should set the filename");
			assertEqual(attachments[0].contentType, "text/plain", "scopeAddBytesAttachment should set the content type");
			assertEqual(attachments[0].attachmentType, "event.attachment", "scopeAddBytesAttachment should set the attachment type");
			assertEqual(attachments[0].data.length, 3, "scopeAddBytesAttachment should carry the bytes");
		});

		runTest("scopeAddFileAttachment()", () => {
			const scope = bridge.createScope();
			bridge.scopeAddFileAttachment(scope, "user://save.dat", "save.dat", "application/octet-stream", "");
			const attachments = scope.getScopeData().attachments;
			assertEqual(attachments.length, 1, "scopeAddFileAttachment should add the attachment to the scope");
			assertEqual(attachments[0].filename, "save.dat", "scopeAddFileAttachment should set the filename");
			assertEqual(attachments[0].data.length, 0, "scopeAddFileAttachment should leave the bytes for capture time");
			assertEqual(attachments[0].__godotPath, "user://save.dat",
					"scopeAddFileAttachment should mark the attachment with the path to read at capture time");
		});

		runTest("file attachments resolve on capture", () => {
			const scope = bridge.createScope();
			bridge.scopeAddFileAttachment(scope, "user://save.dat", "save.dat", "application/octet-stream", "");

			const readCountBefore = readAttachmentPaths.length;
			bridge.captureEvent({ message : "Event with a file attachment" }, scope);
			assertEqual(readAttachmentPaths.length, readCountBefore + 1, "captureEvent should read the file once");
			assertEqual(readAttachmentPaths[readAttachmentPaths.length - 1], "user://save.dat", "captureEvent should read the attachment path");

			const sent = sentAttachments[sentAttachments.length - 1];
			assertEqual(sent.length, 1, "captureEvent should send the resolved attachment");
			assertEqual(sent[0].data.length, 4, "the sent attachment should carry the bytes that were read");
			assertEqual(sent[0].filename, "save.dat", "resolving should keep the filename");
			assertEqual(sent[0].__godotPath, undefined, "the path marker should not be sent with the event");

			const kept = scope.getScopeData().attachments[0];
			assertEqual(kept.data.length, 0, "the scope's own attachment should not keep the bytes");
			assertEqual(kept.__godotPath, "user://save.dat",
					"the scope should keep the path so the next capture reads the file again");
		});

		runTest("unreadable file attachments are dropped", () => {
			const scope = bridge.createScope();
			bridge.scopeAddFileAttachment(scope, "user://missing.txt", "missing.txt", "", "");
			bridge.captureEvent({ message : "Event with a missing file attachment" }, scope);

			const sent = sentAttachments[sentAttachments.length - 1];
			assertEqual(sent.length, 0, "an attachment the C++ layer could not read should not be sent");
		});

		runTest("scopeClear()", () => {
			const scope = bridge.createScope();
			bridge.scopeSetContext(scope, "test-context", '{"key": "value"}');
			const traceId = scope.getPropagationContext().traceId;
			const cleared = bridge.scopeClear(scope);
			assertEqual(cleared.getScopeData().contexts["test-context"], undefined, "scopeClear should clear scope data");
			assertEqual(cleared.getPropagationContext().traceId, traceId,
					"scopeClear should preserve the propagation context");
			assert(cleared.getClient() !== undefined, "scopeClear should keep the client bound");
		});

		runTest("spans stream instead of becoming transactions", () => {
			const client = bridge.createScope().getClient();
			assertEqual(client.getOptions().traceLifecycle, "stream",
					"init should put the client on the streaming trace lifecycle");
			assert(client.getIntegrationByName("SpanStreaming"),
					"init should register the integration that streams ended spans");
		});

		runTest("startSpan()", () => {
			const span = bridge.startSpan("load-level", '{"sentry.op":"asset.load","chunks":7}');
			const json = spanToStreamedSpanJSON(span);
			assertEqual(json.name, "load-level", "startSpan should name the span");
			assertEqual(json.attributes["sentry.op"], "asset.load", "startSpan should carry the op through as an attribute");
			assertEqual(json.attributes.chunks, 7, "startSpan should apply the attributes");
			span.setAttribute("biome", "forest");
			assertEqual(spanToStreamedSpanJSON(span).attributes.biome, "forest", "the started span should record attributes");
			span.end();
		});

		runTest("startSpan() with a parent", () => {
			const parent = bridge.startSpan("load-level", "");
			const child = bridge.startSpan("decompress", "", parent);
			assertEqual(child.spanContext().traceId, parent.spanContext().traceId,
					"a child span should stay on the parent's trace");
			assertEqual(spanToStreamedSpanJSON(child).parent_span_id, parent.spanContext().spanId,
					"a child span should point at the parent");
			assertEqual(spanToStreamedSpanJSON(child).is_segment, false,
					"a child span should not be a segment");
			assertEqual(spanToStreamedSpanJSON(bridge.startSpan("unrelated", "")).is_segment, true,
					"a span started without a parent should be a segment");
			child.end();
			parent.end();
		});

		runTest("spanSetStatus()", () => {
			const span = bridge.startSpan("load-level", "");
			bridge.spanSetStatus(span, 1);
			assertEqual(spanToStreamedSpanJSON(span).status, "error", "SPAN_STATUS_ERROR should stream as error");
			bridge.spanSetStatus(span, 0);
			assertEqual(spanToStreamedSpanJSON(span).status, "ok", "SPAN_STATUS_OK should stream as ok");
			span.end();
		});

		runTest("spanGetTraceHeaders()", () => {
			const span = bridge.startSpan("load-level", "");
			const headers = bridge.spanGetTraceHeaders(span).split("\n");
			assertEqual(headers.length, 2, "the default configuration should yield sentry-trace and baggage");
			assertEqual(headers[0], `sentry-trace: ${span.spanContext().traceId}-${span.spanContext().spanId}-1`,
					"sentry-trace should carry the span's own ids");
			assert(headers[1].startsWith(`baggage: sentry-environment=production,`),
					`baggage should carry the dynamic sampling context, got "${headers[1]}"`);
			assert(headers[1].includes(`sentry-trace_id=${span.spanContext().traceId}`),
					"baggage should stay on the span's trace");
			span.end();
		});

		runTest("scopeSetSpan()", () => {
			const scope = bridge.createScope();
			const span = bridge.startSpan("load-level", "");
			bridge.scopeSetSpan(scope, span);
			assertEqual(scope.getScopeData().span, span, "scopeSetSpan should bind the span to the scope");
			assertEqual(scope.clone().getScopeData().span, span, "a forked scope should inherit the bound span");
			assertEqual(bridge.scopeClear(scope).getScopeData().span, undefined, "scopeClear should drop the bound span");
			bridge.scopeSetSpan(scope, span);
			bridge.scopeSetSpan(scope);
			assertEqual(scope.getScopeData().span, undefined, "scopeSetSpan without a span should unbind");
			span.end();
		});

		runTest("logTrace()", () => {
			bridge.logTrace("Test trace message", '{"key": "value"}');
		});

		runTest("logDebug()", () => {
			bridge.logDebug("Test debug message", '{"key": "value"}');
		});

		runTest("logInfo()", () => {
			bridge.logInfo("Test info message", '{"key": "value"}');
		});

		runTest("logWarn()", () => {
			bridge.logWarn("Test warn message", '{"key": "value"}');
		});

		runTest("logError()", () => {
			bridge.logError("Test error message", '{"key": "value"}');
		});

		runTest("logFatal()", () => {
			bridge.logFatal("Test fatal message", '{"key": "value"}');
		});

		runTest("captureEvent()", () => {
			const result = bridge.captureEvent({ message : "Test event" });
			assertEqual(typeof result, "string", "captureEvent should return a string");
			const scoped = bridge.captureEvent({ message : "Test event" }, bridge.createScope());
			assertEqual(typeof scoped, "string", "Scoped captureEvent should return a string");
		});

		runTest("lastEventId()", () => {
			const result = bridge.lastEventId();
			assertEqual(typeof result, "string", "lastEventId should return a string");
		});

		runTest("addBreadcrumb()", () => {
			bridge.addBreadcrumb({ message : "Test breadcrumb", category : "test" });
		});

		runTest("captureFeedback()", () => {
			const feedbackCountBefore = feedbackEvents.length;
			const result = bridge.captureFeedback("Test feedback", "Test User", "test@example.com", "");
			assertEqual(typeof result, "string", "captureFeedback should return a string");
			assertEqual(feedbackEvents.length, feedbackCountBefore + 1, "captureFeedback should run the before-send-feedback callback once");
			const seen = feedbackEvents[feedbackEvents.length - 1];
			assertEqual(seen.type, "feedback", "the callback should receive a feedback event");
			assertEqual(seen.contexts.feedback.message, "Test feedback", "the callback should see the submitted message");
			assertEqual(seen.contexts.feedback.name, "Test User", "the callback should see the submitted name");
			assertEqual(seen.contexts.feedback.contact_email, "test@example.com", "the callback should see the submitted email");
			const scoped = bridge.captureFeedback("Test feedback", "", "", "", bridge.createScope());
			assertEqual(typeof scoped, "string", "Scoped captureFeedback should return a string");
		});

		runTest("storeBytes() / takeBytes()", () => {
			const id1 = bridge.storeBytes(new Uint8Array([ 1, 2, 3, 4 ]));
			assertEqual(typeof id1, "number", "storeBytes should return a number");
			assert(id1 > 0, "storeBytes ID should be positive");
			const id2 = bridge.storeBytes(new Uint8Array([ 5, 6 ]));
			assert(id2 > 0 && id2 !== id1, "storeBytes should return unique IDs");
			const retrieved = bridge.takeBytes(id1);
			assert(retrieved instanceof Uint8Array, "takeBytes should return a Uint8Array");
			assertEqual(retrieved.length, 4, "takeBytes should return correct length");
			assertEqual(retrieved[0], 1, "takeBytes should return correct data");
			assertEqual(bridge.takeBytes(id1), undefined, "takeBytes should return undefined after take");
			bridge.takeBytes(id2);
		});

		runTest("releaseBytes()", () => {
			const id = bridge.storeBytes(new Uint8Array([ 10, 20 ]));
			bridge.releaseBytes(id);
			assertEqual(bridge.takeBytes(id), undefined, "releaseBytes should discard bytes");
		});

		// Globally added file attachments go through the same resolution as the scoped ones above.
		runTest("addFileAttachment()", () => {
			bridge.addFileAttachment("user://global.dat", "global.dat", "application/json", "event.view_hierarchy");
			bridge.captureEvent({ message : "Event with a global file attachment" }, bridge.createScope());

			const sent = sentAttachments[sentAttachments.length - 1];
			const globalAttachment = sent.find((attachment) => attachment.filename === "global.dat");
			assert(globalAttachment, "addFileAttachment should send the attachment with the event");
			assertEqual(globalAttachment.data.length, 4, "the global attachment should carry the bytes that were read");
			assertEqual(globalAttachment.contentType, "application/json", "addFileAttachment should set the content type");
			assertEqual(globalAttachment.attachmentType, "event.view_hierarchy", "addFileAttachment should set the attachment type");
		});

		runTest("addBytesAttachment()", () => {
			bridge.addBytesAttachment("test.txt", new Uint8Array([ 104, 101, 108, 108, 111 ]), "text/plain", "");
		});

		runTest("clearAttachments()", () => {
			bridge.clearAttachments();
		});

		runTest("storeObject() / getObject() / releaseObject()", () => {
			const obj = { key : "value" };
			const id = bridge.storeObject(obj);
			assertEqual(typeof id, "number", "storeObject should return a number");
			assert(id > 0, "storeObject ID should be positive");
			const retrieved = bridge.getObject(id);
			assertEqual(retrieved, obj, "getObject should return the same object");
			bridge.releaseObject(id);
			assertEqual(bridge.getObject(id), undefined, "getObject should return undefined after release");
		});

		runTest("metricsAddCount()", () => {
			bridge.metricsAddCount("test.count", 1, '{"key": "value"}');
		});

		runTest("metricsAddGauge()", () => {
			bridge.metricsAddGauge("test.gauge", 42.5, "millisecond", '{"key": "value"}');
		});

		runTest("metricsAddDistribution()", () => {
			bridge.metricsAddDistribution("test.dist", 100, "byte", '{"key": "value"}');
		});

		runTest("setAttribute()", () => {
			bridge.setAttribute("test-attr", "test-value");
		});

		runTest("removeAttribute()", () => {
			bridge.removeAttribute("test-attr");
		});

		runTest("spanGetTraceHeaders() with traceparent enabled", () => {
			bridge.close(2000);
			initBridge(bridge.TraceLifecycle.Stream, true, "42");
			const span = bridge.startSpan("load-level", "");
			const headers = bridge.spanGetTraceHeaders(span).split("\n");
			assertEqual(headers.length, 3, "propagateTraceparent should add a third header");
			assertEqual(headers[2], `traceparent: 00-${span.spanContext().traceId}-${span.spanContext().spanId}-01`,
					"traceparent should follow the W3C format");
			assert(headers[1].includes("sentry-org_id=42"), "orgId should reach the baggage header");
			span.end();
		});

		runTest("static trace lifecycle", () => {
			bridge.close(2000);
			initBridge(bridge.TraceLifecycle.Static);
			const client = bridge.createScope().getClient();
			assertEqual(client.getOptions().traceLifecycle, "static",
					"init should preserve the static trace lifecycle");
			assertEqual(client.getIntegrationByName("SpanStreaming"), undefined,
					"init should not register span streaming in static mode");
		});

		runTest("close()", () => {
			bridge.close(2000);
		});

		// Print summary
		console.log("\n" +
				"=".repeat(50));
		console.log(`📊 Test Summary: ${passCount} passed, ${failureCount} failed`);
		console.log("=".repeat(50));

		if (failureCount > 0) {
			console.log(`\n❌ ${failureCount} test(s) failed!`);
			process.exit(1);
		} else {
			console.log("\n🎉 All tests passed!");
		}
	} else {
		console.log("❌ SentryBridge not found on window object");
		process.exit(1);
	}
} catch (error) {
	console.error("❌ Bundle test failed:", error.message);
	console.error(error.stack);
	process.exit(1);
}
