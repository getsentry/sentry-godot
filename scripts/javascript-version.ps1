#!/usr/bin/env pwsh

# Updates Sentry JavaScript dependency version across project files.

Set-StrictMode -Version latest

# Paths.
$bridgeDir = "$PSScriptRoot/../src/sentry/javascript/bridge"
$packageJsonFile = "$bridgeDir/package.json"

function Write-Usage
{
	Write-Host "Usage: $(Split-Path $MyInvocation.ScriptName -Leaf) <action> [version]"
	Write-Host "Actions:"
	Write-Host "  get-version           - return the currently specified dependency version"
	Write-Host "  get-repo              - return the repository url"
	Write-Host "  set-version <version> - update the dependency version"
}

function Test-FileExists($filePath)
{
	if (-not(Test-Path $filePath))
	{
		Write-Error "$filePath not found"
		exit 1
	}
}

function Get-CurrentVersion
{
	Test-FileExists $packageJsonFile

	$packageJson = Get-Content $packageJsonFile -Raw | ConvertFrom-Json
	$currentVersion = $packageJson.dependencies.'@sentry/browser'
	if ($currentVersion)
	{
		# Strip version prefix (e.g. ^, ~)
		return $currentVersion -replace '^[\^~]', ''
	}

	Write-Error "Could not find @sentry/browser version in package.json"
	exit 1
}

function Get-RepositoryUrl
{
	return "https://github.com/getsentry/sentry-javascript"
}

function Set-SentryJavaScriptVersion
{
	param(
		[Parameter(Mandatory=$true)]
		[string]$Version
	)

	Test-FileExists $packageJsonFile

	$packageJson = Get-Content $packageJsonFile -Raw | ConvertFrom-Json
	$browserVersion = $packageJson.dependencies.'@sentry/browser' -replace '^[\^~]', ''
	$wasmVersion = $packageJson.dependencies.'@sentry/wasm' -replace '^[\^~]', ''

	if ($browserVersion -eq $Version -and $wasmVersion -eq $Version)
	{
		Write-Host "No changes needed."
		return
	}

	Write-Host "Updating Sentry JavaScript dependencies to version $Version"

	# Read file as text and replace versions to preserve formatting
	$content = Get-Content $packageJsonFile -Raw
	$content = $content -replace '"@sentry/browser": "[\^~]?[^"]+"', ('"@sentry/browser": "^' + $Version + '"')
	$content = $content -replace '"@sentry/wasm": "[\^~]?[^"]+"', ('"@sentry/wasm": "^' + $Version + '"')
	[System.IO.File]::WriteAllText((Resolve-Path $packageJsonFile), $content)

	# Verify write succeeded
	$verifyJson = Get-Content $packageJsonFile -Raw | ConvertFrom-Json
	$readBrowser = $verifyJson.dependencies.'@sentry/browser' -replace '^[\^~]', ''
	$readWasm = $verifyJson.dependencies.'@sentry/wasm' -replace '^[\^~]', ''
	if ($readBrowser -ne $Version -or $readWasm -ne $Version)
	{
		throw "Update failed - read-after-write: @sentry/browser='$readBrowser', @sentry/wasm='$readWasm', expected '$Version'"
	}

	# Update package-lock.json
	Write-Host "Running npm install in $bridgeDir"
	Push-Location $bridgeDir
	try
	{
		npm install
		if ($LASTEXITCODE -ne 0)
		{
			throw "npm install failed with exit code $LASTEXITCODE"
		}
	} finally
	{
		Pop-Location
	}

	Write-Host "Successfully updated Sentry JavaScript dependencies to version $Version."
}

# ----------------------- Main -----------------------

if ($args.Count -eq 0)
{
	Write-Usage
	exit 0
}

$action = $args[0]

switch ($action)
{
	"get-version"
	{
		Write-Output (Get-CurrentVersion)
	}
	"get-repo"
	{
		Write-Output (Get-RepositoryUrl)
	}
	"set-version"
	{
		if ($args.Count -lt 2)
		{
			Write-Error "set-version requires a version argument"
			Write-Usage
			exit 1
		}
		$version = $args[1]
		Set-SentryJavaScriptVersion -Version $version
	}
	default
	{
		Write-Error "Unknown action: $action"
		Write-Usage
		exit 1
	}
}
