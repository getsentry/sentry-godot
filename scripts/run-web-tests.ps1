#!/usr/bin/env pwsh

# Run GDScript suite and isolated tests in a headless browser using Playwright.
# Requires a Godot web export in exports/web/ (or WEB_EXPORT_DIR).

$testWebDir = Join-Path $PSScriptRoot "../test_web" | Resolve-Path

$startDir = Get-Location
Set-Location $testWebDir

# Install/update npm dependencies if needed.
$nodeModules = Join-Path $testWebDir "node_modules"
if (-not (Test-Path $nodeModules)) {
    Write-Host "Installing npm dependencies..." -ForegroundColor Cyan
    npm install
    if ($LASTEXITCODE -ne 0) { Set-Location $startDir; exit $LASTEXITCODE }
    npx playwright install chromium
    if ($LASTEXITCODE -ne 0) { Set-Location $startDir; exit $LASTEXITCODE }
} else {
    $lockFile = Join-Path $testWebDir "package-lock.json"
    if ((Test-Path $lockFile) -and (Get-Item $lockFile).LastWriteTime -gt (Get-Item $nodeModules).LastWriteTime) {
        Write-Host "package-lock.json changed, running npm install..." -ForegroundColor Cyan
        npm install
        if ($LASTEXITCODE -ne 0) { Set-Location $startDir; exit $LASTEXITCODE }
        npx playwright install chromium
        if ($LASTEXITCODE -ne 0) { Set-Location $startDir; exit $LASTEXITCODE }
    }
}

# Run Playwright tests.
npx playwright test
$result = $LASTEXITCODE

Set-Location $startDir
exit $result
