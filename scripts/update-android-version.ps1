#!/usr/bin/env pwsh

# Updates Sentry Android dependency version across project files.

# Fetch the latest release version from GitHub
Write-Host "Fetching latest Sentry Android release info from GitHub..."
try
{
	$response = Invoke-RestMethod -Uri "https://api.github.com/repos/getsentry/sentry-java/releases/latest"
	$latestVersion = $response.tag_name
	Write-Host "Latest release version found: $latestVersion"
} catch
{
	Write-Error "Failed to fetch latest release info from GitHub: $_"
	exit 1
}
Set-StrictMode -Version latest

$performedChanges = $false

# Update gradle file if version differs
$gradleFile = "$PSScriptRoot/../android_lib/build.gradle.kts"
$gradleContent = Get-Content $gradleFile
$currentGradleVersion = [regex]::Match("$gradleContent", 'implementation\("io\.sentry:sentry-android:([^"]+)"\)').Groups[1].Value
if ($currentGradleVersion -ne $latestVersion)
{
	Write-Host "Updating build.gradle.kts from version $currentGradleVersion to $latestVersion"
	$gradleContent -replace 'implementation\("io\.sentry:sentry-android:([^"]+)"\)', ('implementation("io.sentry:sentry-android:' + $latestVersion + '")') | Out-File $gradleFile
	$performedChanges = $true
} else
{
	Write-Host "build.gradle.kts already declares version $latestVersion"
}

# Update export plugin file if version differs
$exportPluginFile = "$PSScriptRoot/../src/editor/sentry_editor_export_plugin.cpp"
$exportContent = Get-Content $exportPluginFile
$currentExportVersion = [regex]::Match("$exportContent", 'deps\.append\("io\.sentry:sentry-android:([^"]+)"\);').Groups[1].Value
if ($currentExportVersion -ne $latestVersion)
{
	Write-Host "Updating sentry_editor_export_plugin.cpp from version $currentExportVersion to $latestVersion"
	$exportContent -replace 'deps\.append\("io\.sentry:sentry-android:([^"]+)"\);', ('deps.append("io.sentry:sentry-android:' + $latestVersion + '");') | Out-File $exportPluginFile
	$performedChanges = $true
} else
{
	Write-Host "sentry_editor_export_plugin.cpp already declares version $latestVersion"
}

if (-not $performedChanges)
{
	Write-Host "No changes needed."
} else
{
	Write-Host "Successfully updated Sentry Android dependency to version $latestVersion in all files."
}
