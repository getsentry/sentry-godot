import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";
import { wasmIntegration } from "@sentry/wasm";

// ID-based store for WASM/JS interop. Assigns auto-incrementing uint32 IDs (0 is reserved).
class IdStore<T> {
  private _lastId = 0;
  private _items = new Map<number, T>();

  public store(item: T): number {
    let id = this._lastId;
    do {
      id = (id % 0xffffffff) + 1;
    } while (this._items.has(id));
    this._lastId = id;
    this._items.set(id, item);
    return id;
  }

  public get(id: number): T | undefined {
    return this._items.get(id);
  }

  public release(id: number): void {
    this._items.delete(id);
  }

  public clear(): void {
    this._items.clear();
  }
}

// Stores info about attachments loaded from C++ layer during event processing.
interface AttachmentData {
  id: number; // the content is stored in the byte store and referenced by this id.
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

  private _byteStore = new IdStore<Uint8Array>();
  private _objectStore = new IdStore<object>();

  public storeBytes(bytes: Uint8Array): number {
    return this._byteStore.store(bytes);
  }

  public takeBytes(id: number): Uint8Array | undefined {
    const bytes = this._byteStore.get(id);
    this._byteStore.release(id);
    return bytes;
  }

  public releaseBytes(id: number): void {
    this._byteStore.release(id);
  }

  public storeObject(obj: any): number {
    return this._objectStore.store(obj);
  }

  public getObject(id: number): any {
    return this._objectStore.get(id);
  }

  public releaseObject(id: number): void {
    this._objectStore.release(id);
  }

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
        if (!this.isEnabled()) {
          // SDK is disabled, skip processing.
          return null;
        }

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
            this._byteStore.release(attachmentData.id);
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

  public close(timeout: number): void {
    Sentry.close(timeout);
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
}

const sentryBridge = new SentryBridge();

if (typeof window !== "undefined") {
  (window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
