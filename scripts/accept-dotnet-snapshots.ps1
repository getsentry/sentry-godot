#!/usr/bin/env pwsh

# Regenerate and promote the .NET source-generator snapshot baseline.
# Use when the generator output has intentionally changed (e.g. after a
# Sentry NuGet bump): regenerates *.received.txt, moves it over *.verified.txt,
# then re-runs the tests to confirm the promoted baseline matches.

$PSNativeCommandUseErrorActionPreference = $false

$scriptDir = Split-Path -Parent (Resolve-Path $MyInvocation.MyCommand.Path)
$testDir = Join-Path $scriptDir "../tests/dotnet/Sentry.Godot.SourceGenerators.Tests"

Push-Location $testDir
try {
    Write-Host "Regenerating received snapshots..." -ForegroundColor Cyan
    dotnet test --nologo

    $received = @(Get-ChildItem -Filter "*.received.txt")
    if ($received.Count -eq 0) {
        Write-Host "No drift — nothing to promote." -ForegroundColor Green
        exit 0
    }

    foreach ($f in $received) {
        $dest = $f.FullName -replace "\.received\.txt$", ".verified.txt"
        Write-Host "Promoting $($f.Name) -> $(Split-Path -Leaf $dest)" -ForegroundColor Cyan
        Move-Item -Force $f.FullName $dest
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
