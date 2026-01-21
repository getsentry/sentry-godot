// Type Definitions
import * as Sentry from "@sentry/browser";
import type { Breadcrumb } from "@sentry/browser";

interface SentryEvent {
	message?: string;
	stacktrace?: any;
}

interface User {
	id?: string;
	username?: string;
	email?: string;
	ip_address?: string;
}

interface SentryBridge {
	init(
		dsn: string,
		debug?: boolean,
		release?: string,
		dist?: string,
		environment?: string,
		sampleRate?: number,
		maxBreadcrumbs?: number,
		enableLogs?: boolean,
	): void;
	close(): void;
	isEnabled(): boolean;
	setContext(key: string, valueJson: string): void;
	removeContext(key: string): void;
	setTag(key: string, value: string): void;
	removeTag(key: string): void;
	setUser(id?: string, username?: string, email?: string, ip?: string): void;
	removeUser(): void;
	logTrace(message: string, attributesJson?: string): void;
	logDebug(message: string, attributesJson?: string): void;
	logInfo(message: string, attributesJson?: string): void;
	logWarning(message: string, attributesJson?: string): void;
	logError(message: string, attributesJson?: string): void;
	logFatal(message: string, attributesJson?: string): void;
	captureMessage(message: string): string;
	captureError(message: string, stacktraceJson?: string): string;
	lastEventId(): string;
	addBreadcrumb(dataJson: string): void;
}

// Utility Functions
function safeParseJSON<T = any>(json: string, fallback: T): T {
	if (json === "" || json === null || json === undefined) {
		return fallback;
	}

	try {
		return JSON.parse(json);
	} catch (error) {
		console.error("Failed to parse JSON:", error);
		return fallback;
	}
}

function parseAttributes(attributesJson: string): Record<string, any> {
	return safeParseJSON(attributesJson, {});
}

// Shared logger instance for all log methods (JUST A TEST FOR NOW)
const logger = {
	trace: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[TRACE] ${msg}`, attrs);
	},
	debug: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[DEBUG] ${msg}`, attrs);
	},
	info: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[INFO] ${msg}`, attrs);
	},
	warning: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[WARNING] ${msg}`, attrs);
	},
	error: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[ERROR] ${msg}`, attrs);
	},
	fatal: (msg: string, attrs?: Record<string, any>) => {
		console.log(`[FATAL] ${msg}`, attrs);
	},
};

// Main SentryBridge Implementation
class SentryBridgeImpl implements SentryBridge {
	constructor() {
		// Sentry functions are imported directly
	}

	/**
	 * Initialize Sentry with provided options
	 */
	init(
		dsn: string,
		debug?: boolean,
		release?: string,
		dist?: string,
		environment?: string,
		sampleRate?: number,
		maxBreadcrumbs?: number,
		enableLogs?: boolean,
	): void {
		console.log("Initializing Sentry via bridge...");

		const options: any = {
			dsn,
			...(debug !== undefined && { debug }),
			...(release !== undefined && { release }),
			...(dist !== undefined && { dist }),
			...(environment !== undefined && { environment }),
			...(sampleRate !== undefined && { sampleRate }),
			...(maxBreadcrumbs !== undefined && { maxBreadcrumbs }),
			...(enableLogs !== undefined && { enableLogs }),
		};

		Sentry.init(options);
	}

	/**
	 * Close Sentry
	 */
	close(): void {
		Sentry.close();
	}

	/**
	 * Check if Sentry is enabled
	 */
	isEnabled(): boolean {
		return Sentry.isEnabled();
	}

	/**
	 * Set context with JSON value
	 */
	setContext(key: string, valueJson: string): void {
		try {
			const value = JSON.parse(valueJson);
			Sentry.setContext(key, value);
		} catch (error) {
			console.error("Failed to parse context JSON:", error);
		}
	}

	/**
	 * Remove context
	 */
	removeContext(key: string): void {
		Sentry.setContext(key, null);
	}

	/**
	 * Set tag
	 */
	setTag(key: string, value: string): void {
		Sentry.setTag(key, value);
	}

	/**
	 * Remove tag
	 */
	removeTag(key: string): void {
		Sentry.setTag(key, undefined);
	}

	/**
	 * Set user information
	 */
	setUser(id?: string, username?: string, email?: string, ip?: string): void {
		const user: User = {};

		if (id && id !== "") {
			user.id = id;
		}
		if (username && username !== "") {
			user.username = username;
		}
		if (email && email !== "") {
			user.email = email;
		}
		if (ip && ip !== "") {
			user.ip_address = ip;
		}

		console.log("setting user ", user);
		Sentry.setUser(user);
	}

	/**
	 * Remove user
	 */
	removeUser(): void {
		Sentry.setUser(null);
	}

	/**
	 * Log trace message with attributes
	 */
	logTrace(message: string, attributesJson?: string): void {
		logger.trace(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log debug message with attributes
	 */
	logDebug(message: string, attributesJson?: string): void {
		logger.debug(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log info message with attributes
	 */
	logInfo(message: string, attributesJson?: string): void {
		logger.info(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log warning message with attributes
	 */
	logWarning(message: string, attributesJson?: string): void {
		logger.warning(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log error message with attributes
	 */
	logError(message: string, attributesJson?: string): void {
		logger.error(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log fatal message with attributes
	 */
	logFatal(message: string, attributesJson?: string): void {
		logger.fatal(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Capture message
	 */
	captureMessage(message: string): string {
		return Sentry.captureMessage(message);
	}

	/**
	 * Capture error with stacktrace
	 */
	captureError(message: string, stacktraceJson?: string): string {
		try {
			const stacktrace = safeParseJSON(stacktraceJson || "", undefined);
			const event: SentryEvent = {
				message,
			};

			if (stacktrace) {
				event.stacktrace = stacktrace;
			}

			return Sentry.captureEvent(event);
		} catch (error) {
			console.error("Failed to capture event:", error);
			return "";
		}
	}

	/**
	 * Get last event ID
	 */
	lastEventId(): string {
		return Sentry.lastEventId() || "";
	}

	/**
	 * Add breadcrumb
	 */
	addBreadcrumb(dataJson: string): void {
		try {
			const breadcrumb = safeParseJSON<Breadcrumb>(dataJson || "", {});
			Sentry.addBreadcrumb(breadcrumb);
		} catch (error) {
			console.error("Failed to add breadcrumb:", error);
		}
	}
}

// Create and export the bridge instance
const sentryBridge = new SentryBridgeImpl();

// Export to global window object to maintain compatibility with existing API
if (typeof window !== "undefined") {
	(window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
