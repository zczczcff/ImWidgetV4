param(
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$SmokeDir = "build/package-sdk-smoke",
    [string]$Generator = "",
    [string]$Platform = "",
    [string[]]$Architectures = @(),
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
if ($Architectures.Count -gt 0) {
    $packageArgs.Architectures = $Architectures
}

Write-Host "[sdk] Building ImWidgetV4 SDK package..."
& $packageScript @packageArgs
if ($LASTEXITCODE -ne 0) {
    throw "SDK package build failed with exit code $LASTEXITCODE."
}

if (-not $SkipSmoke) {
    foreach ($configuration in $configurations) {
        $smokeArchitectures = if ($Architectures.Count -gt 0) { $Architectures } elseif (-not [string]::IsNullOrWhiteSpace($Platform)) { @($Platform) } else { @("win32", "win64") }
        foreach ($architecture in $smokeArchitectures) {
            $architectureName = $architecture.Trim().ToLowerInvariant()
            if ($architectureName -eq "x86") { $architectureName = "win32" }
            if ($architectureName -eq "x64" -or $architectureName -eq "amd64") { $architectureName = "win64" }
            $configurationSmokeDir = Join-Path $SmokeDir "$architectureName-$configuration"
            $smokeArgs = @{
                PackageDir = $PackageDir
                Configuration = $configuration
                SmokeDir = $configurationSmokeDir
                Architecture = $architectureName
            }
            if (-not [string]::IsNullOrWhiteSpace($Generator)) {
                $smokeArgs.Generator = $Generator
            }

            Write-Host "[sdk] Running SDK smoke test ($architectureName/$configuration)..."
            & $smokeScript @smokeArgs
            if ($LASTEXITCODE -ne 0) {
                throw "SDK smoke test failed for $architectureName/$configuration with exit code $LASTEXITCODE."
            }
        }
    }
}

Write-Host "[sdk] SDK package ready: $(Join-Path $repoRoot $PackageDir)"
