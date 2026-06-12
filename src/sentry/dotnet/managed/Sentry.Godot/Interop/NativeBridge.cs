using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using Sentry.Godot.Internal;
using Sentry.Protocol;

namespace Sentry.Godot.Interop;

/// <summary>
/// Handles native layer SDK operations via P/Invoke.
/// </summary>
internal static partial class NativeBridge
{
    internal const string Lib = "sentry-godot";

    private const int SizeOfChar32 = 4;

    [LibraryImport(Lib)]
    private static partial void csharp_interop_string_free(IntPtr handle);

    // Must match layout of GodotStringHandle in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private struct GodotStringHandle
    {
        public IntPtr Ptr;
        public long Len;
        public IntPtr Handle;

        public readonly unsafe string? TakeString()
        {
            try
            {
                if (Ptr == IntPtr.Zero)
                {
                    return null;
                }
                return Encoding.UTF32.GetString((byte*)Ptr, (int)Len * SizeOfChar32);
            }
            finally
            {
                csharp_interop_string_free(Handle);
            }
        }
    }

    // Must match layout of NativeArray in csharp_interop.cpp.
    // Cast Ptr to the concrete element type. Dispose frees the array.
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeArray : IDisposable
    {
        public IntPtr Ptr;
        public int Count;

        public void Dispose()
        {
            if (Ptr != IntPtr.Zero)
            {
                csharp_interop_free_array(Ptr);
                Ptr = IntPtr.Zero;
            }
        }
    }

    [LibraryImport(Lib)]
    private static partial void csharp_interop_free_array(IntPtr array);

    // Must match layout of LoggerLimitsData in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private struct LoggerLimitsData
    {
        public int events_per_frame;
        public int repeated_error_window_ms;
        public int throttle_events;
        public int throttle_window_ms;
    }

    // Must match layout of AttachmentMeta in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private struct AttachmentMeta
    {
        public GodotStringHandle path;
        public GodotStringHandle filename;
        public GodotStringHandle content_type;
        public GodotStringHandle attachment_type;
    }

    // Must match layout of NativeOptions in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeOptions
    {
        public LoggerLimitsData logger_limits;
        public GodotStringHandle dsn;
        public GodotStringHandle release;
        public GodotStringHandle dist;
        public GodotStringHandle environment;
        public byte debug;
        public int diagnostic_level;
        public double sample_rate;
        public int max_breadcrumbs;
        public double shutdown_timeout_ms;
        public byte send_default_pii;
        public byte enable_logs;
        public byte attach_log;
        public byte attach_scene_tree;
        public byte attach_screenshot;
        public int screenshot_level;
        public byte enable_app_hang_tracking;
        public int app_hang_timeout_ms;
        public byte logger_enabled;
        public byte logger_include_source;
        public byte logger_include_variables;
        public int logger_event_mask;
        public int logger_breadcrumb_mask;
        public int logger_log_mask;
        public byte enable_metrics;
        public byte android_enable_anr_detection;
        public int android_anr_timeout_interval_ms;
        public byte android_attach_anr_thread_dump;
    }

    // Must match layout of ManagedOptions in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct ManagedOptions
    {
        public LoggerLimitsData logger_limits;
        public char* dsn;
        public int dsn_len;
        public char* release;
        public int release_len;
        public char* dist;
        public int dist_len;
        public char* environment;
        public int environment_len;
        public byte debug;
        public int diagnostic_level;
        public double sample_rate;
        public int max_breadcrumbs;
        public double shutdown_timeout_ms;
        public byte send_default_pii;
        public byte enable_logs;
        public byte attach_log;
        public byte attach_scene_tree;
        public byte attach_screenshot;
        public int screenshot_level;
        public byte enable_app_hang_tracking;
        public int app_hang_timeout_ms;
        public byte logger_enabled;
        public byte logger_include_source;
        public byte logger_include_variables;
        public int logger_event_mask;
        public int logger_breadcrumb_mask;
        public int logger_log_mask;
        public byte enable_metrics;
        public byte android_enable_anr_detection;
        public int android_anr_timeout_interval_ms;
        public byte android_attach_anr_thread_dump;
    }

    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct NativeTraceContext
    {
        public GodotStringHandle trace_id;
        public GodotStringHandle parent_span_id;
    }

    // Managed-owned string map for passing Dictionary<string, string> across P/Invoke.
    // Buffer layout: key1+val1+key2+val2... concatenated as UTF-16.
    // Lengths array: key1_len, val1_len, key2_len, val2_len, ... (2*pair_count entries).
    // Must match layout of ManagedStringMap in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct ManagedStringMap
    {
        public char* Buffer;
        public int* Lengths;
        public int PairCount;
    }

    private delegate void ManagedStringMapAction(ManagedStringMap map);

    /// <summary>
    /// Marshals a dictionary into a stack-allocated interleaved buffer and passes the resulting struct to a native
    /// function call. The map is only valid for the duration of the call (owned by managed code).
    /// </summary>
    private static unsafe void MarshallStringMap(IReadOnlyDictionary<string, string>? dict, ManagedStringMapAction nativeFunc)
    {
        if (dict == null || dict.Count == 0)
        {
            nativeFunc(default);
            return;
        }

        int pairCount = dict.Count;
        Span<int> lengthSpan = pairCount * 2 <= 256
                ? stackalloc int[pairCount * 2]
                : new int[pairCount * 2];

        int totalChars = 0;
        foreach (var kv in dict)
        {
            totalChars += kv.Key.Length + kv.Value.Length;
        }

        // Allocate at least one char so `fixed` produces a non-null pointer for empty pairs.
        int bufferLen = Math.Max(totalChars, 1);
        Span<char> buffer = bufferLen <= 512
                ? stackalloc char[bufferLen]
                : new char[bufferLen];

        int i = 0;
        int pos = 0;
        foreach (var kv in dict)
        {
            lengthSpan[i++] = kv.Key.Length;
            lengthSpan[i++] = kv.Value.Length;
            kv.Key.AsSpan().CopyTo(buffer.Slice(pos));
            pos += kv.Key.Length;
            kv.Value.AsSpan().CopyTo(buffer.Slice(pos));
            pos += kv.Value.Length;
        }

        fixed (char* bufPtr = buffer)
        fixed (int* lenPtr = lengthSpan)
        {
            var map = new ManagedStringMap
            {
                Buffer = bufPtr,
                Lengths = lenPtr,
                PairCount = pairCount
            };
            nativeFunc(map);
        }
    }

    // Must match ManagedFunctions struct in csharp_interop.cpp
    [StructLayout(LayoutKind.Sequential)]
    private unsafe struct ManagedFunctions
    {
        public delegate* unmanaged[Cdecl]<void> init;
        public delegate* unmanaged[Cdecl]<void> close;
        public delegate* unmanaged[Cdecl]<char*, int, char*, int, void> logger_error;
        public delegate* unmanaged[Cdecl]<char*, int, char*, int, char*, int, int, void> add_breadcrumb;
        public delegate* unmanaged[Cdecl]<char*, int, char*, int, void> set_tag;
        public delegate* unmanaged[Cdecl]<char*, int, void> remove_tag;
        public delegate* unmanaged[Cdecl]<char*, int, char*, int, char*, int, char*, int, void> set_user;
        public delegate* unmanaged[Cdecl]<void> remove_user;
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_register_managed_functions(
        ManagedFunctions functions);

    /// <summary>
    /// Checks whether the native interop library is loaded and exposes the expected symbol.
    /// Returns false when running outside the Godot/GDExtension host, so initialization can be skipped.
    /// </summary>
    public static bool IsNativeAvailable()
    {
        try
        {
            return NativeLibrary.TryGetExport(NativeLibResolver.Resolve(),
                nameof(csharp_interop_register_managed_functions), out _);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Registers managed functions that are called from native layer.
    /// </summary>
    public static unsafe void RegisterManagedFunctions()
    {
        csharp_interop_register_managed_functions(new ManagedFunctions
        {
            init = &InitCallback,
            close = &CloseCallback,
            logger_error = &LoggerErrorCallback,
            add_breadcrumb = &AddBreadcrumbCallback,
            set_tag = &SetTagCallback,
            remove_tag = &RemoveTagCallback,
            set_user = &SetUserCallback,
            remove_user = &RemoveUserCallback,
        });
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static void InitCallback()
    {
        try
        {
            SentrySdk.InitFromNative();
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to initialize Sentry .NET layer: {ex}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static void CloseCallback()
    {
        try
        {
            SentrySdk.CloseFromNative();
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to close Sentry .NET layer: {ex}");
        }
    }

    private static Action<string, string>? _loggerErrorHandler;

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void LoggerErrorCallback(char* code, int codeLen, char* file, int fileLen)
    {
        try
        {
            _loggerErrorHandler?.Invoke(
                    new string(file, 0, fileLen),
                    new string(code, 0, codeLen));
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward Godot logger error to Sentry .NET layer: {ex}");
        }
    }

    public static unsafe void SetLoggerErrorHandler(Action<string, string> handler)
    {
        _loggerErrorHandler = handler;
    }

    public static unsafe void ClearLoggerErrorHandler()
    {
        _loggerErrorHandler = null;
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void AddBreadcrumbCallback(
        char* message, int messageLen,
        char* category, int categoryLen,
        char* type, int typeLen,
        int level
    )
    {
        try
        {
            using var _ = new GodotScopeObserver.SyncGuard();
            Sentry.Godot.SentrySdk.AddBreadcrumb(
                message: new string(message, 0, messageLen),
                category: new string(category, 0, categoryLen),
                level: level switch
                {
                    0 => BreadcrumbLevel.Debug,
                    1 => BreadcrumbLevel.Info,
                    2 => BreadcrumbLevel.Warning,
                    3 => BreadcrumbLevel.Error,
                    4 => BreadcrumbLevel.Fatal,
                    _ => BreadcrumbLevel.Info,
                },
                type: new string(type, 0, typeLen));
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward breadcrumb to Sentry .NET layer: {ex}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void SetTagCallback(
        char* key, int keyLen,
        char* value, int valueLen)
    {
        try
        {
            using var _ = new GodotScopeObserver.SyncGuard();
            Sentry.Godot.SentrySdk.SetTag(new string(key, 0, keyLen), new string(value, 0, valueLen));
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward set_tag to Sentry .NET layer: {ex}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void RemoveTagCallback(
        char* key, int keyLen)
    {
        try
        {
            using var _ = new GodotScopeObserver.SyncGuard();
            Sentry.Godot.SentrySdk.UnsetTag(new string(key, 0, keyLen));
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward remove_tag to Sentry .NET layer: {ex}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void SetUserCallback(
        char* id, int idLen,
        char* username, int usernameLen,
        char* email, int emailLen,
        char* ip, int ipLen)
    {
        try
        {
            using var _ = new GodotScopeObserver.SyncGuard();
            Sentry.Godot.SentrySdk.ConfigureScope(scope =>
            {
                scope.User = new Sentry.SentryUser
                {
                    Id = idLen > 0 ? new string(id, 0, idLen) : null,
                    Username = usernameLen > 0 ? new string(username, 0, usernameLen) : null,
                    Email = emailLen > 0 ? new string(email, 0, emailLen) : null,
                    IpAddress = ipLen > 0 ? new string(ip, 0, ipLen) : null,
                };
            });
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward set_user to Sentry .NET layer: {ex}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
    private static unsafe void RemoveUserCallback()
    {
        try
        {
            using var _ = new GodotScopeObserver.SyncGuard();
            Sentry.Godot.SentrySdk.ConfigureScope(scope =>
            {
                scope.User = new Sentry.SentryUser();
            });
        }
        catch (Exception ex)
        {
            GodotLog.Error($"Failed to forward remove_user to Sentry .NET layer: {ex}");
        }
    }

    [LibraryImport(Lib)]
    private static unsafe partial NativeOptions csharp_interop_get_options();

    [LibraryImport(Lib)]
    private static unsafe partial NativeOptions csharp_interop_get_options_defaults();

    private static void ApplyNativeOptions(NativeOptions data, SentryGodotOptions opts)
    {
        opts.GodotLogger.Limits = new SentryLoggerLimits
        {
            EventsPerFrame = data.logger_limits.events_per_frame,
            RepeatedErrorWindow = TimeSpan.FromMilliseconds(data.logger_limits.repeated_error_window_ms),
            ThrottleEvents = data.logger_limits.throttle_events,
            ThrottleWindow = TimeSpan.FromMilliseconds(data.logger_limits.throttle_window_ms),
        };
        opts.Dsn = data.dsn.TakeString() ?? "";
        opts.Release = data.release.TakeString();
        opts.Distribution = data.dist.TakeString();
        opts.Environment = data.environment.TakeString();
        opts.Debug = data.debug != 0;
        opts.DiagnosticLevel = (SentryLevel)data.diagnostic_level;
        opts.SampleRate = (float)data.sample_rate;
        opts.MaxBreadcrumbs = data.max_breadcrumbs;
        opts.ShutdownTimeout = TimeSpan.FromMilliseconds(data.shutdown_timeout_ms);
        opts.SendDefaultPii = data.send_default_pii != 0;
        opts.EnableLogs = data.enable_logs != 0;
        opts.AttachLog = data.attach_log != 0;
        opts.AttachSceneTree = data.attach_scene_tree != 0;
        opts.AttachScreenshot = data.attach_screenshot != 0;
        opts.ScreenshotLevel = (SentryLevel)data.screenshot_level;
        opts.EnableAppHangTracking = data.enable_app_hang_tracking != 0;
        opts.AppHangTimeout = TimeSpan.FromMilliseconds(data.app_hang_timeout_ms);
        opts.GodotLogger.Enabled = data.logger_enabled != 0;
        opts.GodotLogger.IncludeSource = data.logger_include_source != 0;
        opts.GodotLogger.IncludeVariables = data.logger_include_variables != 0;
        opts.GodotLogger.EventMask = (GodotLoggerEventMask)data.logger_event_mask;
        opts.GodotLogger.BreadcrumbMask = (GodotLoggerEventMask)data.logger_breadcrumb_mask;
        opts.GodotLogger.LogMask = (GodotLoggerEventMask)data.logger_log_mask;
        opts.EnableMetrics = data.enable_metrics != 0;
        opts.Android.EnableAnrDetection = data.android_enable_anr_detection != 0;
        opts.Android.AnrTimeoutInterval = TimeSpan.FromMilliseconds(data.android_anr_timeout_interval_ms);
        opts.Android.AttachAnrThreadDump = data.android_attach_anr_thread_dump != 0;
    }

    public static void ApplyNativeOptions(SentryGodotOptions opts)
    {
        ApplyNativeOptions(csharp_interop_get_options(), opts);
    }

    public static void ApplyNativeOptionsDefaults(SentryGodotOptions opts)
    {
        ApplyNativeOptions(csharp_interop_get_options_defaults(), opts);
    }

    [LibraryImport(Lib)]
    private static partial NativeArray csharp_interop_get_default_attachments();

    /// <summary>
    /// Adds default attachments resolved by the native layer after init() to the managed options.
    /// </summary>
    public static unsafe void FetchDefaultAttachments(SentryGodotOptions opts)
    {
        using var result = csharp_interop_get_default_attachments();
        if (result.Ptr == IntPtr.Zero)
        {
            return;
        }
        var p = (AttachmentMeta*)result.Ptr;
        for (int i = 0; i < result.Count; ++i)
        {
            string? path = p[i].path.TakeString();
            string? filename = p[i].filename.TakeString();
            string? contentType = p[i].content_type.TakeString();
            string? attachmentType = p[i].attachment_type.TakeString();

            if (path is null)
            {
                // Only file-based default attachments are supported.
                continue;
            }
            opts.AddDefaultAttachment(new SentryAttachment(
                type: attachmentType switch
                {
                    "event.view_hierarchy" => AttachmentType.ViewHierarchy,
                    _ => AttachmentType.Default,
                },
                content: new FileAttachmentContent(path),
                fileName: String.IsNullOrEmpty(filename)
                        ? System.IO.Path.GetFileName(path)
                        : filename,
                contentType: contentType
            ));
        }
    }

    [LibraryImport(Lib)]
    private static unsafe partial GodotStringHandle csharp_interop_detect_environment();

    public static unsafe string? DetectEnvironment()
    {
        return csharp_interop_detect_environment().TakeString();
    }

    [LibraryImport(Lib)]
    private static unsafe partial NativeTraceContext csharp_interop_get_trace_context();

    public static unsafe (string? traceId, string? parentSpanId) GetTraceContext()
    {
        var nativeContext = csharp_interop_get_trace_context();
        return (nativeContext.trace_id.TakeString(), nativeContext.parent_span_id.TakeString());
    }

    [LibraryImport(Lib)]
    private static unsafe partial GodotStringHandle csharp_interop_get_app_name();

    [LibraryImport(Lib)]
    private static unsafe partial GodotStringHandle csharp_interop_get_app_version();

    public static string GetAppName()
    {
        return csharp_interop_get_app_name().TakeString() ?? "";
    }

    public static string GetAppVersion()
    {
        return csharp_interop_get_app_version().TakeString() ?? "";
    }

    [LibraryImport(Lib)]
    private static unsafe partial byte csharp_interop_sdk_is_enabled();

    public static bool IsEnabled()
    {
        return csharp_interop_sdk_is_enabled() != 0;
    }

    [LibraryImport(Lib)]
    private static unsafe partial byte csharp_interop_is_debugger_active();

    public static bool IsDebuggerActive()
    {
        return csharp_interop_is_debugger_active() != 0;
    }

    [LibraryImport(Lib)]
    private static unsafe partial byte csharp_interop_is_android();

    /// <remarks>
    /// OperatingSystem.IsAndroid() returns false on Godot Android.
    /// </remarks>
    public static bool IsAndroid()
    {
        return csharp_interop_is_android() != 0;
    }

    // Must match layout of AssemblyHandle in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    internal struct AssemblyHandle
    {
        public IntPtr Handle;
        public long Length;
    }

    [LibraryImport(Lib)]
    private static unsafe partial AssemblyHandle csharp_interop_open_managed_assembly(char* name, int nameLen);

    [LibraryImport(Lib)]
    private static unsafe partial long csharp_interop_read_managed_assembly(IntPtr handle, long offset, long count, byte* dst);

    [LibraryImport(Lib)]
    private static partial void csharp_interop_close_managed_assembly(IntPtr handle);

    /// <summary>
    /// Opens managed assembly by name from Godot's virtual filesystem as a seekable handle.
    /// Returns null handle with zero length if the assembly was not found.
    /// </summary>
    public static unsafe AssemblyHandle OpenManagedAssembly(string assemblyName)
    {
        fixed (char* namePtr = assemblyName)
        {
            return csharp_interop_open_managed_assembly(namePtr, assemblyName.Length);
        }
    }

    /// <summary>
    /// Reads bytes from an open assembly handle at specified offset into destination.
    /// Returns the number of bytes read, which may be less than requested at EOF.
    /// </summary>
    public static unsafe int ReadManagedAssembly(IntPtr handle, long offset, Span<byte> destination)
    {
        if (handle == IntPtr.Zero || destination.IsEmpty)
        {
            return 0;
        }
        fixed (byte* dstPtr = destination)
        {
            return (int)csharp_interop_read_managed_assembly(handle, offset, destination.Length, dstPtr);
        }
    }

    /// <summary>
    /// Closes a managed assembly handle opened by <see cref="OpenManagedAssembly"/>.
    /// </summary>
    public static void CloseManagedAssembly(IntPtr handle)
    {
        if (handle != IntPtr.Zero)
        {
            csharp_interop_close_managed_assembly(handle);
        }
    }

    // Must match layout of CocoaDebugImageEntry in csharp_interop.cpp.
    [StructLayout(LayoutKind.Sequential)]
    private struct CocoaDebugImageEntry
    {
        public GodotStringHandle code_file;
        public GodotStringHandle debug_id;
        public long image_address;
        public long image_size;
    }

    [LibraryImport(Lib)]
    private static unsafe partial int csharp_interop_get_cocoa_debug_images(
            long* addresses, int addressesCount,
            CocoaDebugImageEntry* entries, int entriesCapacity);

    /// <summary>
    /// Resolves Mach-O debug images for the given image base addresses from the
    /// Cocoa SDK's in-process cache.
    /// Used to populate debug_meta.images for NativeAOT frames on iOS, which
    /// sentry-dotnet would otherwise leave empty for our TFM-neutral build.
    /// </summary>
    public static unsafe List<DebugImage> GetCocoaDebugImages(IReadOnlyCollection<long> imageAddresses)
    {
        var result = new List<DebugImage>();
        int count = imageAddresses.Count;
        if (count == 0)
        {
            return result;
        }

        Span<long> addresses = count <= 64
                ? stackalloc long[count]
                : new long[count];
        int i = 0;
        foreach (var addr in imageAddresses)
        {
            addresses[i++] = addr;
        }

        Span<CocoaDebugImageEntry> entries = count <= 16
                ? stackalloc CocoaDebugImageEntry[count]
                : new CocoaDebugImageEntry[count];

        int written;
        fixed (long* addressesPtr = addresses)
        fixed (CocoaDebugImageEntry* entriesPtr = entries)
        {
            written = csharp_interop_get_cocoa_debug_images(addressesPtr, count, entriesPtr, count);
        }

        for (int j = 0; j < written; ++j)
        {
            ref CocoaDebugImageEntry entry = ref entries[j];
            string? codeFile = entry.code_file.TakeString();
            string? debugId = entry.debug_id.TakeString();
            result.Add(new DebugImage
            {
                Type = "macho",
                ImageAddress = entry.image_address,
                ImageSize = entry.image_size != 0 ? entry.image_size : null,
                DebugId = string.IsNullOrEmpty(debugId) ? null : debugId,
                CodeFile = string.IsNullOrEmpty(codeFile) ? null : codeFile,
            });
        }
        return result;
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_init(ManagedOptions opts);

    public static unsafe void InitNativeSdk(SentryGodotOptions opts)
    {
        var dsn = opts.Dsn ?? "";
        var release = opts.Release ?? "";
        var dist = opts.Distribution ?? "";
        var env = opts.Environment ?? "";

        fixed (char* dsnPtr = dsn)
        fixed (char* relPtr = release)
        fixed (char* distPtr = dist)
        fixed (char* envPtr = env)
        {
            var managed = new ManagedOptions
            {
                logger_limits = new LoggerLimitsData
                {
                    events_per_frame = opts.GodotLogger.Limits.EventsPerFrame,
                    repeated_error_window_ms = (int)opts.GodotLogger.Limits.RepeatedErrorWindow.TotalMilliseconds,
                    throttle_events = opts.GodotLogger.Limits.ThrottleEvents,
                    throttle_window_ms = (int)opts.GodotLogger.Limits.ThrottleWindow.TotalMilliseconds,
                },
                dsn = dsnPtr,
                dsn_len = dsn.Length,
                release = relPtr,
                release_len = release.Length,
                dist = distPtr,
                dist_len = dist.Length,
                environment = envPtr,
                environment_len = env.Length,
                debug = (byte)(opts.Debug ? 1 : 0),
                diagnostic_level = (int)opts.DiagnosticLevel,
                sample_rate = opts.SampleRate ?? 1.0,
                max_breadcrumbs = opts.MaxBreadcrumbs,
                shutdown_timeout_ms = opts.ShutdownTimeout.TotalMilliseconds,
                send_default_pii = (byte)(opts.SendDefaultPii ? 1 : 0),
                enable_logs = (byte)(opts.EnableLogs ? 1 : 0),
                attach_log = (byte)(opts.AttachLog ? 1 : 0),
                attach_scene_tree = (byte)(opts.AttachSceneTree ? 1 : 0),
                attach_screenshot = (byte)(opts.AttachScreenshot ? 1 : 0),
                screenshot_level = (int)opts.ScreenshotLevel,
                enable_app_hang_tracking = (byte)(opts.EnableAppHangTracking ? 1 : 0),
                app_hang_timeout_ms = (int)opts.AppHangTimeout.TotalMilliseconds,
                logger_enabled = (byte)(opts.GodotLogger.Enabled ? 1 : 0),
                logger_include_source = (byte)(opts.GodotLogger.IncludeSource ? 1 : 0),
                logger_include_variables = (byte)(opts.GodotLogger.IncludeVariables ? 1 : 0),
                logger_event_mask = (int)opts.GodotLogger.EventMask,
                logger_breadcrumb_mask = (int)opts.GodotLogger.BreadcrumbMask,
                logger_log_mask = (int)opts.GodotLogger.LogMask,
                enable_metrics = (byte)(opts.EnableMetrics ? 1 : 0),
                android_enable_anr_detection = (byte)(opts.Android.EnableAnrDetection ? 1 : 0),
                android_anr_timeout_interval_ms = (int)opts.Android.AnrTimeoutInterval.TotalMilliseconds,
                android_attach_anr_thread_dump = (byte)(opts.Android.AttachAnrThreadDump ? 1 : 0),
            };
            csharp_interop_sdk_init(managed);
        }
    }

    [LibraryImport(Lib)]
    private static partial void csharp_interop_sdk_close();

    public static void CloseNativeSdk()
    {
        csharp_interop_sdk_close();
    }

    [LibraryImport(Lib)]
    private static partial void csharp_interop_process_default_attachments(int level);

    public static void ProcessDefaultAttachments(SentryLevel level)
    {
        csharp_interop_process_default_attachments((int)level);
    }

    /// <remarks>
    /// Returns compile-time literal - don't free!
    /// </remarks>
    [LibraryImport(Lib)]
    private static partial IntPtr csharp_interop_get_sdk_version();

    public static string GetSdkVersion()
    {
        return Marshal.PtrToStringUTF8(csharp_interop_get_sdk_version()) ?? "0.0.0";
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_log(int level, char* msg, int len);

    public static unsafe void Log(SentryLevel level, string message)
    {
        fixed (char* ptr = message)
        {
            csharp_interop_log((int)level, ptr, message.Length);
        }
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_add_breadcrumb(
            char* message, int messageLen,
            char* category, int categoryLen,
            char* type, int typeLen,
            int level,
            ManagedStringMap data);

    public static unsafe void AddBreadcrumb(Breadcrumb breadcrumb)
    {
        var msg = breadcrumb.Message ?? "";
        var cat = breadcrumb.Category ?? "";
        var type = breadcrumb.Type ?? "";
        int level = breadcrumb.Level switch
        {
            BreadcrumbLevel.Debug => 0,
            BreadcrumbLevel.Info => 1,
            BreadcrumbLevel.Warning => 2,
            BreadcrumbLevel.Error => 3,
            BreadcrumbLevel.Fatal => 4,
            _ => 1,
        };

        MarshallStringMap(breadcrumb.Data, map =>
        {
            fixed (char* msgPtr = msg)
            fixed (char* catPtr = cat)
            fixed (char* typePtr = type)
            {
                csharp_interop_sdk_add_breadcrumb(
                        msgPtr, msg.Length,
                        catPtr, cat.Length,
                        typePtr, type.Length,
                        level, map);
            }
        });
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_set_tag(
            char* key, int keyLen, char* value, int valueLen);

    public static unsafe void SetTag(string key, string value)
    {
        fixed (char* keyPtr = key)
        fixed (char* valPtr = value)
        {
            csharp_interop_sdk_set_tag(keyPtr, key.Length, valPtr, value.Length);
        }
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_remove_tag(
            char* key, int keyLen);

    public static unsafe void RemoveTag(string key)
    {
        fixed (char* keyPtr = key)
        {
            csharp_interop_sdk_remove_tag(keyPtr, key.Length);
        }
    }

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_remove_user();

    [LibraryImport(Lib)]
    private static unsafe partial void csharp_interop_sdk_set_user(
            char* id, int idLen, char* username, int usernameLen, char* email, int emailLen, char* ipAddress, int ipAddressLen);

    public static unsafe void SetUser(SentryUser? user)
    {
        if (user is null)
        {
            csharp_interop_sdk_remove_user();
        }
        else
        {
            var username = user.Username ?? "";
            var email = user.Email ?? "";
            var id = user.Id ?? "";
            var ipAddress = user.IpAddress ?? "";

            fixed (char* usernamePtr = username)
            fixed (char* emailPtr = email)
            fixed (char* idPtr = id)
            fixed (char* ipAddressPtr = ipAddress)
            {
                csharp_interop_sdk_set_user(
                        idPtr, id.Length,
                        usernamePtr, username.Length,
                        emailPtr, email.Length,
                        ipAddressPtr, ipAddress.Length);
            }
        }
    }
}
