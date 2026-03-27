using System;
using Godot;

namespace Sentry.Godot.Internal;

#pragma warning disable IDE1006 // ignore property naming conventions

// Caches commonly used StringName values
internal static class StringNames {
	public static readonly StringName SentrySDK = "SentrySDK";
	public static readonly StringName create_breadcrumb = "create_breadcrumb";
	public static readonly StringName message = "message";
	public static readonly StringName category = "category";
	public static readonly StringName type = "type";
	public static readonly StringName data = "data";
	public static readonly StringName level = "level";
	public static readonly StringName add_breadcrumb = "add_breadcrumb";
	public static readonly StringName create = "create";
	public static readonly StringName SentryBreadcrumb = "SentryBreadcrumb";
	public static readonly StringName remove_user = "remove_user";
	public static readonly StringName SentryUser = "SentryUser";
	public static readonly StringName username = "username";
	public static readonly StringName email = "email";
	public static readonly StringName id = "id";
	public static readonly StringName ip_address = "ip_address";
	public static readonly StringName set_user = "set_user";
	public static readonly StringName _get_last_user = "_get_last_user";
}
