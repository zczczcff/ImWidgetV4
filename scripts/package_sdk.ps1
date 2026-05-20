param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-Release",
    [string]$Generator = "",
    [string]$Platform = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildDir
$packagePath = Join-Path $repoRoot $PackageDir

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-DIMWIDGETV4_BUILD_TESTS=OFF",
    "-DIMWIDGETV4_BUILD_SAMPLES=OFF",
    "-DIMWIDGETV4_BUILD_EDITOR=ON",
    "-DIMWIDGETV4_INSTALL_SDK_SUBDIR=sdk"
)

if ($Generator -ne "") {
    $configureArgs += @("-G", $Generator)
}
if ($Platform -ne "") {
    $configureArgs += @("-A", $Platform)
}

Write-Host "[package] Configuring SDK package build..."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

if (-not $SkipBuild) {
    Write-Host "[package] Building package targets ($Configuration)..."
    & cmake --build $buildPath --config $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE."
    }
}

if (Test-Path $packagePath) {
    Write-Host "[package] Removing previous package directory..."
    Remove-Item -LiteralPath $packagePath -Recurse -Force
}

Write-Host "[package] Installing SDK package..."
& cmake --install $buildPath --config $Configuration --prefix $packagePath
if ($LASTEXITCODE -ne 0) {
    throw "CMake install failed with exit code $LASTEXITCODE."
}

Write-Host "[package] Package ready: $packagePath"
