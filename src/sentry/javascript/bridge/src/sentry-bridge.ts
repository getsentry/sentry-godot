import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";
import { wasmIntegration } from "@sentry/wasm";

// Stores byte buffers passed from C++ layer with ID-based retrieval.
// Uses uint32_t range (0 to 4294967295) to stay safe within JavaScript's integer precision.
class ByteStore {
  private _lastId = 0;
  private _buffers = new Map<number, Uint8Array>();

  public get(id: number): Uint8Array | undefined {
    return this._buffers.get(id);
  }

  public add(bytes: Uint8Array): number {
    // Wrap around within uint32_t range (0 is reserved for error)
    this._lastId = (this._lastId % 0xffffffff) + 1;
    this._buffers.set(this._lastId, bytes);
    return this._lastId;
  }

  public remove(id: number): void {
    this._buffers.delete(id);
  }

  public size(): number {
    return this._buffers.size;
  }

  public clear(): void {
    this._buffers.clear();
  }
}

// Stores info about attachments loaded from C++ layer during event processing.
interface AttachmentData {
  id: number; // the content is stored in ByteStore and referenced by this id.
  filename: string;
  contentType?: string;
  attachmentType?: string;
}

// *** Utility Functions

function safeParseJSON<T = any>(json: string, fallback: T): T {
  if (json === "") {
    return fallback;
  }

  try {
    return JSON.parse(json);
  } catch (error) {
    console.error("Failed to parse JSON:", error);
    return fallback;
  }
}

// *** SentryBridge

class SentryBridge {
  constructor() {}

  private _byteStore = new ByteStore();

  public init(
    beforeSendCallback: (event: Sentry.Event, outAttachments: Array<AttachmentData>) => void,
    beforeSendLogCallback: ((log: Sentry.Log) => void) | null,
    dsn: string,
    debug: boolean,
    release: string,
    dist: string,
    environment: string,
    sampleRate: number,
    maxBreadcrumbs: number,
    enableLogs: boolean,
    sdkVersion: string,
  ): void {
    if (debug) {
      console.log("Initializing Sentry via bridge...");
    }

    const options: any = {
      dsn,
      debug,
      release,
      dist,
      environment,
      sampleRate,
      maxBreadcrumbs,
      enableLogs,
      _metadata: {
        sdk: {
          name: "sentry.javascript.godot",
          version: sdkVersion,
        },
      },

      integrations: function (integrations: { name: string }[]) {
        const excludedIntegrations = [
          "Dedupe", // prevents the same message event from being sent twice in a row; since we don't include stacktraces with messages yet, different call sites can look identical and be dropped
          "Breadcrumbs", // added later with custom settings
        ];
        const filtered = integrations.filter(function (integration: { name: string }) {
          return !excludedIntegrations.includes(integration.name);
        });
        filtered.push(wasmIntegration());
        filtered.push(
          Sentry.breadcrumbsIntegration({
            console: false, // very noisy in Godot SDK
            dom: false, // dom clicks are not informative in games
            fetch: true,
            history: true,
            sentry: false, // redundant - doubles our functionality
            xhr: true,
          }),
        );
        return filtered;
      },
    };

    if (beforeSendCallback) {
      options.beforeSend = (event: Sentry.Event, hint: Sentry.EventHint) => {
        // NOTE: Populated during processing in C++ layer
        const outAttachments: Array<AttachmentData> = [];

        beforeSendCallback(event, outAttachments);

        // Add attachments loaded from the C++ layer during event processing
        if (!hint.attachments) {
          hint.attachments = [];
        }
        for (const attachmentData of outAttachments) {
          const bytes = this._byteStore.get(attachmentData.id);
          if (bytes) {
            hint.attachments.push({
              data: bytes,
              filename: attachmentData.filename,
              ...(attachmentData.contentType && { contentType: attachmentData.contentType }),
              ...(attachmentData.attachmentType && { attachmentType: attachmentData.attachmentType }),
            } as any);
            this._byteStore.remove(attachmentData.id);
          }
        }

        const shouldDiscard: boolean = (event as any).shouldDiscard;
        delete (event as any).shouldDiscard;

        return shouldDiscard ? null : event;
      };
    } else {
      console.error(
        "Sentry: beforeSend callback is missing. Events will be sent without native-side processing; this is unexpected and likely indicates the bridge failed to initialize correctly.",
      );
    }

    // beforeSendLogCallback may be null when no user-provided callback is configured.
    if (beforeSendLogCallback) {
      options.beforeSendLog = (log: Sentry.Log) => {
        beforeSendLogCallback(log);

        const shouldDiscard: boolean = (log as any).shouldDiscard;
        delete (log as any).shouldDiscard;

        return shouldDiscard ? null : log;
      };
    } else {
      console.debug("Sentry: beforeSendLog callback not provided.");
    }

    Sentry.init(options);
  }

  public close(): void {
    Sentry.close();
    this._byteStore.clear();
  }

  public isEnabled(): boolean {
    return Sentry.isEnabled();
  }

  public setContext(key: string, valueJson: string): void {
    Sentry.setContext(key, safeParseJSON(valueJson, {}));
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

  public setUser(id: string, username: string, email: string, ip: string): void {
    const user: User = {};

    if (id !== "") {
      user.id = id;
    }
    if (username !== "") {
      user.username = username;
    }
    if (email !== "") {
      user.email = email;
    }
    if (ip !== "") {
      user.ip_address = ip;
    }

    Sentry.setUser(user);
  }

  public removeUser(): void {
    Sentry.setUser(null);
  }

  public logTrace(message: string, attributesJson?: string): void {
    Sentry.logger.trace(message, safeParseJSON(attributesJson || "", {}));
  }

  public logDebug(message: string, attributesJson?: string): void {
    Sentry.logger.debug(message, safeParseJSON(attributesJson || "", {}));
  }

  public logInfo(message: string, attributesJson?: string): void {
    Sentry.logger.info(message, safeParseJSON(attributesJson || "", {}));
  }

  public logWarn(message: string, attributesJson?: string): void {
    Sentry.logger.warn(message, safeParseJSON(attributesJson || "", {}));
  }

  public logError(message: string, attributesJson?: string): void {
    Sentry.logger.error(message, safeParseJSON(attributesJson || "", {}));
  }

  public logFatal(message: string, attributesJson?: string): void {
    Sentry.logger.fatal(message, safeParseJSON(attributesJson || "", {}));
  }

  public captureMessage(message: string, level: string): string {
    return Sentry.captureMessage(message, level as Sentry.SeverityLevel);
  }

  public captureEvent(event: Sentry.Event): string {
    return Sentry.captureEvent(event);
  }

  public captureFeedback(message: string, name: string, email: string, associatedEventId: string): string {
    const feedback: any = { message };
    if (name !== "") {
      feedback.name = name;
    }
    if (email !== "") {
      feedback.email = email;
    }
    if (associatedEventId !== "") {
      feedback.associatedEventId = associatedEventId;
    }
    return Sentry.captureFeedback(feedback);
  }

  public lastEventId(): string {
    return Sentry.lastEventId() || "";
  }

  public addBreadcrumb(crumb: Breadcrumb): void {
    Sentry.addBreadcrumb(crumb);
  }

  public addBytesAttachment(filename: string, bytes: Uint8Array, contentType: string): void {
    Sentry.getCurrentScope().addAttachment({
      filename,
      data: bytes,
      contentType,
    });
  }

  // *** Native-JS interop helpers

  public storeBytes(bytes: Uint8Array): number {
    return this._byteStore.add(bytes);
  }

  public mergeJsonIntoObject(obj: object, jsonString: string): void {
    const source = safeParseJSON(jsonString, {});
    Object.assign(obj, source);
  }

  public pushJsonObjectToArray(arr: any[], jsonString: string): void {
    const item = safeParseJSON(jsonString, null);
    if (item !== null) {
      arr.push(item);
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

  // Gets double property as string to preserve precision across the JS/C++ boundary.
  // NOTE: Numbers lose precision when crossing JS boundary in current Godot bindings.
  public getDoubleAsString(obj: object, prop: string): string {
    const value = (obj as Record<string, unknown>)[prop];
    if (typeof value === "number") {
      return value.toString();
    }
    return "";
  }

  // Sets double property from string to preserve precision across the JS/C++ boundary.
  public setDoubleFromString(obj: object, prop: string, valueStr: string): void {
    const value = parseFloat(valueStr);
    if (!isNaN(value)) {
      (obj as Record<string, unknown>)[prop] = value;
    }
  }
}

const sentryBridge = new SentryBridge();

if (typeof window !== "undefined") {
  (window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
