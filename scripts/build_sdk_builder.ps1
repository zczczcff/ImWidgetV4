param(
    [string]$BuildDir = "build/sdk-builder-release",
    [string]$Generator = "",
    [string]$Platform = "x64",
    [string]$Configuration = "Release",
    [string]$OutputPath = "ImWidgetSDKBuilder.exe",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildDir
$outputFile = Join-Path $repoRoot $OutputPath

if ($Clean -and (Test-Path $buildPath)) {
    Write-Host "[sdk-builder] Removing build directory: $buildPath"
    Remove-Item -LiteralPath $buildPath -Recurse -Force
}

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-DIMWIDGETV4_BUILD_SDK_BUILDER=ON",
    "-DIMWIDGETV4_BUILD_EDITOR=OFF",
    "-DIMWIDGETV4_BUILD_CLI=OFF",
    "-DIMWIDGETV4_BUILD_TESTS=OFF",
    "-DIMWIDGETV4_BUILD_SAMPLES=OFF",
    "-DIMWIDGETV4_ENABLE_CPACK=OFF"
)

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs += @("-G", $Generator)
}

if (-not [string]::IsNullOrWhiteSpace($Platform)) {
    $configureArgs += @("-A", $Platform)
}

Write-Host "[sdk-builder] Configuring Release builder..."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "SDK Builder configure failed with exit code $LASTEXITCODE."
}

Write-Host "[sdk-builder] Building ImWidgetSDKBuilder ($Configuration)..."
& cmake --build $buildPath --config $Configuration --target ImWidgetSDKBuilder
if ($LASTEXITCODE -ne 0) {
    throw "SDK Builder build failed with exit code $LASTEXITCODE."
}

$candidatePaths = @(
    (Join-Path $buildPath "ImWidgetSDKBuilder\$Configuration\ImWidgetSDKBuilder.exe"),
    (Join-Path $buildPath "$Configuration\ImWidgetSDKBuilder.exe"),
    (Join-Path $buildPath "ImWidgetSDKBuilder\ImWidgetSDKBuilder.exe")
)

$builtExe = $candidatePaths | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $builtExe) {
    throw "Built ImWidgetSDKBuilder.exe was not found under $buildPath."
}

Copy-Item -LiteralPath $builtExe -Destination $outputFile -Force

Write-Host "[sdk-builder] Copied: $outputFile"
Write-Host "[sdk-builder] Run it from the repository root before building full SDK packages."
