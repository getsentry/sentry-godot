#!/usr/bin/env pwsh

# Updates the built-in class reference documentation by generating XML files for
# new classes and updating existing ones. This process removes and adds members
# as needed, but does not handle renaming automatically.

$godot = $env:GODOT

if (-not $godot) {
    Write-Host "GODOT environment variable is not set. Defaulting to `"godot`"."
    $godot = "godot"
}

if (-not (Get-Command $godot -ErrorAction SilentlyContinue)) {
    Write-Error "Godot executable not found. Please set the GODOT environment variable." -CategoryActivity "ERROR"
    exit 1
}

Push-Location "$PSScriptRoot/../project"

try {
    # Without an import pass the doctool finds no classes, erases every file in doc_classes/ and still exits 0.
    Write-Host "Importing project..."
    & $godot --headless --path . --import
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Project import failed with exit code $LASTEXITCODE. Aborting." -CategoryActivity "ERROR"
        exit 1
    }

    Write-Host "Generating class reference..."
    & $godot --doctool ../ --gdextension-docs
} finally {
    Pop-Location
}
