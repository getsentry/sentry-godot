#!/usr/bin/env pwsh

# Accept the drifted .NET source-generator snapshot as the new baseline.
# Use after intentional changes (e.g. a Sentry NuGet bump).

$PSNativeCommandUseErrorActionPreference = $false

$testDir = Join-Path $PSScriptRoot "../tests/dotnet/Sentry.Godot.SourceGenerators.Tests"

Push-Location $testDir
try {
    Write-Host "Regenerating received snapshots..." -ForegroundColor Cyan
    dotnet test --nologo
    $testExit = $LASTEXITCODE

    $received = @(Get-ChildItem -Filter "*.received.txt")
    if ($received.Count -eq 0) {
        if ($testExit -ne 0) {
            Write-Error "Tests failed without producing a snapshot — likely a compilation or runtime error unrelated to snapshot drift. See the output above."
            exit $testExit
        }
        Write-Host "No drift — nothing to promote." -ForegroundColor Green
        exit 0
    }

    foreach ($file in $received) {
        $dest = $file.FullName -replace "\.received\.txt$", ".verified.txt"
        Write-Host "Promoting $($file.Name) -> $(Split-Path -Leaf $dest)" -ForegroundColor Cyan
        Move-Item -Force $file.FullName $dest
    }

    Write-Host "Confirming promoted baseline..." -ForegroundColor Cyan
    dotnet test --nologo
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests still failing after promotion. Inspect the snapshot manually."
        exit $LASTEXITCODE
    }

    Write-Host "Promoted. Review git diff and commit the updated .verified.txt." -ForegroundColor Green
}
finally {
    Pop-Location
}
