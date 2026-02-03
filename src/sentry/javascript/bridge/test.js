#!/usr/bin/env node

// Mock DOM environment
global.window = {};

console.log("🔍 Testing Final Sentry Bridge Bundle...\n");

let failureCount = 0;
let passCount = 0;

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
		const methods = [
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
		methods.forEach((method) => {
			runTest(method, () => {
				if (typeof bridge[method] !== "function") {
					throw new Error(`${method} is not a function`);
				}
			});
		});

		console.log("\n🧪 Functional tests:");

		runTest("init()", () => {
			bridge.init(() => {}, null, "https://test@sentry.io/123", false, "1.0.0", "1", "production");
		});

		runTest("isEnabled()", () => {
			bridge.isEnabled();
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
			bridge.captureMessage("Test message");
		});

		runTest("captureEvent()", () => {
			bridge.captureEvent({ message : "Test event" });
		});

		runTest("lastEventId()", () => {
			bridge.lastEventId();
		});

		runTest("addBreadcrumb()", () => {
			bridge.addBreadcrumb({ message : "Test breadcrumb", category : "test" });
		});

		runTest("captureFeedback()", () => {
			bridge.captureFeedback("Test feedback", "Test User", "test@example.com");
		});

		runTest("storeBytes()", () => {
			bridge.storeBytes(new Uint8Array([ 1, 2, 3, 4 ]));
		});

		runTest("addBytesAttachment()", () => {
			bridge.addBytesAttachment("test.txt", new Uint8Array([ 104, 101, 108, 108, 111 ]), "text/plain");
		});

		runTest("mergeJsonIntoObject()", () => {
			const target = { existing : "value" };
			bridge.mergeJsonIntoObject(target, '{"new": "property"}');
		});

		runTest("pushJsonObjectToArray()", () => {
			const arr = [];
			bridge.pushJsonObjectToArray(arr, '{"item": "value"}');
		});

		runTest("objectToJson()", () => {
			bridge.objectToJson({ message : "test", level : "info" });
		});

		runTest("getDoubleAsString()", () => {
			const obj = { timestamp : 1234567890.123456 };
			bridge.getDoubleAsString(obj, "timestamp");
		});

		runTest("setDoubleFromString()", () => {
			const obj = {};
			bridge.setDoubleFromString(obj, "timestamp", "1234567890.123456");
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
