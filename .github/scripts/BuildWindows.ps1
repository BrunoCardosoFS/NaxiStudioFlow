# .github/scripts/BuildWindows.ps1
param (
    [string]$BuildDir = "build_windows",
    [string]$Config = "Release"
)

$ProjectName = $env:PROGRAM_NAME
$AbsDistFilesDir = Resolve-Path -Path $env:DIST_FILES_DIR
$AbsDistWindowsDir = Resolve-Path -Path $env:DIST_WINDOWS_DIR

Write-Host "--- Starting CMake Configuration ---" -ForegroundColor Green
cmake -B $BuildDir -S . -DCMAKE_BUILD_TYPE=$Config -DAPP_VERSION="$env:VERSION" -DVERSION_MAJOR="$env:VERSION_MAJOR"  -DVERSION_MINOR="$env:VERSION_MINOR"  -DVERSION_PATCH="$env:VERSION_PATCH"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }


Write-Host "--- Compiling ---" -ForegroundColor Green
cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }


Write-Host "--- Preparing Deploy ---" -ForegroundColor Green
Copy-Item "$BuildDir\$Config\flow.exe" -Destination $AbsDistWindowsDir
Copy-Item "COPYING" -Destination $AbsDistWindowsDir
Remove-Item -Recurse -Force $BuildDir


Write-Host "--- Windeployqt ---" -ForegroundColor Green
windeployqt --dir $AbsDistWindowsDir --no-translations "$AbsDistWindowsDir\flow.exe"


Write-Host "--- Creating a ZIP File ---" -ForegroundColor Green
$ZipName = "$AbsDistFilesDir\${ProjectName}-${env:VERSION}-Portable-Windows-x86_64.zip"
Compress-Archive -Path "$AbsDistWindowsDir\*" -DestinationPath $ZipName -Force
Write-Host "Build and Packaging completed: $ZipName"
