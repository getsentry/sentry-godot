using System.Threading.Tasks;
using Microsoft.CodeAnalysis.CSharp.Testing;
using Microsoft.CodeAnalysis.Testing;
using Sentry.Godot.Analyzers;
using Xunit;

namespace Sentry.Godot.Analyzers.Tests;

public class PreferGodotSentrySdkCodeFixTests
{
    private static Task RunAsync(string source, string fixedSource) =>
        new CSharpCodeFixTest<PreferGodotSentrySdkAnalyzer, PreferGodotSentrySdkCodeFix, DefaultVerifier>
        {
            ReferenceAssemblies = ReferenceAssemblies.Net.Net80,
            TestState =
            {
                Sources = { source, TestSources.GodotSdkStub },
                AdditionalReferences = { typeof(global::Sentry.SentrySdk).Assembly },
            },
            FixedState =
            {
                Sources = { fixedSource, TestSources.GodotSdkStub },
            },
        }.RunAsync();

    [Fact]
    public Task FullyQualifiedMemberCall_Rewritten() => RunAsync(
        source: """
            class Foo
            {
                void Bar() => {|SENTRYGD1001:Sentry.SentrySdk.CaptureMessage|}("hi");
            }
            """,
        fixedSource: """
            class Foo
            {
                void Bar() => Sentry.Godot.SentrySdk.CaptureMessage("hi");
            }
            """);

    [Fact]
    public Task BareIdentifierMemberCall_Rewritten() => RunAsync(
        source: """
            using Sentry;
            class Foo
            {
                void Bar() => {|SENTRYGD1001:SentrySdk.CaptureMessage|}("hi");
            }
            """,
        fixedSource: """
            using Sentry;
            class Foo
            {
                void Bar() => Sentry.Godot.SentrySdk.CaptureMessage("hi");
            }
            """);
}
