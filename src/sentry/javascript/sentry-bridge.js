window.SentryBridge = {
	init : function(
			dsn,
			debug,
			release,
			dist,
			environment,
			sampleRate,
			maxBreadcrumbs,
			enableLogs,
	) {
		if (window.Sentry) {
			console.log("Initializing Sentry via bridge...");
			Sentry.init({
				dsn : dsn,
				debug : debug,
				release : release,
				dist : dist,
				environment : environment,
				sampleRate : sampleRate,
				maxBreadcrumbs : maxBreadcrumbs,
				enableLogs : enableLogs,
				integrations : [ window.Sentry.wasmIntegration() ],
			});
		} else {
			console.error("Sentry JavaScript SDK not found!");
		}
	},

	isEnabled : function() {
		return window.Sentry.isEnabled();
	},

	setContext : function(key, valueJson) {
		try {
			var value = JSON.parse(valueJson);
			Sentry.setContext(key, value);
		} catch (e) {
			console.error("Failed to parse context JSON:", e);
		}
	},

	removeContext : function(key) {
		Sentry.removeContext(key);
	},

	setTag : function(key, val) {
		Sentry.setTag(key, val);
	},

	removeTag : function(key) {
		Sentry.removeTag(key);
	},

	captureMessage : function(message) {
		return Sentry.captureMessage(message);
	},

	captureError : function(message, stacktraceJson) {
		try {
			var stacktrace = JSON.parse(stacktraceJson);
			return Sentry.captureEvent({
				message : message,
				stacktrace : stacktrace,
			});
		} catch (e) {
			console.error("Failed to capture event:", e);
			return "";
		}
	},
};
