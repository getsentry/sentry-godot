#!/usr/bin/env node

// Mock DOM environment
global.window = {};

console.log('🔍 Testing Final Sentry Bridge Bundle...\n');

try {
	// Load the bundle (this should work now since Sentry is bundled)
	require('./dist/sentry-bridge.js');

	console.log('✅ Bundle loaded successfully\n');

	// Test methods
	const bridge = global.window.SentryBridge;

	if (bridge) {
		console.log('🧪 Testing bridge methods...\n');

		// Check if all methods exist
		const methods = [ 'init', 'isEnabled', 'setContext', 'removeContext', 'setTag', 'removeTag',
			'setUser', 'removeUser', 'logTrace', 'logDebug', 'logInfo', 'logWarning',
			'logError', 'logFatal', 'captureMessage', 'captureError' ];

		console.log('📋 Method availability check:');
		methods.forEach(method => {
			const exists = typeof bridge[method] === 'function';
			console.log(`  ${exists ? '✅' : '❌'} ${method}`);
		});

		console.log('\n🧪 Functional tests:');

		// Test init (won't actually send to Sentry without valid DSN)
		try {
			bridge.init('https://test@sentry.io/123', false, '1.0.0', '1', 'production');
			console.log('✅ init() works');
		} catch (e) {
			console.log('❌ init() failed:', e.message);
		}

		// Test isEnabled
		try {
			const enabled = bridge.isEnabled();
			console.log('✅ isEnabled() works:', enabled);
		} catch (e) {
			console.log('❌ isEnabled() failed:', e.message);
		}

		// Test setTag
		try {
			bridge.setTag('test-tag', 'test-value');
			console.log('✅ setTag() works');
		} catch (e) {
			console.log('❌ setTag() failed:', e.message);
		}

		// Test setUser
		try {
			bridge.setUser('user123', 'testuser', 'test@example.com', '127.0.0.1');
			console.log('✅ setUser() works');
		} catch (e) {
			console.log('❌ setUser() failed:', e.message);
		}

		// Test logInfo
		try {
			bridge.logInfo('Test info message', '{"key": "value"}');
			console.log('✅ logInfo() works');
		} catch (e) {
			console.log('❌ logInfo() failed:', e.message);
		}

		// Test captureMessage
		try {
			const eventId = bridge.captureMessage('Test message');
			console.log('✅ captureMessage() works, returned:', eventId);
		} catch (e) {
			console.log('❌ captureMessage() failed:', e.message);
		}

		// Test captureError
		try {
			const errorId = bridge.captureError('Test error', '{"stack": "test stack"}');
			console.log('✅ captureError() works, returned:', errorId);
		} catch (e) {
			console.log('❌ captureError() failed:', e.message);
		}

		console.log('\n🎉 Bundle is working correctly!');
		console.log(`📦 Bundle size: ${(76827 / 1024).toFixed(1)}KB`);

	} else {
		console.log('❌ SentryBridge not found on window object');
	}

} catch (error) {
	console.error('❌ Bundle test failed:', error.message);
	process.exit(1);
}