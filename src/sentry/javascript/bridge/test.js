#!/usr/bin/env node

// Mock DOM environment
global.window = {};

console.log("🔍 Testing Final Sentry Bridge Bundle...\n");

let failureCount = 0;

try {
	// Load the bundle (this should work now since Sentry is bundled)
	require("./dist/sentry-bundle.js");

	console.log("✅ Bundle loaded successfully\n");

	// Test methods
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
			"addBytes",
			"mergeJsonIntoObject",
			"pushJsonObjectToArray",
			"objectToJson",
		];

		console.log("📋 Method availability check:");
		methods.forEach((method) => {
			const exists = typeof bridge[method] === "function";
			console.log(`  ${exists ? "✅" : "❌"} ${method}`);
			if (!exists) {
				failureCount++;
			}
		});

		console.log("\n🧪 Functional tests:");

		// Test init (won't actually send to Sentry without valid DSN)
		try {
			bridge.init(() => {}, "https://test@sentry.io/123", false, "1.0.0", "1", "production");
			console.log("✅ init() works");
		} catch (e) {
			console.log("❌ init() failed:", e.message);
			failureCount++;
		}

		// Test isEnabled
		try {
			const enabled = bridge.isEnabled();
			console.log("✅ isEnabled() works:", enabled);
		} catch (e) {
			console.log("❌ isEnabled() failed:", e.message);
			failureCount++;
		}

		// Test setTag
		try {
			bridge.setTag("test-tag", "test-value");
			console.log("✅ setTag() works");
		} catch (e) {
			console.log("❌ setTag() failed:", e.message);
			failureCount++;
		}

		// Test removeTag
		try {
			bridge.removeTag("test-tag");
			console.log("✅ removeTag() works");
		} catch (e) {
			console.log("❌ removeTag() failed:", e.message);
			failureCount++;
		}

		// Test setContext
		try {
			bridge.setContext("test-context", '{"key": "value"}');
			console.log("✅ setContext() works");
		} catch (e) {
			console.log("❌ setContext() failed:", e.message);
			failureCount++;
		}

		// Test removeContext
		try {
			bridge.removeContext("test-context");
			console.log("✅ removeContext() works");
		} catch (e) {
			console.log("❌ removeContext() failed:", e.message);
			failureCount++;
		}

		// Test setUser
		try {
			bridge.setUser("user123", "testuser", "test@example.com", "127.0.0.1");
			console.log("✅ setUser() works");
		} catch (e) {
			console.log("❌ setUser() failed:", e.message);
			failureCount++;
		}

		// Test removeUser
		try {
			bridge.removeUser();
			console.log("✅ removeUser() works");
		} catch (e) {
			console.log("❌ removeUser() failed:", e.message);
			failureCount++;
		}

		// Test logInfo
		try {
			bridge.logInfo("Test info message", '{"key": "value"}');
			console.log("✅ logInfo() works");
		} catch (e) {
			console.log("❌ logInfo() failed:", e.message);
			failureCount++;
		}

		// Test captureMessage
		try {
			const eventId = bridge.captureMessage("Test message");
			console.log("✅ captureMessage() works, returned:", eventId);
		} catch (e) {
			console.log("❌ captureMessage() failed:", e.message);
			failureCount++;
		}

		// Test captureEvent
		try {
			const eventId = bridge.captureEvent({ message : "Test event" });
			console.log("✅ captureEvent() works, returned:", eventId);
		} catch (e) {
			console.log("❌ captureEvent() failed:", e.message);
			failureCount++;
		}

		// Test lastEventId
		try {
			const lastId = bridge.lastEventId();
			console.log("✅ lastEventId() works, returned:", lastId);
		} catch (e) {
			console.log("❌ lastEventId() failed:", e.message);
			failureCount++;
		}

		// Test addBreadcrumb
		try {
			bridge.addBreadcrumb({ message : "Test breadcrumb", category : "test" });
			console.log("✅ addBreadcrumb() works");
		} catch (e) {
			console.log("❌ addBreadcrumb() failed:", e.message);
			failureCount++;
		}

		// Test captureFeedback
		try {
			const feedbackId = bridge.captureFeedback("Test feedback", "Test User", "test@example.com");
			console.log("✅ captureFeedback() works, returned:", feedbackId);
		} catch (e) {
			console.log("❌ captureFeedback() failed:", e.message);
			failureCount++;
		}

		// Test addBytes
		try {
			const bytesId = bridge.addBytes(new Uint8Array([ 1, 2, 3, 4 ]));
			console.log("✅ addBytes() works, returned:", bytesId);
		} catch (e) {
			console.log("❌ addBytes() failed:", e.message);
			failureCount++;
		}

		// Test addBytesAttachment
		try {
			bridge.addBytesAttachment("test.txt", new Uint8Array([ 104, 101, 108, 108, 111 ]), "text/plain");
			console.log("✅ addBytesAttachment() works");
		} catch (e) {
			console.log("❌ addBytesAttachment() failed:", e.message);
			failureCount++;
		}

		// Test mergeJsonIntoObject
		try {
			const target = { existing : "value" };
			bridge.mergeJsonIntoObject(target, '{"new": "property"}');
			console.log("✅ mergeJsonIntoObject() works, result:", target);
		} catch (e) {
			console.log("❌ mergeJsonIntoObject() failed:", e.message);
			failureCount++;
		}

		// Test pushJsonObjectToArray
		try {
			const arr = [ "existing" ];
			bridge.pushJsonObjectToArray(arr, '{"item": "value"}');
			console.log("✅ pushJsonObjectToArray() works, result:", arr);
		} catch (e) {
			console.log("❌ pushJsonObjectToArray() failed:", e.message);
			failureCount++;
		}

		// Test objectToJson
		try {
			const testObj = { message : "test", level : "info", tags : { foo : "bar" } };
			const json = bridge.objectToJson(testObj);
			console.log("✅ objectToJson() works, returned:", json);
		} catch (e) {
			console.log("❌ objectToJson() failed:", e.message);
			failureCount++;
		}

		// Test close
		try {
			bridge.close();
			console.log("✅ close() works");
		} catch (e) {
			console.log("❌ close() failed:", e.message);
			failureCount++;
		}

		if (failureCount > 0) {
			console.log(`\n❌ ${failureCount} test(s) failed!`);
			process.exit(1);
		} else {
			console.log("\n🎉 Bundle is working correctly!");
		}
	} else {
		console.log("❌ SentryBridge not found on window object");
		process.exit(1);
	}
} catch (error) {
	console.error("❌ Bundle test failed:", error.message);
	process.exit(1);
}
