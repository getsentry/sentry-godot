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
