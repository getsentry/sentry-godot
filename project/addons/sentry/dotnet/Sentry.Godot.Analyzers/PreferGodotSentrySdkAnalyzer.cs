using System.Collections.Immutable;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Diagnostics;
using Microsoft.CodeAnalysis.Operations;

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
        context.RegisterCompilationStartAction(OnCompilationStart);
    }

    private static void OnCompilationStart(CompilationStartAnalysisContext context)
    {
        // Resolve once per compilation; skip analysis entirely when the Sentry
        // package isn't referenced.
        var sentrySdkType = context.Compilation.GetTypeByMetadataName("Sentry.SentrySdk");
        if (sentrySdkType is null)
        {
            return;
        }

        context.RegisterOperationAction(
            ctx => AnalyzeOperation(ctx, sentrySdkType),
            OperationKind.Invocation,
            OperationKind.PropertyReference,
            OperationKind.FieldReference,
            OperationKind.EventReference,
            OperationKind.MethodReference);
    }

    private static void AnalyzeOperation(OperationAnalysisContext context, INamedTypeSymbol sentrySdkType)
    {
        var containingType = context.Operation switch
        {
            IInvocationOperation invocation => invocation.TargetMethod.ContainingType,
            IMemberReferenceOperation memberRef => memberRef.Member.ContainingType,
            _ => null,
        };

        if (!SymbolEqualityComparer.Default.Equals(containingType, sentrySdkType))
        {
            return;
        }

        // Report on the member-access (e.g. 'Sentry.SentrySdk.Init') rather than the
        // full invocation syntax, which also contains the argument list.
        var syntax = context.Operation.Syntax is InvocationExpressionSyntax invocationSyntax
            ? invocationSyntax.Expression
            : context.Operation.Syntax;
        context.ReportDiagnostic(Diagnostic.Create(Rule, syntax.GetLocation()));
    }
}
