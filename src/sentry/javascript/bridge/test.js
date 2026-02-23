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
			"takeBytes",
			"releaseBytes",
			"storeObject",
			"getObject",
			"releaseObject",
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

		runTest("addBytesAttachment()", () => {
			bridge.addBytesAttachment("test.txt", new Uint8Array([ 104, 101, 108, 108, 111 ]), "text/plain");
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
