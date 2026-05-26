using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection.PortableExecutable;
using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

/// <summary>
/// Provides assembly bytes to the Sentry .NET SDK for managed stack-trace symbolication.
/// </summary>
/// <remarks>
/// On Android, .NET assemblies live inside the application archive (possibly nested in a .pck) and have no on-disk path
/// the SDK can open. Without their bytes the SDK cannot build debug images, so the server reports every managed frame
/// as unknown_image. This reader is designed to fetch bytes via the native bridge, routed through Godot's FileAccess
/// so the engine's virtual FS handles packing and decompression.
///
/// The SDK only reads two small segments per assembly (PE headers at the start and the debug directory near the end), so
/// each assembly is exposed as a seekable Stream rather than whole. Touched segments are cached per assembly, and repeat
/// captures are served from cache and do no further I/O.
/// </remarks>
internal sealed class GodotAssemblyReader
{
    // Per-assembly read cache, keyed by the assembly name. A null value records a failed lookup (no such assembly found).
    private readonly Dictionary<string, AssemblyReadState?> _cache = new(StringComparer.OrdinalIgnoreCase);
    private readonly object _lock = new();

    /// <summary>
    /// Returns a PEReader over the named assembly, or null if it cannot be resolved.
    /// </summary>
    public PEReader? TryReadAssembly(string assemblyName)
    {
        AssemblyReadState state;
        IntPtr openHandle = IntPtr.Zero;
        lock (_lock)
        {
            if (_cache.TryGetValue(assemblyName, out var cached))
            {
                if (cached is null)
                {
                    // Known miss: Failed to find this assembly previously.
                    return null;
                }
                state = cached;
            }
            else
            {
                // First request: Open assembly and learn its length, and keep the handle for later reads.
                var opened = NativeBridge.OpenManagedAssembly(assemblyName);
                openHandle = opened.Handle;
                if (openHandle == IntPtr.Zero)
                {
                    _cache[assemblyName] = null; // record miss
                    return null;
                }
                state = new AssemblyReadState(opened.Length);
                _cache[assemblyName] = state;
            }
        }

        var stream = new AssemblyReadStream(assemblyName, state, openHandle);
        try
        {
            return new PEReader(stream);
        }
        catch
        {
            stream.Dispose();
            return null;
        }
    }

    private sealed class AssemblyReadState(long length)
    {
        public readonly long Length = length;

        // Byte ranges read from the file so far.
        public readonly List<Segment> Segments = [];
    }

    private readonly struct Segment(long start, byte[] data)
    {
        public readonly long Start = start;
        public readonly byte[] Data = data;

        public long End => Start + Data.Length;
    }

    /// <summary>
    /// Seekable stream over a managed assembly in Godot's virtual FS.
    /// </summary>
    /// <remarks>
    /// Reads hit cached segments when possible, and misses go to the native file handle and the segment is stored so
    /// later captures hit our cache.
    /// The handle is opened lazily and closed on dispose.
    /// </remarks>
    private sealed class AssemblyReadStream(string assemblyName, AssemblyReadState state, IntPtr handle) : Stream
    {
        // The SDK parses the PE headers a few bytes at a time, then reads the debug directory.
        // Reading larger blocks on a miss collapses that into a handful of native reads instead of one per field.
        // The value was derived empirically, resulting in 2 native reads per assembly.
        private const long BlockSize = 4096; // matches Godot's compressed FS block size

        private readonly string _assemblyName = assemblyName;
        private readonly AssemblyReadState _state = state;

        // Contains IntPtr.Zero until a read misses the cached segments and needs the file.
        private IntPtr _handle = handle;

        private long _position;
        public override long Position
        {
            get => _position;
            set => _position = value;
        }


        public override bool CanRead => true;
        public override bool CanSeek => true;
        public override bool CanWrite => false;
        public override long Length => _state.Length;

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (count <= 0 || _position >= _state.Length)
            {
                return 0;
            }

            count = (int)Math.Min(count, _state.Length - _position);

            // Serve from a cached segment if one fully covers the request.
            lock (_state)
            {
                foreach (var segment in _state.Segments)
                {
                    if (segment.Start <= _position && segment.End >= _position + count)
                    {
                        var offsetInSegment = (int)(_position - segment.Start);
                        segment.Data.AsSpan(offsetInSegment, count)
                            .CopyTo(buffer.AsSpan(offset));
                        _position += count;
                        return count;
                    }
                }
            }

            // Miss: Read the block-aligned byte segment covering the request and cache it for later reads.

            long segmentStart = _position & ~(BlockSize - 1); // round down to a multiple of BlockSize
            long requestedEnd = _position + count;
            long blockAlignedEnd = (requestedEnd + BlockSize - 1) & ~(BlockSize - 1);
            long segmentEnd = Math.Min(blockAlignedEnd, _state.Length);

            var segmentData = new byte[segmentEnd - segmentStart];
            int read = ReadFromFile(segmentStart, segmentData);
            if (read <= 0)
            {
                return 0;
            }
            if (read < segmentData.Length)
            {
                Array.Resize(ref segmentData, read);
            }

            lock (_state)
            {
                _state.Segments.Add(new Segment(segmentStart, segmentData));
            }

            // ReadFromFile() can return less than requested at EOF.
            var segmentActualEnd = segmentStart + segmentData.Length;
            if (segmentActualEnd <= _position)
            {
                // Reading truncated at EOF before reaching our position - nothing to return.
                return 0;
            }

            int bytesToCopy = (int)Math.Min(count, segmentActualEnd - _position);
            int offsetInSegmentData = (int)(_position - segmentStart);
            segmentData.AsSpan(offsetInSegmentData, bytesToCopy)
                .CopyTo(buffer.AsSpan(offset));
            _position += bytesToCopy;
            return bytesToCopy;
        }

        private int ReadFromFile(long offset, Span<byte> destination)
        {
            if (_handle == IntPtr.Zero)
            {
                _handle = NativeBridge.OpenManagedAssembly(_assemblyName).Handle;
                if (_handle == IntPtr.Zero)
                {
                    return 0;
                }
            }
            return NativeBridge.ReadManagedAssembly(_handle, offset, destination);
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            _position = origin switch
            {
                SeekOrigin.Begin => offset,
                SeekOrigin.Current => _position + offset,
                SeekOrigin.End => _state.Length + offset,
                _ => _position,
            };
            return _position;
        }

        public override void Flush() { }

        public override void SetLength(long value) => throw new NotSupportedException();

        public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();

        protected override void Dispose(bool disposing)
        {
            if (_handle != IntPtr.Zero)
            {
                NativeBridge.CloseManagedAssembly(_handle);
                _handle = IntPtr.Zero;
            }
            base.Dispose(disposing);
        }
    }
}
