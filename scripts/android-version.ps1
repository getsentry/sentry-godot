#!/usr/bin/env pwsh

# Updates Sentry Android dependency versions across project files.

Set-StrictMode -Version latest

# Files to update.
$gradleFile = "$PSScriptRoot/../android_lib/build.gradle.kts"
$exportPluginFile = "$PSScriptRoot/../src/editor/sentry_editor_export_plugin_android.cpp"

# Dependencies to manage
$dependencies = @(
	@{ 
		Name = "sentry-android"
		Group = "io.sentry"
		GradlePattern = 'implementation\("io\.sentry:sentry-android:([^"]+)"\)'
		CppPattern = 'deps\.append\("io\.sentry:sentry-android:([^"]+)"\);'
	},
	@{ 
		Name = "sentry-android-ndk"
		Group = "io.sentry"
		GradlePattern = 'implementation\("io\.sentry:sentry-android-ndk:([^"]+)"\)'
		CppPattern = 'deps\.append\("io\.sentry:sentry-android-ndk:([^"]+)"\);'
	}
)

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
	
	# Get version from the first dependency (they should all be the same)
	$firstDep = $dependencies[0]
	$currentVersion = [regex]::Match("$gradleContent", $firstDep.GradlePattern).Groups[1].Value
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

	# Update gradle file
	$gradleContent = Get-Content $gradleFile
	$updatedGradleContent = $gradleContent
	
	foreach ($dep in $dependencies) {
		$currentGradleVersion = [regex]::Match("$gradleContent", $dep.GradlePattern).Groups[1].Value
		if ($currentGradleVersion -and $currentGradleVersion -ne $Version)
		{
			Write-Host "Updating $(Split-Path $gradleFile -Leaf) $($dep.Name) from version $currentGradleVersion to $Version"
			$replacement = 'implementation("' + $dep.Group + ':' + $dep.Name + ':' + $Version + '")'
			$updatedGradleContent = $updatedGradleContent -replace $dep.GradlePattern, $replacement
			$performedChanges = $true
		} elseif ($currentGradleVersion -eq $Version) {
			Write-Host "$(Split-Path $gradleFile -Leaf) $($dep.Name) already declares version $Version"
		} else {
			Write-Warning "Could not find $($dep.Name) dependency in $(Split-Path $gradleFile -Leaf)"
		}
	}
	
	if ($updatedGradleContent -ne $gradleContent) {
		$updatedGradleContent | Out-File $gradleFile
	}

	# Update export plugin file
	$exportContent = Get-Content $exportPluginFile
	$updatedExportContent = $exportContent
	
	foreach ($dep in $dependencies) {
		$currentExportVersion = [regex]::Match("$exportContent", $dep.CppPattern).Groups[1].Value
		if ($currentExportVersion -and $currentExportVersion -ne $Version)
		{
			Write-Host "Updating $(Split-Path $exportPluginFile -Leaf) $($dep.Name) from version $currentExportVersion to $Version"
			$replacement = 'deps.append("' + $dep.Group + ':' + $dep.Name + ':' + $Version + '");'
			$updatedExportContent = $updatedExportContent -replace $dep.CppPattern, $replacement
			$performedChanges = $true
		} elseif ($currentExportVersion -eq $Version) {
			Write-Host "$(Split-Path $exportPluginFile -Leaf) $($dep.Name) already declares version $Version"
		} else {
			Write-Warning "Could not find $($dep.Name) dependency in $(Split-Path $exportPluginFile -Leaf)"
		}
	}
	
	if ($updatedExportContent -ne $exportContent) {
		$updatedExportContent | Out-File $exportPluginFile
	}

	if (-not $performedChanges)
	{
		Write-Host "No changes needed."
	} else
	{
		Write-Host "Successfully updated Sentry Android dependencies to version $Version in all files."
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