# Helper functions dot-sourced in integration testing


function script:Write-GitHub {
    param (
        [Parameter(Mandatory=$true)]
        [string]$message
    )
    if ($env:GITHUB_ACTIONS) {
        Write-Host "${message}"
    }
}


function script:ConvertTo-AndroidExtras {
    param (
        [Parameter(Mandatory=$true)]
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [string[]]$Arguments
    )

    if (-not $Arguments -or $Arguments.Count -eq 0) {
        return @()
    }

    # Convert argument array to Android intent extras format
    $extras = @()
    for ($i = 0; $i -lt $Arguments.Count; $i++) {
        $arg = $Arguments[$i]
        if (-not ($arg[0] -eq '"' -and $arg[-1] -eq '"')) {
            $arg = "$arg" -replace '"', '\"'
            $arg = "`"$arg`""
        }

        $extras += @("--es", "arg$i", $arg)
    }

    return $extras
}

function script:Connect-IntegrationTestDevice {
    param (
        [Parameter(Mandatory=$true)]
        [PSCustomObject]$TestSetup
    )

    $connectParameters = @{ Platform = $TestSetup.Platform }
    if (-not [string]::IsNullOrEmpty($TestSetup.Device)) {
        $connectParameters.Target = $TestSetup.Device
    }

    Connect-Device @connectParameters
    Install-DeviceApp -Path $TestSetup.Executable

    if ($TestSetup.Platform -eq "Adb") {
        $TestSetup.AndroidComponent = Resolve-AdbActivityComponent `
            -Component $TestSetup.AndroidComponent `
            -DeviceSerial (Get-DeviceSession).Identifier
    }
}

function script:Resolve-AdbActivityComponent {
    param (
        [Parameter(Mandatory=$true)]
        [string]$Component,
        [Parameter(Mandatory=$true)]
        [string]$DeviceSerial
    )

    $componentParts = $Component -split "/", 2
    $packageName = $componentParts[0]
    $resolvedOutput = & adb -s $DeviceSerial shell cmd package resolve-activity --brief $packageName 2>$null
    $resolveExitCode = $LASTEXITCODE
    $resolvedComponents = @($resolvedOutput | Where-Object { $_ -like "$packageName/*" })

    if ($resolveExitCode -eq 0 -and $resolvedComponents.Count -gt 0) {
        return $resolvedComponents[-1].Trim()
    }

    Write-Warning "Could not resolve the Android launcher activity. Falling back to '$Component'."
    return $Component
}
