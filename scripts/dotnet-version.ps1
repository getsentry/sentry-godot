#!/usr/bin/env pwsh

# Updates Sentry .NET SDK dependency version across project files.

Set-StrictMode -Version latest

# Files to update.
$versionsPropsFile = "$PSScriptRoot/../project/addons/sentry/dotnet/Sentry.Godot.Versions.props"

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
	Test-FileExists $versionsPropsFile

	$content = Get-Content $versionsPropsFile -Raw
	$match = [regex]::Match($content, '<SentryVersion>([^<]+)</SentryVersion>')
	if ($match.Success)
	{
		return $match.Groups[1].Value
	}

	Write-Error "Could not find SentryVersion in $versionsPropsFile"
	exit 1
}

function Get-RepositoryUrl
{
	return "https://github.com/getsentry/sentry-dotnet"
}

function Set-SentryDotnetVersion
{
	param(
		[Parameter(Mandatory=$true)]
		[string]$Version
	)

	Test-FileExists $versionsPropsFile

	$currentVersion = Get-CurrentVersion
	if ($currentVersion -eq $Version)
	{
		Write-Host "$(Split-Path $versionsPropsFile -Leaf) already declares version $Version"
		Write-Host "No changes needed."
		return
	}

	Write-Host "Updating $(Split-Path $versionsPropsFile -Leaf) from version $currentVersion to $Version"
	$content = Get-Content $versionsPropsFile -Raw
	$updated = $content.Replace("<SentryVersion>$currentVersion</SentryVersion>", "<SentryVersion>$Version</SentryVersion>")
	[System.IO.File]::WriteAllText((Resolve-Path $versionsPropsFile), $updated)

	# Verify write succeeded
	$verifyVersion = Get-CurrentVersion
	if ($verifyVersion -ne $Version)
	{
		throw "Update failed - read-after-write: '$verifyVersion', expected '$Version'"
	}

	Write-Host "Successfully updated Sentry .NET dependency to version $Version."
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
		Set-SentryDotnetVersion -Version $version
	}
	default
	{
		Write-Error "Unknown action: $action"
		Write-Usage
		exit 1
	}
}
