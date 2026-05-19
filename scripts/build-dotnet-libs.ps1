#!/usr/bin/env pwsh

# Build the .NET layer (Sentry.Godot + analyzer) and stage the resulting
# artifacts into the addon under project/addons/sentry/dotnet/lib/.
#
# Source generators are not staged: they run inside Sentry.Godot's own build
# (via the ProjectReference in Sentry.Godot.csproj) and their output is baked
# into Sentry.Godot.dll.

[CmdletBinding()]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $true

$repoRoot   = Resolve-Path (Join-Path $PSScriptRoot "..")
$managedDir = Join-Path $repoRoot "src/sentry/dotnet/managed"
$addonDir   = Join-Path $repoRoot "project/addons/sentry/dotnet"
$libDir     = Join-Path $addonDir "lib"

Write-Host "==> Cleaning $libDir" -ForegroundColor Cyan
if (Test-Path $libDir) { Remove-Item -Recurse -Force $libDir }
New-Item -ItemType Directory -Path $libDir | Out-Null

Write-Host "==> Building Sentry.Godot (net8.0)" -ForegroundColor Cyan
dotnet build (Join-Path $managedDir "Sentry.Godot/Sentry.Godot.csproj") -c $Configuration

$sentryGodotBin = Join-Path $managedDir "Sentry.Godot/bin/$Configuration/net8.0"
foreach ($ext in "dll", "pdb", "xml") {
    $src = Join-Path $sentryGodotBin "Sentry.Godot.$ext"
    if (Test-Path $src) {
        Copy-Item $src $libDir
    } else {
        Write-Warning "Expected artifact not found: $src"
    }
}

Write-Host "==> Building Sentry.Godot.Analyzers" -ForegroundColor Cyan
dotnet build (Join-Path $managedDir "Sentry.Godot.Analyzers/Sentry.Godot.Analyzers.csproj") -c $Configuration
Copy-Item (Join-Path $managedDir "Sentry.Godot.Analyzers/bin/$Configuration/netstandard2.0/Sentry.Godot.Analyzers.dll") $libDir

Write-Host "==> Copying Versions.props to addon" -ForegroundColor Cyan
Copy-Item (Join-Path $managedDir "Sentry.Godot.Versions.props") $addonDir

Write-Host "==> Done. Staged files:" -ForegroundColor Green
Get-ChildItem -Recurse $libDir | Where-Object { -not $_.PSIsContainer } | ForEach-Object { Write-Host "  $($_.FullName.Substring($repoRoot.Path.Length + 1))" }
