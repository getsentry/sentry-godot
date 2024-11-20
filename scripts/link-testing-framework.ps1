# Create symbolic link in the demo project for gdUnit4 testing framework on Windows platform.
# Note: Creating symbolic links requires elevated privileges on a default Windows installation.

$src = "modules\gdUnit4\addons\gdUnit4"
$dst = "project\addons\gdUnit4"

$repoRoot = Resolve-Path (Split-Path -Path $PSScriptRoot -Parent)

# Stop script execution on error.
$ErrorActionPreference = "Stop"

if (Test-Path (Join-Path -Path $repoRoot -ChildPath $dst)) {
    Write-Host "Target path '$dst' already exists. Exiting."
    exit
}

# Checks if the script is running with elevated privileges.
function Test-Admin {
    $currentIdentity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
    $currentPrincipal = New-Object System.Security.Principal.WindowsPrincipal($currentIdentity)
    return $currentPrincipal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)
}

# If not running as administrator, start a new PowerShell process with elevated privileges.
if (-not (Test-Admin)) {
    $args = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    Start-Process powershell -ArgumentList $args -Verb RunAs
    exit
}

Write-Host "Running as Administrator..."
Push-Location -Path $repoRoot

# Create the symlink.
New-Item -ItemType SymbolicLink -Path $dst -Target $src

Write-Host
Write-Host "Symlink created '$src' => '$dst'"
Pop-Location
