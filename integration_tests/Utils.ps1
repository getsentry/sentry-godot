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
        [string]$Arguments
    )

    if ([string]::IsNullOrWhiteSpace($Arguments)) {
        return ""
    }

    # Split arguments into tokens, respecting quoted strings
    $tokens = @()
    $current = ""
    $inQuotes = $false
    $escapeNext = $false

    for ($i = 0; $i -lt $Arguments.Length; $i++) {
        $char = $Arguments[$i]

        if ($escapeNext) {
            $current += $char
            $escapeNext = $false
        }
        elseif ($char -eq '\') {
            $escapeNext = $true
        }
        elseif ($char -eq '"' -or $char -eq "'") {
            $inQuotes = -not $inQuotes
        }
        elseif ($char -eq ' ' -and -not $inQuotes) {
            if ($current.Length -gt 0) {
                $tokens += $current
                $current = ""
            }
        }
        else {
            $current += $char
        }
    }

    # Add the last token if it exists
    if ($current.Length -gt 0) {
        $tokens += $current
    }

    # Convert tokens to Android intent extras format
    $extras = ""
    for ($i = 0; $i -lt $tokens.Count; $i++) {
        $extras += " --es arg$i `"$($tokens[$i])`""
    }

    return $extras.TrimStart()
}
