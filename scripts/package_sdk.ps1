param(
    [string]$Configuration = "",
    [string[]]$Configurations = @(),
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$Generator = "",
    [string]$Platform = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildDir
$packagePath = Join-Path $repoRoot $PackageDir
$requestedConfigurations = @()

if ($Configurations.Count -gt 0) {
    foreach ($entry in $Configurations) {
        foreach ($name in ($entry -split ",")) {
            $trimmedName = $name.Trim()
            if ($trimmedName -ne "") {
                $requestedConfigurations += $trimmedName
            }
        }
    }
} elseif ($Configuration -ne "") {
    $requestedConfigurations = @($Configuration)
} else {
    $requestedConfigurations = @("Debug", "Release")
}

$normalizedConfigurations = @()
foreach ($name in $requestedConfigurations) {
    if ($normalizedConfigurations -notcontains $name) {
        $normalizedConfigurations += $name
    }
}
$requestedConfigurations = $normalizedConfigurations

Write-Host "[package] Configurations: $($requestedConfigurations -join ', ')"

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-DIMWIDGETV4_BUILD_TESTS=OFF",
    "-DIMWIDGETV4_BUILD_SAMPLES=OFF",
    "-DIMWIDGETV4_BUILD_EDITOR=ON",
    "-DIMWIDGETV4_INSTALL_SDK_SUBDIR=sdk"
)

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs += @("-G", $Generator)
}
if (-not [string]::IsNullOrWhiteSpace($Platform)) {
    $configureArgs += @("-A", $Platform)
}

Write-Host "[package] Configuring SDK package build..."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

if (Test-Path $packagePath) {
    Write-Host "[package] Removing previous package directory..."
    Remove-Item -LiteralPath $packagePath -Recurse -Force
}

foreach ($currentConfiguration in $requestedConfigurations) {
    if (-not $SkipBuild) {
        Write-Host "[package] Building package targets ($currentConfiguration)..."
        & cmake --build $buildPath --config $currentConfiguration
        if ($LASTEXITCODE -ne 0) {
            throw "CMake build failed for $currentConfiguration with exit code $LASTEXITCODE."
        }
    }

    Write-Host "[package] Installing SDK package ($currentConfiguration)..."
    & cmake --install $buildPath --config $currentConfiguration --prefix $packagePath
    if ($LASTEXITCODE -ne 0) {
        throw "CMake install failed for $currentConfiguration with exit code $LASTEXITCODE."
    }
}

Write-Host "[package] Package ready: $packagePath"
