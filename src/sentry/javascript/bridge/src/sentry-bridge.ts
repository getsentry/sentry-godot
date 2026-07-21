import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";
import type { Metric } from "@sentry/core";
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
  bytes: Uint8Array;
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

function makeUser(id: string, username: string, email: string, ip: string): User {
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

  return user;
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

  public pushAttachmentData(
    attachmentData: Array<AttachmentData>,
    bytes: Uint8Array,
    filename: string,
    contentType?: string,
    attachmentType?: string,
  ): void {
    attachmentData.push({
      bytes,
      filename,
      contentType,
      attachmentType,
    });
  }

  public init(
    beforeSendCallback: (event: Sentry.Event, outAttachments: Array<AttachmentData>) => void,
    beforeSendLogCallback: ((log: Sentry.Log) => void) | null,
    beforeSendMetricCallback: ((metric: Metric) => void) | null,
    dsn: string,
    debug: boolean,
    release: string,
    dist: string,
    environment: string,
    sampleRate: number,
    maxBreadcrumbs: number,
    enableLogs: boolean,
    enableMetrics: boolean,
    sendDefaultPii: boolean,
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
      enableMetrics,
      sendDefaultPii,
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
          if (attachmentData.bytes) {
            hint.attachments.push({
              data: attachmentData.bytes,
              filename: attachmentData.filename,
              ...(attachmentData.contentType && { contentType: attachmentData.contentType }),
              ...(attachmentData.attachmentType && { attachmentType: attachmentData.attachmentType }),
            } as any);
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

    if (beforeSendMetricCallback) {
      options.beforeSendMetric = (metric: Metric) => {
        beforeSendMetricCallback(metric);

        const shouldDiscard: boolean = (metric as any).shouldDiscard;
        delete (metric as any).shouldDiscard;

        return shouldDiscard ? null : metric;
      };
    } else {
      console.debug("Sentry: beforeSendMetric callback not provided.");
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
    Sentry.setUser(makeUser(id, username, email, ip));
  }

  public removeUser(): void {
    Sentry.setUser(null);
  }

  public eventSetUser(event: Sentry.Event, id: string, username: string, email: string, ip: string): void {
    event.user = makeUser(id, username, email, ip);
  }

  public createScope(): Sentry.Scope {
    const scope = new Sentry.Scope();
    scope.setClient(Sentry.getClient());
    scope.setPropagationContext(Sentry.getCurrentScope().getPropagationContext());
    return scope;
  }

  public scopeSetContext(scope: Sentry.Scope, key: string, valueJson: string): void {
    scope.setContext(key, safeParseJSON(valueJson, {}));
  }

  public scopeSetFingerprint(scope: Sentry.Scope, fingerprintJson: string): void {
    scope.setFingerprint(safeParseJSON<string[]>(fingerprintJson, []));
  }

  public scopeSetUser(scope: Sentry.Scope, id: string, username: string, email: string, ip: string): void {
    scope.setUser(makeUser(id, username, email, ip));
  }

  public scopeClear(scope: Sentry.Scope): void {
    // Preserve the propagation context across clear() because Scope.clear() rotates the trace in JS.
    const propagationContext = scope.getPropagationContext();
    scope.clear();
    scope.setPropagationContext(propagationContext);
  }

  public logTrace(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.trace(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public logDebug(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.debug(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public logInfo(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.info(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public logWarn(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.warn(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public logError(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.error(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public logFatal(message: string, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.logger.fatal(message, safeParseJSON(attributesJson || "", {}), { scope: scope ?? undefined });
  }

  public metricsAddCount(name: string, value: number, attributesJson?: string, scope?: Sentry.Scope): void {
    Sentry.metrics.count(name, value, {
      attributes: safeParseJSON(attributesJson || "", {}),
      scope: scope ?? undefined,
    });
  }

  public metricsAddGauge(
    name: string,
    value: number,
    unit: string,
    attributesJson?: string,
    scope?: Sentry.Scope,
  ): void {
    Sentry.metrics.gauge(name, value, {
      ...(unit !== "" && { unit }),
      attributes: safeParseJSON(attributesJson || "", {}),
      scope: scope ?? undefined,
    });
  }

  public metricsAddDistribution(
    name: string,
    value: number,
    unit: string,
    attributesJson?: string,
    scope?: Sentry.Scope,
  ): void {
    Sentry.metrics.distribution(name, value, {
      ...(unit !== "" && { unit }),
      attributes: safeParseJSON(attributesJson || "", {}),
      scope: scope ?? undefined,
    });
  }

  public setAttribute(name: string, value: any): void {
    // Value is sanitized at C++ boundary
    Sentry.getGlobalScope().setAttribute(name, value);
  }

  public removeAttribute(name: string): void {
    Sentry.getGlobalScope().removeAttribute(name);
  }

  public captureEvent(event: Sentry.Event, scope?: Sentry.Scope): string {
    if (!scope) {
      return Sentry.captureEvent(event);
    }
    // Ensure scoped captures use the active client. Scopes created before init
    // have no client and would otherwise drop events silently.
    scope.setClient(Sentry.getClient());
    return scope.captureEvent(event);
  }

  public captureFeedback(
    message: string,
    name: string,
    email: string,
    associatedEventId: string,
    scope?: Sentry.Scope,
  ): string {
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
    // Ensure scoped captures use the active client. Scopes created before init
    // have no client and would otherwise drop events silently.
    scope?.setClient(Sentry.getClient());
    return Sentry.captureFeedback(feedback, undefined, scope ?? undefined);
  }

  public lastEventId(): string {
    return Sentry.lastEventId() || "";
  }

  public addBreadcrumb(crumb: Breadcrumb): void {
    Sentry.addBreadcrumb(crumb);
  }

  public addBytesAttachment(filename: string, bytes: Uint8Array, contentType: string): void {
    Sentry.getIsolationScope().addAttachment({
      filename,
      data: bytes,
      contentType,
    });
  }

  public clearAttachments(): void {
    Sentry.getIsolationScope().clearAttachments();
  }
}

const sentryBridge = new SentryBridge();

if (typeof window !== "undefined") {
  (window as any).SentryBridge = sentryBridge;
}

export default sentryBridge;
