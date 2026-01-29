import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";

// Stores bytes data passed from C++ layer with ID-based retrieval.
// Uses uint32_t range (0 to 4294967295) to stay safe within JavaScript's integer precision.
const BytesHandler = {
  _lastId: 0,
  _refs: {} as Record<number, Uint8Array>,

  get(id: number): Uint8Array | undefined {
    return BytesHandler._refs[id];
  },

  add(bytes: Uint8Array): number {
    // Wrap around within uint32_t range (0 is reserved for error)
    BytesHandler._lastId = (BytesHandler._lastId % 0xffffffff) + 1;
    BytesHandler._refs[BytesHandler._lastId] = bytes;
    return BytesHandler._lastId;
  },

  remove(id: number): void {
    delete BytesHandler._refs[id];
  },

  size(): number {
    return Object.keys(BytesHandler._refs).length;
  },
};

// Stores info about attachments loaded from C++ layer during event processing.
// The content is stored in BytesHandler and referenced by id.
interface AttachmentData {
  id: number;
  filename: string;
  contentType?: string;
  attachmentType?: string;
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

        // Add attachments loaded from the C++ layer during event processing
        if (!hint.attachments) {
          hint.attachments = [];
        }
        for (var attachmentData of outAttachments) {
          var bytes = BytesHandler.get(attachmentData.id);
          if (bytes) {
            hint.attachments.push({
              data: bytes,
              filename: attachmentData.filename,
              ...(attachmentData.contentType && { contentType: attachmentData.contentType }),
              ...(attachmentData.attachmentType && { attachmentType: attachmentData.attachmentType }),
            } as any);
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

  public close(): void {
    Sentry.close();
  }

  public isEnabled(): boolean {
    return Sentry.isEnabled();
  }

  public setContext(key: string, valueJson: string): void {
    try {
      const value = JSON.parse(valueJson);
      Sentry.setContext(key, value);
    } catch (error) {
      console.error("Failed to parse context JSON:", error);
    }
  }

  public removeContext(key: string): void {
    Sentry.setContext(key, null);
  }

  public setTag(key: string, value: string): void {
    Sentry.setTag(key, value);
  }

  public removeTag(key: string): void {
    Sentry.setTag(key, undefined);
  }

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

    Sentry.setUser(user);
  }

  public removeUser(): void {
    Sentry.setUser(null);
  }

  public logTrace(message: string, attributesJson?: string): void {
    Sentry.logger.trace(message, parseAttributes(attributesJson || ""));
  }

  public logDebug(message: string, attributesJson?: string): void {
    Sentry.logger.debug(message, parseAttributes(attributesJson || ""));
  }

  public logInfo(message: string, attributesJson?: string): void {
    Sentry.logger.info(message, parseAttributes(attributesJson || ""));
  }

  public logWarn(message: string, attributesJson?: string): void {
    Sentry.logger.warn(message, parseAttributes(attributesJson || ""));
  }

  public logError(message: string, attributesJson?: string): void {
    Sentry.logger.error(message, parseAttributes(attributesJson || ""));
  }

  public logFatal(message: string, attributesJson?: string): void {
    Sentry.logger.fatal(message, parseAttributes(attributesJson || ""));
  }

  public captureMessage(message: string, level?: string): string {
    const sentryLevel = level as Sentry.SeverityLevel | undefined;
    return Sentry.captureMessage(message, sentryLevel);
  }

  public captureEvent(event: Sentry.Event): string {
    try {
      return Sentry.captureEvent(event);
    } catch (error) {
      console.error("Failed to capture event:", error);
      return "";
    }
  }

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

  public lastEventId(): string {
    return Sentry.lastEventId() || "";
  }

  public addBreadcrumb(crumb: Breadcrumb): void {
    try {
      Sentry.addBreadcrumb(crumb);
    } catch (error) {
      console.error("Failed to add breadcrumb:", error);
    }
  }

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

  public mergeJsonIntoObject(target: object, jsonString: string): void {
    const source = safeParseJSON(jsonString, {});
    Object.assign(target, source);
  }

  public pushJsonObjectToArray(target: any[], jsonString: string): void {
    const item = safeParseJSON(jsonString, null);
    if (item !== null) {
      target.push(item);
    }
  }

  public objectToJson(obj: object): string {
    try {
      return JSON.stringify(obj);
    } catch (error) {
      console.error("Failed to stringify object:", error);
      return "{}";
    }
  }

  public addBytes(bytes: Uint8Array): number {
    return BytesHandler.add(bytes);
  }
}

const sentryBridge = new SentryBridge();

if (typeof window !== "undefined") {
  (window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
