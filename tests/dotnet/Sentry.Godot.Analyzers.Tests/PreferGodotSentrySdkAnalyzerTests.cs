using System.Threading.Tasks;
using Microsoft.CodeAnalysis.CSharp.Testing;
using Microsoft.CodeAnalysis.Testing;
using Sentry.Godot.Analyzers;
using Xunit;

namespace Sentry.Godot.Analyzers.Tests;

public class PreferGodotSentrySdkAnalyzerTests
{
    private static Task RunAsync(string source) =>
        new CSharpAnalyzerTest<PreferGodotSentrySdkAnalyzer, DefaultVerifier>
        {
            ReferenceAssemblies = ReferenceAssemblies.Net.Net80,
            TestState =
            {
                Sources = { source, TestSources.GodotSdkStub },
                AdditionalReferences = { typeof(global::Sentry.SentrySdk).Assembly },
            },
        }.RunAsync();

    [Fact]
    public Task Init_Flagged() => RunAsync("""
        class Foo
        {
            void Bar() => {|SENTRYGD1001:Sentry.SentrySdk.Init|}();
        }
        """);

    [Fact]
    public Task FullyQualifiedMemberCall_Flagged() => RunAsync("""
        class Foo
        {
            void Bar() => {|SENTRYGD1001:Sentry.SentrySdk.CaptureMessage|}("hi");
        }
        """);

    [Fact]
    public Task BareIdentifierMemberCall_Flagged() => RunAsync("""
        using Sentry;
        class Foo
        {
            void Bar() => {|SENTRYGD1001:SentrySdk.CaptureMessage|}("hi");
        }
        """);

    [Fact]
    public Task GodotSdkCall_NotFlagged() => RunAsync("""
        class Foo
        {
            void Bar() => Sentry.Godot.SentrySdk.CaptureMessage("hi");
        }
        """);

    [Fact]
    public Task UnrelatedCode_NotFlagged() => RunAsync("""
        class Foo
        {
            void Bar() => System.Console.WriteLine("hi");
        }
        """);

    // A user-defined nested 'SentrySdk' isn't the same symbol as the top-level
    // 'Sentry.SentrySdk' that the analyzer resolves via GetTypeByMetadataName.
    [Fact]
    public Task NestedSentrySdkInSentryNamespace_NotFlagged() => RunAsync("""
        namespace Sentry;
        public class Outer
        {
            public class SentrySdk
            {
                public static void CaptureMessage(string message) { }
            }
        }
        class Foo
        {
            void Bar() => Outer.SentrySdk.CaptureMessage("hi");
        }
        """);

    [Fact]
    public Task PropertyReference_Flagged() => RunAsync("""
        class Foo
        {
            bool Bar() => {|SENTRYGD1001:Sentry.SentrySdk.IsEnabled|};
        }
        """);

    [Fact]
    public Task UsingStaticInvocation_Flagged() => RunAsync("""
        using static Sentry.SentrySdk;
        class Foo
        {
            void Bar() => {|SENTRYGD1001:CaptureMessage|}("hi");
        }
        """);
}
