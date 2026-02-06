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
			"logTrace",
			"logDebug",
			"logInfo",
			"logWarn",
			"logError",
			"logFatal",
			"captureMessage",
			"captureEvent",
			"captureFeedback",
			"lastEventId",
			"addBreadcrumb",
			"addBytesAttachment",
			"storeBytes",
			"mergeJsonIntoObject",
			"pushJsonObjectToArray",
			"objectToJson",
			"getDoubleAsString",
			"setDoubleFromString",
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

		runTest("init()", () => {
			bridge.init(() => {}, null, "https://test@sentry.io/123", false, "1.0.0", "1", "production", 1.0, 100, false, "0.1.0");
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

		runTest("captureMessage()", () => {
			const result = bridge.captureMessage("Test message", "info");
			assertEqual(typeof result, "string", "captureMessage should return a string");
		});

		runTest("captureEvent()", () => {
			const result = bridge.captureEvent({ message : "Test event" });
			assertEqual(typeof result, "string", "captureEvent should return a string");
		});

		runTest("lastEventId()", () => {
			const result = bridge.lastEventId();
			assertEqual(typeof result, "string", "lastEventId should return a string");
		});

		runTest("addBreadcrumb()", () => {
			bridge.addBreadcrumb({ message : "Test breadcrumb", category : "test" });
		});

		runTest("captureFeedback()", () => {
			const result = bridge.captureFeedback("Test feedback", "Test User", "test@example.com", "");
			assertEqual(typeof result, "string", "captureFeedback should return a string");
		});

		runTest("storeBytes()", () => {
			const id1 = bridge.storeBytes(new Uint8Array([ 1, 2, 3, 4 ]));
			assertEqual(typeof id1, "number", "storeBytes should return a number");
			assert(id1 > 0, "storeBytes ID should be positive");
			const id2 = bridge.storeBytes(new Uint8Array([ 5, 6 ]));
			assert(id2 > 0 && id2 !== id1, "storeBytes should return unique IDs");
		});

		runTest("addBytesAttachment()", () => {
			bridge.addBytesAttachment("test.txt", new Uint8Array([ 104, 101, 108, 108, 111 ]), "text/plain");
		});

		runTest("mergeJsonIntoObject()", () => {
			const target = { existing : "value" };
			bridge.mergeJsonIntoObject(target, '{"new": "property"}');
			assertEqual(target.existing, "value", "existing property should be preserved");
			assertEqual(target.new, "property", "new property should be merged");
		});

		runTest("pushJsonObjectToArray()", () => {
			const arr = [];
			bridge.pushJsonObjectToArray(arr, '{"item": "value"}');
			assertEqual(arr.length, 1, "array should have one element");
			assertEqual(arr[0].item, "value", "pushed object should have correct value");
		});

		runTest("objectToJson()", () => {
			const result = bridge.objectToJson({ message : "test", level : "info" });
			assertEqual(typeof result, "string", "objectToJson should return a string");
			const parsed = JSON.parse(result);
			assertEqual(parsed.message, "test", "JSON should contain message");
			assertEqual(parsed.level, "info", "JSON should contain level");
		});

		runTest("objectToJson() with empty object", () => {
			const result = bridge.objectToJson({});
			assertEqual(result, "{}", "empty object should serialize to '{}'");
		});

		runTest("getDoubleAsString()", () => {
			const obj = { timestamp : 1234567890.123456 };
			const result = bridge.getDoubleAsString(obj, "timestamp");
			assertEqual(typeof result, "string", "should return a string");
			assertEqual(result, "1234567890.123456", "should preserve precision");
		});

		runTest("setDoubleFromString()", () => {
			const obj = {};
			bridge.setDoubleFromString(obj, "timestamp", "1234567890.123456");
			assertEqual(typeof obj.timestamp, "number", "should set a number property");
			assertEqual(obj.timestamp, 1234567890.123456, "should set correct value");
		});

		runTest("close()", () => {
			bridge.close();
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
