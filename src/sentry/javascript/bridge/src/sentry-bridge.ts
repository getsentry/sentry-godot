// Type Definitions
import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";

interface SentryEvent {
	message?: string;
	stacktrace?: any;
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
	captureEvent(event: Sentry.Event): string;
	lastEventId(): string;
	addBreadcrumb(crumb: Breadcrumb): void;
	mergeJsonIntoObject(target: object, jsonString: string): void;
	pushJsonToArray(target: any[], jsonString: string): void;
	objectToJson(obj: object): string;
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
		Sentry.logger.trace(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log debug message with attributes
	 */
	logDebug(message: string, attributesJson?: string): void {
		Sentry.logger.debug(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log info message with attributes
	 */
	logInfo(message: string, attributesJson?: string): void {
		Sentry.logger.info(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log warning message with attributes
	 */
	logWarning(message: string, attributesJson?: string): void {
		Sentry.logger.warn(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log error message with attributes
	 */
	logError(message: string, attributesJson?: string): void {
		Sentry.logger.error(message, parseAttributes(attributesJson || ""));
	}

	/**
	 * Log fatal message with attributes
	 */
	logFatal(message: string, attributesJson?: string): void {
		Sentry.logger.fatal(message, parseAttributes(attributesJson || ""));
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

	/*
	 * Capture event
	 */
	captureEvent(event: Sentry.Event): string {
		try {
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
	addBreadcrumb(crumb: Breadcrumb): void {
		try {
			Sentry.addBreadcrumb(crumb);
		} catch (error) {
			console.error("Failed to add breadcrumb:", error);
		}
	}

	/**
	 * Merge JSON content into a target object
	 */
	mergeJsonIntoObject(target: object, jsonString: string): void {
		const source = safeParseJSON(jsonString, {});
		Object.assign(target, source);
	}

	/**
	 * Push JSON content to a target array
	 */
	pushJsonToArray(target: any[], jsonString: string): void {
		const item = safeParseJSON(jsonString, null);
		if (item !== null) {
			target.push(item);
		}
	}

	/**
	 * Convert object to JSON string
	 */
	objectToJson(obj: object): string {
		try {
			return JSON.stringify(obj);
		} catch (error) {
			console.error("Failed to stringify object:", error);
			return "{}";
		}
	}
}

// Create and export the bridge instance
const sentryBridge = new SentryBridgeImpl();

// Export to global window object to maintain compatibility with existing API
if (typeof window !== "undefined") {
	(window as any).SentryBridge = sentryBridge;
	(window as any).Sentry = Sentry;
}

export default sentryBridge;
