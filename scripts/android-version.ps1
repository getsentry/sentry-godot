#!/usr/bin/env pwsh

# Updates Sentry Android dependency version across project files.

Set-StrictMode -Version latest

# Files to update.
$gradleFile = "$PSScriptRoot/../android_lib/build.gradle.kts"
$exportPluginFile = "$PSScriptRoot/../src/editor/sentry_editor_export_plugin_android.cpp"

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
	Test-FileExists $gradleFile
	Test-FileExists $exportPluginFile

	$gradleContent = Get-Content $gradleFile
	$currentVersion = [regex]::Match("$gradleContent", 'implementation\("io\.sentry:sentry-android:([^"]+)"\)').Groups[1].Value
	if ($currentVersion)
	{
		return $currentVersion
	}

	Write-Error "Could not find current version in the project files"
	exit 1
}

function Get-RepositoryUrl
{
	return "https://github.com/getsentry/sentry-java"
}

function Set-SentryAndroidVersion
{
	param(
		[Parameter(Mandatory=$true)]
		[string]$Version
	)

	Test-FileExists $gradleFile
	Test-FileExists $exportPluginFile

	$performedChanges = $false

	# Update gradle file if version differs
	$gradleContent = Get-Content $gradleFile
	$currentGradleVersion = [regex]::Match("$gradleContent", 'implementation\("io\.sentry:sentry-android:([^"]+)"\)').Groups[1].Value
	if ($currentGradleVersion -ne $Version)
	{
		Write-Host "Updating $(Split-Path $gradleFile -Leaf) from version $currentGradleVersion to $Version"
		$gradleContent -replace 'implementation\("io\.sentry:sentry-android:([^"]+)"\)', ('implementation("io.sentry:sentry-android:' + $Version + '")') | Out-File $gradleFile
		$performedChanges = $true
	} else
	{
		Write-Host "$(Split-Path $gradleFile -Leaf) already declares version $Version"
	}

	# Update export plugin file if version differs
	$exportContent = Get-Content $exportPluginFile
	$currentExportVersion = [regex]::Match("$exportContent", 'deps\.append\("io\.sentry:sentry-android:([^"]+)"\);').Groups[1].Value
	if ($currentExportVersion -ne $Version)
	{
		Write-Host "Updating $(Split-Path $exportPluginFile -Leaf) from version $currentExportVersion to $Version"
		$exportContent -replace 'deps\.append\("io\.sentry:sentry-android:([^"]+)"\);', ('deps.append("io.sentry:sentry-android:' + $Version + '");') | Out-File $exportPluginFile
		$performedChanges = $true
	} else
	{
		Write-Host "$(Split-Path $exportPluginFile -Leaf) already declares version $Version"
	}

	if (-not $performedChanges)
	{
		Write-Host "No changes needed."
	} else
	{
		Write-Host "Successfully updated Sentry Android dependency to version $Version in all files."
	}
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
		$currentVersion = Get-CurrentVersion
		Write-Output $currentVersion
	}
	"get-repo"
	{
		$repoUrl = Get-RepositoryUrl
		Write-Output $repoUrl
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
		Set-SentryAndroidVersion -Version $version
	}
	default
	{
		Write-Error "Unknown action: $action"
		Write-Usage
		exit 1
	}
}
