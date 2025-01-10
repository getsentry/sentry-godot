Param(
    [Parameter(Mandatory = $true)][String]$prevVersion,
    [Parameter(Mandatory = $true)][String]$newVersion
)
Set-StrictMode -Version latest

git checkout main

if ($newVersion -match '^(?<major>\d+)\.(?<minor>\d+)\.(?<patch>\d+)(?<status>.*)?$') {
    $major = [int]$matches['major']
    $minor = [int]$matches['minor']
    $patch = [int]$matches['patch']
    $status = $matches['status']

    if ($status -match "^-(?<prerelease>[a-zA-Z]+)\.?(?<prereleaseNum>\d+)?$") {
        # Increment prerelease version
        $prerelease = $matches['prerelease']
        $prereleaseNum = [int]$matches['prereleaseNum']
        $prereleaseNum += 1
        $status = "-$prerelease.$prereleaseNum"
    } else {
        # Increment minor version, reset patch version, and add -dev prerelease status
        $minor += 1
        $patch = 0
        $status = "-dev"
    }

    $nextVersion = "$major.$minor.$patch$status"

    & ".\scripts\bump-version.ps1" -prevVersion $newVersion -newVersion $nextVersion

    git diff --quiet
    if ($LASTEXITCODE -ne 0) {
        git commit -anm 'meta: Bump new development version'
        git pull --rebase
        git push
    }

} else {`
    Throw "Failed to find version in $sconsFile"
}
