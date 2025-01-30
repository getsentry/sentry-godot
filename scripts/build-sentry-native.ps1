Push-Location modules\sentry-native

$curDir = Get-Location
$buildDir = Join-Path -Path $curDir -ChildPath "build"
$installDir = Join-Path -Path $curDir -ChildPath "install"
$pdbBuildDir = Join-Path -Path $buildDir -ChildPath "pdb"
$pdbSourceDir = Join-Path -Path $pdbBuildDir -ChildPath "RelWithDebInfo"
$libInstallDir = Join-Path -Path $installDir -ChildPath "lib"

cmake -B $buildDir -DSENTRY_BUILD_SHARED_LIBS=OFF -DSENTRY_BUILD_RUNTIMESTATIC=ON -DSENTRY_BACKEND=crashpad -DSENTRY_SDK_NAME="sentry.native.godot" -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DCMAKE_COMPILE_PDB_OUTPUT_DIRECTORY="$pdbBuildDir"
cmake --build $buildDir --target sentry --parallel --config RelWithDebInfo
cmake --build $buildDir --target crashpad_handler --parallel --config RelWithDebInfo
cmake --install $buildDir --prefix $installDir --config RelWithDebInfo

# *** Install PDB files

Write-Host "Installing PDB files"

if (!(Test-Path -Path $libInstallDir)) {
    Throw "Directory $libInstallDir does not exist."
}
if (!(Test-Path -Path $pdbSourceDir)) {
    Throw "Directory $pdbSourceDir does not exist."
}

Get-ChildItem -Path $pdbSourceDir -Filter "*.pdb" -Recurse | ForEach-Object {
    $pdbFile = $_.Name
    $pdbSource = $_.FullName
    $pdbTarget = Join-Path -Path $libInstallDir -ChildPath $pdbFile
    $libBase = [System.IO.Path]::GetFileNameWithoutExtension($pdbFile)
    $libPath = Join-Path -Path $libInstallDir -ChildPath "$libBase.lib"

    if (Test-Path -Path $libPath) {
        if (Test-Path -Path $pdbTarget) {
            $sourceHash = Get-FileHash -Path $pdbSource -Algorithm SHA256
            $targetHash = Get-FileHash -Path $pdbTarget -Algorithm SHA256
            if ($sourceHash.Hash -eq $targetHash.Hash) {
                Write-Host "-- Up-to-date: $pdbTarget."
                return
            }
        }
        Write-Host "-- Installing: $pdbFile to $libInstallDir"
        Copy-Item -Path $pdbSource -Destination $pdbTarget -Force
    }
}

Pop-Location
