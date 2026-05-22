using System.Collections.Immutable;
using System.Composition;
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

        foreach (var diagnostic in context.Diagnostics)
        {
            // The analyzer reports on a member-access (e.g. 'Sentry.SentrySdk.Init').
            // Replace just its qualifier so the member name is preserved. Bare
            // identifier targets (e.g. under 'using static Sentry.SentrySdk') have
            // no qualifier to swap — we skip offering a fix in that case.
            if (root.FindNode(diagnostic.Location.SourceSpan) is not MemberAccessExpressionSyntax memberAccess)
            {
                continue;
            }

            context.RegisterCodeFix(
                CodeAction.Create(
                    title: "Use Sentry.Godot.SentrySdk",
                    createChangedDocument: _ => Task.FromResult(
                        ReplaceWithGodotSdk(context.Document, root, memberAccess.Expression)),
                    equivalenceKey: nameof(PreferGodotSentrySdkCodeFix)),
                diagnostic);
        }
    }

    private static Document ReplaceWithGodotSdk(Document document, SyntaxNode root, ExpressionSyntax target)
    {
        // Preserve a leftmost alias qualifier (e.g. 'global::') so
        // 'global::Sentry.SentrySdk.X' rewrites to 'global::Sentry.Godot.SentrySdk.X'.
        var alias = GetLeftmostAlias(target);
        var qualifiedName = alias is null
            ? "Sentry.Godot.SentrySdk"
            : $"{alias}::Sentry.Godot.SentrySdk";

        var replacement = SyntaxFactory.ParseExpression(qualifiedName).WithTriviaFrom(target);
        return document.WithSyntaxRoot(root.ReplaceNode(target, replacement));
    }

    private static string? GetLeftmostAlias(SyntaxNode target)
    {
        var current = target;
        while (current is not null)
        {
            switch (current)
            {
                case AliasQualifiedNameSyntax alias:
                    return alias.Alias.Identifier.Text;
                case MemberAccessExpressionSyntax memberAccess:
                    current = memberAccess.Expression;
                    break;
                case QualifiedNameSyntax qualified:
                    current = qualified.Left;
                    break;
                default:
                    return null;
            }
        }
        return null;
    }
}
