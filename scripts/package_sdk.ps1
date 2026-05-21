param(
    [string]$Configuration = "",
    [string[]]$Configurations = @(),
    [string]$BuildDir = "build/package-sdk-build",
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$Generator = "",
    [string]$Platform = "",
    [string[]]$Architectures = @(),
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildDir
$packagePath = Join-Path $repoRoot $PackageDir
$requestedConfigurations = @()
$requestedArchitectures = @()

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

function Set-ReadOnlyFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Paths
    )

    foreach ($path in $Paths) {
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }

        Get-ChildItem -LiteralPath $path -Recurse -File -Force | ForEach-Object {
            $_.Attributes = $_.Attributes -bor [System.IO.FileAttributes]::ReadOnly
        }
    }
}

function Normalize-ArchitectureName {
    param([string]$Name)

    $normalized = $Name.Trim().ToLowerInvariant()
    if ($normalized -eq "x86" -or $normalized -eq "win32") {
        return "win32"
    }
    if ($normalized -eq "x64" -or $normalized -eq "amd64" -or $normalized -eq "win64") {
        return "win64"
    }
    throw "Unsupported SDK architecture '$Name'. Use win32 or win64."
}

function Get-CMakePlatformForArchitecture {
    param([string]$Architecture)

    if ($Architecture -eq "win32") {
        return "Win32"
    }
    if ($Architecture -eq "win64") {
        return "x64"
    }
    throw "Unsupported SDK architecture '$Architecture'."
}

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

if ($Architectures.Count -gt 0) {
    foreach ($entry in $Architectures) {
        foreach ($name in ($entry -split ",")) {
            $trimmedName = $name.Trim()
            if ($trimmedName -ne "") {
                $requestedArchitectures += (Normalize-ArchitectureName $trimmedName)
            }
        }
    }
} elseif (-not [string]::IsNullOrWhiteSpace($Platform)) {
    $requestedArchitectures = @(Normalize-ArchitectureName $Platform)
} else {
    $requestedArchitectures = @("win32", "win64")
}

$normalizedArchitectures = @()
foreach ($name in $requestedArchitectures) {
    if ($normalizedArchitectures -notcontains $name) {
        $normalizedArchitectures += $name
    }
}
$requestedArchitectures = $normalizedArchitectures
Write-Host "[package] Architectures: $($requestedArchitectures -join ', ')"


if (Test-Path $packagePath) {
    Write-Host "[package] Removing previous package directory..."
    Clear-ReadOnlyAttribute -Path $packagePath
    Remove-Item -LiteralPath $packagePath -Recurse -Force
}

$manifestEntries = @()

foreach ($architecture in $requestedArchitectures) {
    $architectureBuildPath = Join-Path $buildPath $architecture
    $cmakePlatform = Get-CMakePlatformForArchitecture $architecture

    $configureArgs = @(
        "-S", $repoRoot,
        "-B", $architectureBuildPath,
        "-DIMWIDGETV4_BUILD_TESTS=OFF",
        "-DIMWIDGETV4_BUILD_SAMPLES=OFF",
        "-DIMWIDGETV4_BUILD_EDITOR=ON",
        "-DIMWIDGETV4_BUILD_CLI=ON",
        "-DIMWIDGETV4_INSTALL_SDK_SUBDIR=sdk",
        "-DIMWIDGETV4_SDK_ARCHITECTURE=$architecture"
    )

    if (-not [string]::IsNullOrWhiteSpace($Generator)) {
        $configureArgs += @("-G", $Generator)
    }
    $configureArgs += @("-A", $cmakePlatform)

    Write-Host "[package] Configuring SDK package build ($architecture)..."
    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed for $architecture with exit code $LASTEXITCODE."
    }

    foreach ($currentConfiguration in $requestedConfigurations) {
        if (-not $SkipBuild) {
            Write-Host "[package] Building package targets ($architecture/$currentConfiguration)..."
            & cmake --build $architectureBuildPath --config $currentConfiguration
            if ($LASTEXITCODE -ne 0) {
                throw "CMake build failed for $architecture/$currentConfiguration with exit code $LASTEXITCODE."
            }
        }

        Write-Host "[package] Installing SDK package ($architecture/$currentConfiguration)..."
        & cmake --install $architectureBuildPath --config $currentConfiguration --prefix $packagePath
        if ($LASTEXITCODE -ne 0) {
            throw "CMake install failed for $architecture/$currentConfiguration with exit code $LASTEXITCODE."
        }
    }

    $metadataPath = Join-Path $packagePath "sdk/cmake/ImWidgetV4SdkMetadata-$architecture.cmake"
    $compilerId = ""
    $compilerVersion = ""
    $msvcToolset = ""
    $msvcVersion = ""
    $generatorPlatform = $cmakePlatform
    if (Test-Path -LiteralPath $metadataPath) {
        foreach ($line in Get-Content -LiteralPath $metadataPath) {
            if ($line -match '^set\(ImWidgetV4_SDK_COMPILER_ID "([^"]*)"\)') { $compilerId = $Matches[1] }
            if ($line -match '^set\(ImWidgetV4_SDK_CXX_COMPILER_VERSION "([^"]*)"\)') { $compilerVersion = $Matches[1] }
            if ($line -match '^set\(ImWidgetV4_SDK_MSVC_TOOLSET "([^"]*)"\)') { $msvcToolset = $Matches[1] }
            if ($line -match '^set\(ImWidgetV4_SDK_MSVC_VERSION "([^"]*)"\)') { $msvcVersion = $Matches[1] }
            if ($line -match '^set\(ImWidgetV4_SDK_GENERATOR_PLATFORM "([^"]*)"\)') { $generatorPlatform = $Matches[1] }
        }
    }

    $manifestEntries += [ordered]@{
        name = $architecture
        configurations = @($requestedConfigurations)
        compiler = [ordered]@{
            id = $compilerId
            version = $compilerVersion
            msvcToolset = $msvcToolset
            msvcVersion = $msvcVersion
        }
        generator = [ordered]@{
            platform = $generatorPlatform
        }
    }
}

$version = "0.1.0"
$versionFile = Join-Path $packagePath "sdk/cmake/ImWidgetV4ConfigVersion.cmake"
if (Test-Path -LiteralPath $versionFile) {
    foreach ($line in Get-Content -LiteralPath $versionFile) {
        if ($line -match 'PACKAGE_VERSION "([^"]+)"') {
            $version = $Matches[1]
            break
        }
    }
}

$manifestPath = Join-Path $packagePath "sdk/ImWidgetV4SdkManifest.json"
$editorToolPath = Join-Path $packagePath "tools/ImWidgetEditor.exe"
$cliToolPath = Join-Path $packagePath "tools/ImWidgetEditorCLI.exe"

if (-not (Test-Path -LiteralPath $editorToolPath)) {
    throw "Expected editor tool was not found under '$editorToolPath'."
}
if (-not (Test-Path -LiteralPath $cliToolPath)) {
    throw "Expected CLI tool was not found under '$cliToolPath'."
}

$manifest = [ordered]@{
    name = "ImWidgetV4"
    version = $version
    architectures = @($manifestEntries)
    tools = [ordered]@{
        editor = "tools/ImWidgetEditor.exe"
        cli = "tools/ImWidgetEditorCLI.exe"
    }
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $manifestPath -Encoding utf8

$readOnlyRoots = @(
    (Join-Path $packagePath "sdk/include"),
    (Join-Path $packagePath "sdk/src"),
    (Join-Path $packagePath "sdk/cmake"),
    $manifestPath
)
Write-Host "[package] Marking SDK headers, sources, and CMake files as read-only..."
Set-ReadOnlyFiles -Paths $readOnlyRoots

Write-Host "[package] Package ready: $packagePath"
