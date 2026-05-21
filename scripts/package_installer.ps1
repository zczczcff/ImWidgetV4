param(
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$InstallerBuildDir = "build/package-installer-build",
    [string]$OutputDir = "build/package/installers",
    [string]$Generator = "",
    [string]$Platform = "",
    [string[]]$Architectures = @(),
    [string[]]$CpackGenerators = @(),
    [switch]$SkipSdkBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$packageScript = Join-Path $PSScriptRoot "package_sdk.ps1"
$packagePath = Join-Path $repoRoot $PackageDir
$installerBuildPath = Join-Path $repoRoot $InstallerBuildDir
$outputPath = Join-Path $repoRoot $OutputDir
$cpackSourcePath = Join-Path $installerBuildPath "src"
$cpackBuildPath = Join-Path $installerBuildPath "build"

function Clear-ReadOnlyAttribute {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    Get-ChildItem -LiteralPath $Path -Recurse -Force | ForEach-Object {
        $_.Attributes = $_.Attributes -band (-bnot [System.IO.FileAttributes]::ReadOnly)
    }
    $rootItem = Get-Item -LiteralPath $Path -Force
    $rootItem.Attributes = $rootItem.Attributes -band (-bnot [System.IO.FileAttributes]::ReadOnly)
}

if (-not $SkipSdkBuild) {
    $packageArgs = @{
        Configurations = @("Debug", "Release")
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

    Write-Host "[installer] Building SDK staging tree..."
    & $packageScript @packageArgs
    if ($LASTEXITCODE -ne 0) {
        throw "SDK package build failed with exit code $LASTEXITCODE."
    }
}

if (-not (Test-Path -LiteralPath $packagePath)) {
    throw "SDK package directory was not found: $packagePath"
}

if (Test-Path -LiteralPath $installerBuildPath) {
    Write-Host "[installer] Removing previous installer build directory..."
    Clear-ReadOnlyAttribute -Path $installerBuildPath
    Remove-Item -LiteralPath $installerBuildPath -Recurse -Force
}
if (Test-Path -LiteralPath $outputPath) {
    Write-Host "[installer] Removing previous installer output directory..."
    Clear-ReadOnlyAttribute -Path $outputPath
    Remove-Item -LiteralPath $outputPath -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $cpackSourcePath | Out-Null
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

$resolvedPackagePath = (Resolve-Path -LiteralPath $packagePath).Path.Replace("\", "/")
$resolvedOutputPath = (Resolve-Path -LiteralPath $outputPath).Path.Replace("\", "/")
$packageVersion = "0.1.0"
$rootCmake = Get-Content -Path (Join-Path $repoRoot "CMakeLists.txt") -Raw
$versionMatch = [regex]::Match($rootCmake, 'project\s*\(\s*ImWidgetWorkspace\s+VERSION\s+([0-9]+(?:\.[0-9]+){0,3})', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
if ($versionMatch.Success) {
    $packageVersion = $versionMatch.Groups[1].Value
}

if ($CpackGenerators.Count -eq 0) {
    $CpackGenerators = @("ZIP")
    if (Get-Command makensis -ErrorAction SilentlyContinue) {
        $CpackGenerators += "NSIS"
    } else {
        Write-Host "[installer] NSIS was not found on PATH; generating ZIP package only."
    }
}

$generatorList = ($CpackGenerators | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }) -join ";"
if ($generatorList -eq "") {
    throw "No CPack generators were selected."
}

@"
cmake_minimum_required(VERSION 3.24)
project(ImWidgetV4StagedInstaller VERSION $packageVersion LANGUAGES NONE)

install(
    DIRECTORY "$resolvedPackagePath/"
    DESTINATION "."
)

set(CPACK_PACKAGE_NAME "ImWidgetV4")
set(CPACK_PACKAGE_VENDOR "ImWidgetV4")
set(CPACK_PACKAGE_VERSION "`$`{PROJECT_VERSION`}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ImWidgetV4 SDK, editor, and CLI tools")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ImWidgetV4")
set(CPACK_PACKAGE_FILE_NAME "ImWidgetV4-`$`{CPACK_PACKAGE_VERSION`}-windows-msvc")
set(CPACK_PACKAGE_DIRECTORY "$resolvedOutputPath")
set(CPACK_GENERATOR "$generatorList")
set(CPACK_VERBATIM_VARIABLES ON)

if("NSIS" IN_LIST CPACK_GENERATOR)
    set(CPACK_NSIS_DISPLAY_NAME "ImWidgetV4")
    set(CPACK_NSIS_PACKAGE_NAME "ImWidgetV4")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "tools")
    set(CPACK_PACKAGE_EXECUTABLES "ImWidgetEditor" "ImWidget Editor")
    set(CPACK_CREATE_DESKTOP_LINKS "ImWidgetEditor")
endif()

include(CPack)
"@ | Set-Content -Path (Join-Path $cpackSourcePath "CMakeLists.txt") -Encoding utf8

Write-Host "[installer] Configuring CPack staging project..."
& cmake -S $cpackSourcePath -B $cpackBuildPath
if ($LASTEXITCODE -ne 0) {
    throw "CPack staging configure failed with exit code $LASTEXITCODE."
}

Write-Host "[installer] Generating installer package(s): $generatorList"
& cpack --config (Join-Path $cpackBuildPath "CPackConfig.cmake")
if ($LASTEXITCODE -ne 0) {
    throw "CPack failed with exit code $LASTEXITCODE."
}

Write-Host "[installer] Installer package(s) ready: $outputPath"
