using System.Collections.Immutable;
using System.Composition;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CodeActions;
using Microsoft.CodeAnalysis.CodeFixes;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Sentry.Godot.Analyzers;

[ExportCodeFixProvider(LanguageNames.CSharp, Name = nameof(PreferGodotSentrySdkCodeFix))]
[Shared]
public sealed class PreferGodotSentrySdkCodeFix : CodeFixProvider
{
    public override ImmutableArray<string> FixableDiagnosticIds { get; } =
        ImmutableArray.Create(PreferGodotSentrySdkAnalyzer.DiagnosticId);

    public override FixAllProvider GetFixAllProvider() => WellKnownFixAllProviders.BatchFixer;

    public override async Task RegisterCodeFixesAsync(CodeFixContext context)
    {
        var root = await context.Document.GetSyntaxRootAsync(context.CancellationToken).ConfigureAwait(false);
        if (root is null)
        {
            return;
        }
        var semanticModel = await context.Document.GetSemanticModelAsync(context.CancellationToken).ConfigureAwait(false);
        if (semanticModel is null)
        {
            return;
        }

        foreach (var diagnostic in context.Diagnostics)
        {
            var diagnosticNode = root.FindNode(diagnostic.Location.SourceSpan);
            var target = FindReplacementTarget(diagnosticNode, semanticModel, context.CancellationToken);
            if (target is null)
            {
                continue;
            }

            context.RegisterCodeFix(
                CodeAction.Create(
                    title: "Use Sentry.Godot.SentrySdk",
                    createChangedDocument: _ => Task.FromResult(ReplaceWithGodotSdk(context.Document, root, target)),
                    equivalenceKey: nameof(PreferGodotSentrySdkCodeFix)),
                diagnostic);
        }
    }

    /// <summary>
    /// Returns the syntax node that denotes the 'Sentry.SentrySdk' type reference,
    /// so the code fix can swap just that part for 'Sentry.Godot.SentrySdk' without
    /// disturbing the surrounding expression.
    /// </summary>
    private static SyntaxNode? FindReplacementTarget(SyntaxNode node, SemanticModel model, CancellationToken ct)
    {
        switch (node)
        {
            case MemberAccessExpressionSyntax memberAccess:
                // nameof(Sentry.SentrySdk): the whole access IS the type reference.
                if (model.GetSymbolInfo(memberAccess, ct).Symbol is INamedTypeSymbol)
                {
                    return memberAccess;
                }
                // Else the access is a member call; replace only its qualifier.
                return memberAccess.Expression;

            case IdentifierNameSyntax identifier:
                // typeof(Sentry.SentrySdk) etc.: identifier sits inside a QualifiedName;
                // replace the whole QualifiedName so we don't end up with "Sentry.Sentry.Godot.SentrySdk".
                if (identifier.Parent is QualifiedNameSyntax qualified && qualified.Right == identifier)
                {
                    return qualified;
                }
                return identifier;

            default:
                return null;
        }
    }

    private static Document ReplaceWithGodotSdk(Document document, SyntaxNode root, SyntaxNode target)
    {
        SyntaxNode replacement = target is NameSyntax
            ? SyntaxFactory.ParseName("Sentry.Godot.SentrySdk")
            : SyntaxFactory.ParseExpression("Sentry.Godot.SentrySdk");
        replacement = replacement.WithTriviaFrom(target);
        var newRoot = root.ReplaceNode(target, replacement);
        return document.WithSyntaxRoot(newRoot);
    }
}
