using System.Runtime.CompilerServices;
using VerifyTests;
using VerifyTests.DiffPlex;

internal static class ModuleInitializer
{
    [ModuleInitializer]
    public static void Init()
    {
        VerifyDiffPlex.Initialize(OutputType.Compact);
    }
}
