# .github/scripts/BuildWindows.ps1
param (
    [string]$BuildDir = "build_windows",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$ProjectName = $env:PROGRAM_NAME
$AbsDistFilesDir = Resolve-Path -Path $env:DIST_FILES_DIR
$AbsDistWindowsDir = Resolve-Path -Path $env:DIST_WINDOWS_DIR

Write-Host "--- Starting CMake Configuration ---" -ForegroundColor Green
cmake -B $BuildDir -S . -DCMAKE_BUILD_TYPE=$Config -DBUILD_TESTING=ON -DAPP_VERSION="$env:VERSION" -DVERSION_MAJOR="$env:VERSION_MAJOR" -DVERSION_MINOR="$env:VERSION_MINOR" -DVERSION_PATCH="$env:VERSION_PATCH"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "--- Compiling ---" -ForegroundColor Green
cmake --build $BuildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "--- Running Tests ---" -ForegroundColor Green
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir $BuildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "--- Preparing Portable Layout (bin/ + empty data/) ---" -ForegroundColor Green
$BinDir = Join-Path $AbsDistWindowsDir "bin"
$DataDir = Join-Path $AbsDistWindowsDir "data"

if (Test-Path $BinDir) { Remove-Item -Recurse -Force $BinDir }
if (Test-Path $DataDir) { Remove-Item -Recurse -Force $DataDir }
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
New-Item -ItemType Directory -Force -Path $DataDir | Out-Null

Copy-Item "$BuildDir\$Config\flow.exe" -Destination $BinDir
Copy-Item "COPYING" -Destination $AbsDistWindowsDir
Remove-Item -Recurse -Force $BuildDir

Write-Host "--- Windeployqt ---" -ForegroundColor Green
windeployqt --dir $BinDir --no-translations "$BinDir\flow.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "--- Creating Portable ZIP File ---" -ForegroundColor Green
$ZipName = Join-Path $AbsDistFilesDir "${ProjectName}-${env:VERSION}-Portable-Windows-x86_64.zip"
if (Test-Path $ZipName) { Remove-Item -Force $ZipName }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory(
    $AbsDistWindowsDir,
    $ZipName,
    [System.IO.Compression.CompressionLevel]::Optimal,
    $false
)
Write-Host "Build and Packaging completed: $ZipName"
