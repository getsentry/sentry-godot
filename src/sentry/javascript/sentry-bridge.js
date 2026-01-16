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

	setUser : function(id, username, email, ip) {
		var user = {};
		if (id && id != '') {
			user.id = id;
		}
		if (username && username != '') {
			user.username = username;
		}
		if (email && email != '') {
			user.email = email;
		}
		if (ip && ip != '') {
			user.ip_address = ip;
		}
		console.log("setting user ", user);
		Sentry.setUser(user);
	},

	removeUser : function() {
		Sentry.removeUser();
	},

	parseAttributes : function(json) {
		if (json == '') {
			return {}
		}
		try {
			var attributes = JSON.parse(json);
		} catch (e) {
			console.error("Failed to parse attributes JSON:", e);
			attributes = {};
		}
		return attributes;
	},

	logTrace : function(message, attributesJson) {
		Sentry.logger().trace(message, parseAttributes(attributesJson));
	},

	logDebug : function(message, attributesJson) {
		Sentry.logger().debug(message, parseAttributes(attributesJson));
	},

	logInfo : function(message, attributesJson) {
		Sentry.logger().info(message, parseAttributes(attributesJson));
	},

	logWarning : function(message, attributesJson) {
		Sentry.logger().warning(message, parseAttributes(attributesJson));
	},

	logError : function(message, attributesJson) {
		Sentry.logger().error(message, parseAttributes(attributesJson));
	},

	logFatal : function(message, attributesJson) {
		Sentry.logger().fatal(message, parseAttributes(attributesJson));
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
