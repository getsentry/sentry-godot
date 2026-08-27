import * as Sentry from "@sentry/browser";
import type { Breadcrumb, User } from "@sentry/browser";
import { _INTERNAL_setSpanForScope, generateSpanId } from "@sentry/core";
import type { Attachment, Metric } from "@sentry/core";
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

// The JS SDK has no notion of file attachments - it only takes bytes. A file is therefore added as an
// attachment object marked with this property, which holds the path, and the C++ layer fills in the
// bytes when an event is captured. Sentry copies only its own known fields into the envelope, so the
// marker is never sent.
const PENDING_PATH_KEY = "__godotPath";

// SentrySpan.SpanStatus value for a failed span, as the C++ layer sends it.
const SPAN_STATUS_ERROR = 1;

// OpenTelemetry status codes, which @sentry/core expects but does not export as constants.
const OTEL_CODE_OK = 1;
const OTEL_CODE_ERROR = 2;

// Handed to the C++ layer to have it read one file; it leaves the bytes unset if the read fails.
interface AttachmentRequest {
  path: string;
  data?: Uint8Array;
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

function makeAttachment(filename: string, bytes: Uint8Array, contentType: string, attachmentType: string): Attachment {
  return {
    filename,
    data: bytes,
    ...(contentType && { contentType }),
    ...(attachmentType && { attachmentType }),
  } as Attachment;
}

// Builds an attachment whose bytes the C++ layer reads from the path when an event is captured.
function makePendingAttachment(path: string, filename: string, contentType: string, attachmentType: string): Attachment {
  return {
    ...makeAttachment(filename, new Uint8Array(0), contentType, attachmentType),
    [PENDING_PATH_KEY]: path,
  } as Attachment;
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

  public init(
    beforeSendCallback: (event: Sentry.Event) => void,
    beforeSendFeedbackCallback: (event: Sentry.Event) => void,
    beforeSendLogCallback: ((log: Sentry.Log) => void) | null,
    beforeSendMetricCallback: ((metric: Metric) => void) | null,
    readAttachmentCallback: (request: AttachmentRequest) => void,
    dsn: string,
    debug: boolean,
    release: string,
    dist: string,
    environment: string,
    sampleRate: number,
    tracesSampleRate: number,
    traceLifecycle: number,
    maxBreadcrumbs: number,
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
      tracesSampleRate,
      traceLifecycle: traceLifecycle === 1 ? "stream" : "static",
      maxBreadcrumbs,
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
        if (traceLifecycle === 1) {
          filtered.push(Sentry.spanStreamingIntegration());
        }
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
      options.beforeSend = (event: Sentry.Event) => {
        if (!this.isEnabled()) {
          // SDK is disabled, skip processing.
          return null;
        }

        beforeSendCallback(event);

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

    // The scopes outlive Sentry.close(), so without this a previous session's data would leak into
    // this one: tags, breadcrumbs and attachments from the isolation scope, attributes from the
    // global one, and the trace that captures and forked scopes inherit from the current one.
    // Clearing here is safe even if a close() flush is still in flight: scope data and attachments are
    // merged into the event synchronously at capture time, so nothing already captured reads a scope again.
    Sentry.getIsolationScope().clear();
    Sentry.getGlobalScope().clear();
    Sentry.getCurrentScope().clear();

    Sentry.init(options);

    // Use one stable fallback span ID for captures without an active span.
    Sentry.getCurrentScope().setPropagationContext({
      ...Sentry.getCurrentScope().getPropagationContext(),
      propagationSpanId: generateSpanId(),
    });

    if (beforeSendFeedbackCallback) {
      // @sentry/core has no beforeSendFeedback option, and feedback also bypasses beforeSend.
      Sentry.getClient()?.addEventProcessor((event: Sentry.Event) => {
        if (event.type !== "feedback") {
          return event;
        }

        if (!this.isEnabled()) {
          return null;
        }

        beforeSendFeedbackCallback(event);

        const shouldDiscard: boolean = (event as any).shouldDiscard;
        delete (event as any).shouldDiscard;

        return shouldDiscard ? null : event;
      });
    } else {
      console.error(
        "Sentry: beforeSendFeedback callback is missing. User feedback will be sent without native-side processing; this is unexpected and likely indicates the bridge failed to initialize correctly.",
      );
    }

    if (readAttachmentCallback) {
      // Runs for every event type right before the attachments become envelope items, whereas
      // beforeSend only sees error events and would leave feedback captures with empty bytes.
      Sentry.getClient()?.on("beforeSendEvent", (_event: Sentry.Event, hint?: Sentry.EventHint) => {
        if (!hint?.attachments?.some((attachment: any) => attachment[PENDING_PATH_KEY])) {
          return;
        }

        const outgoing: Array<any> = [];
        for (const attachment of hint.attachments as Array<any>) {
          const path = attachment[PENDING_PATH_KEY];
          if (!path) {
            outgoing.push(attachment);
            continue;
          }

          // The C++ layer fills in the bytes, and leaves the request untouched if it cannot read the file.
          const request: AttachmentRequest = { path };
          readAttachmentCallback(request);
          if (!request.data) {
            // Not a warning: some attachments are expected to be missing.
            console.debug(`Sentry: dropping attachment - file could not be read: ${path}`);
            continue;
          }

          // Copy before adding bytes so scoped attachments don't retain them for future captures.
          const resolved = { ...attachment, data: request.data };
          delete resolved[PENDING_PATH_KEY];
          outgoing.push(resolved);
        }
        hint.attachments = outgoing;
      });
    } else {
      console.error("Sentry: Internal error: attachment read callback is missing. File attachments added to a scope will be empty.");
    }
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

  public scopeAddBytesAttachment(
    scope: Sentry.Scope,
    filename: string,
    bytes: Uint8Array,
    contentType: string,
    attachmentType: string,
  ): void {
    scope.addAttachment(makeAttachment(filename, bytes, contentType, attachmentType));
  }

  public scopeAddFileAttachment(
    scope: Sentry.Scope,
    path: string,
    filename: string,
    contentType: string,
    attachmentType: string,
  ): void {
    scope.addAttachment(makePendingAttachment(path, filename, contentType, attachmentType));
  }

  public scopeClear(scope: Sentry.Scope): Sentry.Scope {
    const fresh = new Sentry.Scope();
    fresh.setClient(scope.getClient() ?? Sentry.getClient());
    fresh.setPropagationContext(scope.getPropagationContext());
    return fresh;
  }

  public scopeSetSpan(scope: Sentry.Scope, span?: Sentry.Span): void {
    // No public alternative: setActiveSpanInBrowser binds only to the current scope.
    _INTERNAL_setSpanForScope(scope, span ?? undefined);
  }

  public startSpan(name: string, attributesJson: string, parentSpan?: Sentry.Span): Sentry.Span {
    return Sentry.startInactiveSpan({
      name,
      attributes: safeParseJSON(attributesJson, {}),
      parentSpan: parentSpan ?? null,
    });
  }

  public spanSetStatus(span: Sentry.Span, status: number): void {
    span.setStatus({ code: status === SPAN_STATUS_ERROR ? OTEL_CODE_ERROR : OTEL_CODE_OK });
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
    return Sentry.captureFeedback(feedback, undefined, scope ?? undefined);
  }

  public lastEventId(): string {
    return Sentry.lastEventId() || "";
  }

  public addBreadcrumb(crumb: Breadcrumb): void {
    Sentry.addBreadcrumb(crumb);
  }

  public addBytesAttachment(filename: string, bytes: Uint8Array, contentType: string, attachmentType: string): void {
    Sentry.getIsolationScope().addAttachment(makeAttachment(filename, bytes, contentType, attachmentType));
  }

  public addFileAttachment(path: string, filename: string, contentType: string, attachmentType: string): void {
    Sentry.getIsolationScope().addAttachment(makePendingAttachment(path, filename, contentType, attachmentType));
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
