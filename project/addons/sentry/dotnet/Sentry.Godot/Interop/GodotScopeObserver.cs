using System;
using Sentry;

namespace Sentry.Godot.Interop;

/// <summary>
/// Scope Observer to sync changes to native layer.
/// </summary>
internal class GodotScopeObserver : IScopeObserver {
	[ThreadStatic]
	private static bool _addingBreadcrumb;
	[ThreadStatic]
	private static bool _settingTag;
	[ThreadStatic]
	private static bool _unsettingTag;
	[ThreadStatic]
	private static bool _settingUser;
	[ThreadStatic]
	private static bool _settingTraceId;

	public void AddBreadcrumb(Breadcrumb breadcrumb) {
		if (_addingBreadcrumb) {
			return;
		}
		_addingBreadcrumb = true;
		try {
			NativeBridge.AddBreadcrumb(breadcrumb);
		} finally {
			_addingBreadcrumb = false;
		}
	}

	public void SetExtra(string key, object? value) {
		// NOTE: Godot SDK doesn't support extras.
	}

	public void SetTag(string key, string value) {
		if (_settingTag) {
			return;
		}
		_settingTag = true;
		try {
			NativeBridge.SetTag(key, value);
		} finally {
			_settingTag = false;
		}
	}

	public void UnsetTag(string key) {
		if (_unsettingTag) {
			return;
		}
		_unsettingTag = true;
		try {
			NativeBridge.RemoveTag(key);
		} finally {
			_unsettingTag = false;
		}
	}

	public void SetUser(SentryUser? user) {
		if (_settingUser) {
			return;
		}
		_settingUser = true;
		try {
			NativeBridge.SetUser(user);
		} finally {
			_settingUser = false;
		}
	}

	public void SetTrace(SentryId traceId, SpanId parentSpanId) {
		if (_settingTraceId) {
			return;
		}
		_settingTraceId = true;
		try {
			NativeBridge.SetTrace(traceId.ToString(), parentSpanId.ToString());
		} finally {
			_settingTraceId = false;
		}
	}
}
