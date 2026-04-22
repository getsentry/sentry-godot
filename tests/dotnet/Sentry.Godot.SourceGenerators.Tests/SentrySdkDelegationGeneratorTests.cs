using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Sentry.Godot.SourceGenerators;
using Xunit;
using static VerifyXunit.Verifier;

namespace Sentry.Godot.SourceGenerators.Tests;

public class SentrySdkDelegationGeneratorTests
{
    private const string HintName = "SentrySdk.Dotnet.g.cs";

    [Fact]
    public Task Run()
    {
        var ct = TestContext.Current.CancellationToken;

        var source = """
            namespace Sentry.Godot;
            public static partial class SentrySdk { }
            """;

        var references = ((string)AppContext.GetData("TRUSTED_PLATFORM_ASSEMBLIES")!)
            .Split(Path.PathSeparator)
            .Select(p => (MetadataReference)MetadataReference.CreateFromFile(p));

        var compilation = CSharpCompilation.Create(
            assemblyName: "Sentry.Godot.Test",
            syntaxTrees: [CSharpSyntaxTree.ParseText(source, cancellationToken: ct)],
            references: references,
            options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));

        var driver = CSharpGeneratorDriver
            .Create(new SentrySdkDelegationGenerator())
            .RunGenerators(compilation, ct);

        var result = driver.GetRunResult().Results.Single();
        Assert.Null(result.Exception);
        var generated = Assert.Single(result.GeneratedSources);
        Assert.Equal(HintName, generated.HintName);

        return Verify(generated.SourceText.ToString());
    }
}
