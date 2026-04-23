using System.Collections.Immutable;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Diagnostics;

namespace Sentry.Godot.Analyzers;

[DiagnosticAnalyzer(LanguageNames.CSharp)]
public sealed class PreferGodotSentrySdkAnalyzer : DiagnosticAnalyzer
{
    public const string DiagnosticId = "SENTRYGD1001";

    private static readonly DiagnosticDescriptor Rule = new(
        id: DiagnosticId,
        title: "Use Sentry.Godot.SentrySdk instead of Sentry.SentrySdk",
        messageFormat: "Sentry.SentrySdk skips Godot-specific initialization and options. Use Sentry.Godot.SentrySdk instead.",
        category: "Usage",
        defaultSeverity: DiagnosticSeverity.Warning,
        isEnabledByDefault: true,
        description: "The upstream Sentry.SentrySdk entry points skip native-layer initialization, option mirroring, and scope synchronization. Use Sentry.Godot.SentrySdk for proper Godot integration.",
        helpLinkUri: "https://docs.sentry.io/platforms/godot/");

    public override ImmutableArray<DiagnosticDescriptor> SupportedDiagnostics { get; } =
        ImmutableArray.Create(Rule);

    public override void Initialize(AnalysisContext context)
    {
        context.ConfigureGeneratedCodeAnalysis(GeneratedCodeAnalysisFlags.None);
        context.EnableConcurrentExecution();
        context.RegisterSyntaxNodeAction(AnalyzeMemberAccess, SyntaxKind.SimpleMemberAccessExpression);
        context.RegisterSyntaxNodeAction(AnalyzeIdentifier, SyntaxKind.IdentifierName);
    }

    private static void AnalyzeMemberAccess(SyntaxNodeAnalysisContext context)
    {
        var memberAccess = (MemberAccessExpressionSyntax)context.Node;
        var symbol = context.SemanticModel.GetSymbolInfo(memberAccess).Symbol;
        if (symbol is null)
        {
            return;
        }

        // X.SentrySdk (type reference): skip if wrapped by an outer member access
        // — the outer access carries the real member call and will be flagged.
        if (symbol is INamedTypeSymbol typeSymbol && IsUpstreamSentrySdk(typeSymbol))
        {
            if (memberAccess.Parent is MemberAccessExpressionSyntax parent && parent.Expression == memberAccess)
            {
                return;
            }
            context.ReportDiagnostic(Diagnostic.Create(Rule, memberAccess.GetLocation()));
            return;
        }

        // X.Y where Y is a static member of Sentry.SentrySdk.
        if (symbol.ContainingType is { } containingType && IsUpstreamSentrySdk(containingType))
        {
            context.ReportDiagnostic(Diagnostic.Create(Rule, memberAccess.GetLocation()));
        }
    }

    private static void AnalyzeIdentifier(SyntaxNodeAnalysisContext context)
    {
        var identifier = (IdentifierNameSyntax)context.Node;
        if (identifier.Identifier.Text != "SentrySdk")
        {
            return;
        }

        // Member-access cases are covered by AnalyzeMemberAccess.
        if (identifier.Parent is MemberAccessExpressionSyntax)
        {
            return;
        }

        if (context.SemanticModel.GetSymbolInfo(identifier).Symbol is INamedTypeSymbol typeSymbol
            && IsUpstreamSentrySdk(typeSymbol))
        {
            context.ReportDiagnostic(Diagnostic.Create(Rule, identifier.GetLocation()));
        }
    }

    private static bool IsUpstreamSentrySdk(INamedTypeSymbol type)
        => type is
        {
            Name: "SentrySdk",
            ContainingType: null,
            ContainingNamespace: { Name: "Sentry", ContainingNamespace.IsGlobalNamespace: true }
        };
}
