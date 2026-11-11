using System.Collections.Generic;
using Microsoft.CodeAnalysis;

namespace Sentry.Godot.Shared;

/// <summary>
/// Rules for mapping upstream SDK members to handwritten or generated Godot APIs.
/// </summary>
/// <remarks>
/// Used by the source generator to skip handwritten and excluded members, and by the
/// analyzer's code fix to avoid suggesting excluded APIs as replacements.
/// Compiled into both projects so they share the same rules.
/// </remarks>
internal static class SentrySdkMemberRules
{
    /// <summary>
    /// Names of public SDK members that are implemented manually in the partial class.
    /// Use this list only when all overloads for a method name, or any property with the
    /// same name, should be suppressed from generated forwarding wrappers.
    /// </summary>
    private static readonly HashSet<string> HandWrittenMembers =
    [
        "Init",
        "Close",
        "StartSession",
        "EndSession"
    ];

    /// <summary>
    /// Method overloads that are implemented manually in the partial class.
    /// </summary>
    /// <remarks>
    /// Entries use <see cref="SignatureFormat"/> so only the listed overload is skipped;
    /// new upstream overloads are still generated and reviewed via snapshot diffs.
    /// See <c>SentrySdk.cs</c> for the implementations.
    /// </remarks>
    private static readonly HashSet<string> HandWrittenSignatures =
    [
        // Hand-written wrappers guard local scope changes from leaking into the native layer current scope.
        "CaptureEvent(SentryEvent, Action<Scope>)",
        "CaptureEvent(SentryEvent, SentryHint?, Action<Scope>)",
        "CaptureException(Exception, Action<Scope>)",
        "CaptureException(Exception, bool, bool)",
        "CaptureException(Exception, bool, bool, Action<Scope>)",
        "CaptureMessage(string, Action<Scope>, SentryLevel)",
        "CaptureFeedback(SentryFeedback, Action<Scope>, SentryHint?)",
        "CaptureFeedback(SentryFeedback, out CaptureFeedbackResult, Action<Scope>, SentryHint?)",
    ];

    /// <summary>
    /// Upstream members omitted from generation and code-fix suggestions.
    /// </summary>
    /// <remarks>
    /// Use this list to exclude every overload of a member; use
    /// <see cref="ExcludedSignatures"/> to exclude a particular overload.
    /// </remarks>
    private static readonly HashSet<string> ExcludedMembers =
    [
        // These managed-session APIs do not control the Godot SDK sessions.
        "PauseSession",
        "ResumeSession",
        "CaptureSession",
        "IsSessionActive"
    ];

    /// <summary>
    /// Specific upstream overloads omitted from generation and code-fix suggestions.
    /// </summary>
    /// <remarks>
    /// Use this list when only a particular overload is unsupported; use
    /// <see cref="ExcludedMembers"/> to exclude every overload of a member.
    /// Entries must match <see cref="SignatureFormat"/>.
    /// </remarks>
    private static readonly HashSet<string> ExcludedSignatures =
    [
        // The native layer owns sessions and does not support explicit statuses.
        "EndSession(SessionEndStatus)"
    ];

    private static readonly SymbolDisplayFormat SignatureFormat = new(
        typeQualificationStyle: SymbolDisplayTypeQualificationStyle.NameAndContainingTypes,
        genericsOptions: SymbolDisplayGenericsOptions.IncludeTypeParameters,
        memberOptions: SymbolDisplayMemberOptions.IncludeParameters,
        parameterOptions:
            SymbolDisplayParameterOptions.IncludeType |
            SymbolDisplayParameterOptions.IncludeParamsRefOut,
        miscellaneousOptions:
            SymbolDisplayMiscellaneousOptions.UseSpecialTypes |
            SymbolDisplayMiscellaneousOptions.IncludeNullableReferenceTypeModifier);

    internal static bool IsHandWritten(ISymbol member) =>
        HandWrittenMembers.Contains(member.Name) ||
        (member is IMethodSymbol method &&
         HandWrittenSignatures.Contains(method.ToDisplayString(SignatureFormat)));

    internal static bool IsExcluded(ISymbol member) =>
        ExcludedMembers.Contains(member.Name) ||
        (member is IMethodSymbol method &&
         ExcludedSignatures.Contains(method.ToDisplayString(SignatureFormat)));
}
