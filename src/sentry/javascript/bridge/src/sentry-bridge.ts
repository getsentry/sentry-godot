import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";

// Stores bytes data passed from C++ layer with ID-based retrieval.
const BytesHandler = {
  _lastId: 0,
  _references: {} as Record<number, Uint8Array>,

  get(id: number): Uint8Array | undefined {
    return BytesHandler._references[id];
  },

  add(bytes: Uint8Array): number {
    const id = ++BytesHandler._lastId;
    BytesHandler._references[id] = bytes;
    return id;
  },

  remove(id: number): void {
    delete BytesHandler._references[id];
  },

  size(): number {
    return Object.keys(BytesHandler._references).length;
  },
};

// Stores information about attachment added from C++ layer during event processing.
// Attachment data is stored in BytesHandler and referenced by id.
interface AttachmentData {
  id: number;
  filename: string;
}

// *** Utility Functions

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

// *** SentryBridge

class SentryBridge {
  constructor() {}

  /**
   * Initialize Sentry with provided options
   */
  public init(
    beforeSendCallback: (event: Sentry.Event, outAttachments: Array<AttachmentData>) => void | null,
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
      beforeSend: (event: Sentry.Event, hint: Sentry.EventHint) => {
        // NOTE: Populated during processing in C++ layer
        var outAttachments: Array<AttachmentData> = [];

        beforeSendCallback(event, outAttachments);

        // console.debug("Byte buffers before adding attachments: ", BytesHandler.size());

        // Add attachments added during processing in C++ layer
        if (!hint.attachments) {
          hint.attachments = [];
        }
        for (var attachmentData of outAttachments) {
          var bytes = BytesHandler.get(attachmentData.id);
          if (bytes) {
            hint.attachments.push({
              data: bytes,
              filename: attachmentData.filename,
            });
            BytesHandler.remove(attachmentData.id);
          }
        }

        // console.debug("Byte buffers remaining after adding attachments: ", BytesHandler.size());

        var shouldDiscard: boolean = (event as any).shouldDiscard;
        delete (event as any).shouldDiscard;

        return shouldDiscard ? null : event;
      },
    };

    Sentry.init(options);
  }

  /**
   * Close Sentry
   */
  public close(): void {
    Sentry.close();
  }

  /**
   * Check if Sentry is enabled
   */
  public isEnabled(): boolean {
    return Sentry.isEnabled();
  }

  /**
   * Set context with JSON value
   */
  public setContext(key: string, valueJson: string): void {
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
  public removeContext(key: string): void {
    Sentry.setContext(key, null);
  }

  /**
   * Set tag
   */
  public setTag(key: string, value: string): void {
    Sentry.setTag(key, value);
  }

  /**
   * Remove tag
   */
  public removeTag(key: string): void {
    Sentry.setTag(key, undefined);
  }

  /**
   * Set user information
   */
  public setUser(id?: string, username?: string, email?: string, ip?: string): void {
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
  public removeUser(): void {
    Sentry.setUser(null);
  }

  /**
   * Log trace message with attributes
   */
  public logTrace(message: string, attributesJson?: string): void {
    Sentry.logger.trace(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Log debug message with attributes
   */
  public logDebug(message: string, attributesJson?: string): void {
    Sentry.logger.debug(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Log info message with attributes
   */
  public logInfo(message: string, attributesJson?: string): void {
    Sentry.logger.info(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Log warning message with attributes
   */
  public logWarn(message: string, attributesJson?: string): void {
    Sentry.logger.warn(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Log error message with attributes
   */
  public logError(message: string, attributesJson?: string): void {
    Sentry.logger.error(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Log fatal message with attributes
   */
  public logFatal(message: string, attributesJson?: string): void {
    Sentry.logger.fatal(message, parseAttributes(attributesJson || ""));
  }

  /**
   * Capture message
   */
  public captureMessage(message: string, level?: string): string {
    const sentryLevel = level as Sentry.SeverityLevel | undefined;
    return Sentry.captureMessage(message, sentryLevel);
  }

  /*
   * Capture event
   */
  public captureEvent(event: Sentry.Event): string {
    try {
      return Sentry.captureEvent(event);
    } catch (error) {
      console.error("Failed to capture event:", error);
      return "";
    }
  }

  /**
   * Capture user feedback
   */
  public captureFeedback(message: string, name?: string, email?: string, associatedEventId?: string): string {
    const feedback: any = { message };
    if (name) {
      feedback.name = name;
    }
    if (email) {
      feedback.email = email;
    }
    if (associatedEventId) {
      feedback.associatedEventId = associatedEventId;
    }
    return Sentry.captureFeedback(feedback);
  }

  /**
   * Get last event ID
   */
  public lastEventId(): string {
    return Sentry.lastEventId() || "";
  }

  /**
   * Add breadcrumb
   */
  public addBreadcrumb(crumb: Breadcrumb): void {
    try {
      Sentry.addBreadcrumb(crumb);
    } catch (error) {
      console.error("Failed to add breadcrumb:", error);
    }
  }

  /**
   * Add bytes attachment to the current scope
   */
  public addBytesAttachment(filename: string, bytes: Uint8Array, contentType: string): void {
    try {
      const attachment = {
        filename,
        data: bytes,
        contentType,
      };

      Sentry.getCurrentScope().addAttachment(attachment);
    } catch (error) {
      console.error("Failed to add bytes attachment:", error);
    }
  }

  /**
   * Merge properties from JSON content into target object
   */
  public mergeJsonIntoObject(target: object, jsonString: string): void {
    const source = safeParseJSON(jsonString, {});
    Object.assign(target, source);
  }

  /**
   * Deserialize object from JSON content and add it to target array
   */
  public pushJsonToArray(target: any[], jsonString: string): void {
    const item = safeParseJSON(jsonString, null);
    if (item !== null) {
      target.push(item);
    }
  }

  /**
   * Serialize object to JSON string
   */
  public objectToJson(obj: object): string {
    try {
      return JSON.stringify(obj);
    } catch (error) {
      console.error("Failed to stringify object:", error);
      return "{}";
    }
  }

  /**
   * Store bytes and return an ID for later retrieval
   */
  public addBytes(bytes: Uint8Array): number {
    return BytesHandler.add(bytes);
  }
}

const sentryBridge = new SentryBridge();

if (typeof window !== "undefined") {
  (window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
