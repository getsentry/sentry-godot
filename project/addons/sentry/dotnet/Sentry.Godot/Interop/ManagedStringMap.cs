using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Sentry.Godot.Interop;

/// <summary>
/// Passes string map to a native function call (aka P/Invoke).
/// Buffer layout: key1+val1+key2+val2... concatenated as UTF-16.
/// Lengths array: key1_len, val1_len, key2_len, val2_len, ... (2*pair_count entries).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedStringMap {
	public char *Buffer;
	public int *Lengths;
	public int PairCount;

	public delegate void NativeAction(ManagedStringMap map);

	/// <summary>
	/// Marshals a dictionary into a stack-allocated interleaved buffer and passes the
	/// resulting struct to native function call (aka P/Invoke).
	/// The map is only valid for the duration of the call (owned by managed code).
	/// </summary>
	public static void Marshall(IReadOnlyDictionary<string, string>? dict, NativeAction nativeFunc) {
		if (dict == null || dict.Count == 0) {
			nativeFunc(default);
			return;
		}

		int pairCount = dict.Count;

		Span<int> lengthSpan = pairCount * 2 <= 256
				? stackalloc int[pairCount * 2]
				: new int[pairCount * 2];
		int totalChars = 0;
		int i = 0;
		foreach (var kv in dict) {
			lengthSpan[i++] = kv.Key.Length;
			lengthSpan[i++] = kv.Value.Length;
			totalChars += kv.Key.Length + kv.Value.Length;
		}

		Span<char> buffer = totalChars <= 512
				? stackalloc char[totalChars]
				: new char[totalChars];

		int pos = 0;
		foreach (var kv in dict) {
			kv.Key.AsSpan().CopyTo(buffer.Slice(pos));
			pos += kv.Key.Length;
			kv.Value.AsSpan().CopyTo(buffer.Slice(pos));
			pos += kv.Value.Length;
		}

		fixed(char *bufPtr = buffer) fixed(int *lenPtr = lengthSpan) {
			var map = new ManagedStringMap {
				Buffer = bufPtr,
				Lengths = lenPtr,
				PairCount = pairCount
			};
			nativeFunc(map);
		}
	}
}
