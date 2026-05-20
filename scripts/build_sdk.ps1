param(
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$SmokeDir = "build/package-sdk-smoke",
    [string]$Generator = "",
    [string]$Platform = "",
    [switch]$SkipSmoke
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$packageScript = Join-Path $PSScriptRoot "package_sdk.ps1"
$smokeScript = Join-Path $PSScriptRoot "smoke_sdk_package.ps1"
$configurations = @("Debug", "Release")

$packageArgs = @{
    Configurations = $configurations
    BuildDir = $BuildDir
    PackageDir = $PackageDir
}

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $packageArgs.Generator = $Generator
}
if (-not [string]::IsNullOrWhiteSpace($Platform)) {
    $packageArgs.Platform = $Platform
}

Write-Host "[sdk] Building ImWidgetV4 SDK package..."
& $packageScript @packageArgs
if ($LASTEXITCODE -ne 0) {
    throw "SDK package build failed with exit code $LASTEXITCODE."
}

if (-not $SkipSmoke) {
    foreach ($configuration in $configurations) {
        $configurationSmokeDir = Join-Path $SmokeDir $configuration
        $smokeArgs = @{
            PackageDir = $PackageDir
            Configuration = $configuration
            SmokeDir = $configurationSmokeDir
        }
        if (-not [string]::IsNullOrWhiteSpace($Generator)) {
            $smokeArgs.Generator = $Generator
        }
        if (-not [string]::IsNullOrWhiteSpace($Platform)) {
            $smokeArgs.Platform = $Platform
        }

        Write-Host "[sdk] Running SDK smoke test ($configuration)..."
        & $smokeScript @smokeArgs
        if ($LASTEXITCODE -ne 0) {
            throw "SDK smoke test failed for $configuration with exit code $LASTEXITCODE."
        }
    }
}

Write-Host "[sdk] SDK package ready: $(Join-Path $repoRoot $PackageDir)"
