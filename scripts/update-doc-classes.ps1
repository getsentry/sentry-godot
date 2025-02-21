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

$startDir = Get-Location
$scriptDir = Split-Path -Parent (Resolve-Path $MyInvocation.MyCommand.Path)
Set-Location "$scriptDir/../project"

& $godot --doctool ../ --gdextension-docs

Set-Location $startDir
