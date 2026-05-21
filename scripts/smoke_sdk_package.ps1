param(
    [string]$PackageDir = "build/package/ImWidgetV4-SDK",
    [string]$Configuration = "Release",
    [string]$SmokeDir = "build/package-sdk-smoke",
    [string]$Generator = "",
    [string]$Platform = "",
    [string]$Architecture = "win64"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$packagePath = Resolve-Path (Join-Path $repoRoot $PackageDir)
$architectureName = $Architecture.Trim().ToLowerInvariant()
if ($architectureName -eq "x86") { $architectureName = "win32" }
if ($architectureName -eq "x64" -or $architectureName -eq "amd64") { $architectureName = "win64" }
$smokePath = Join-Path $repoRoot $SmokeDir
$sourcePath = Join-Path $smokePath "src"
$buildPath = Join-Path $smokePath "build"
$sdkCmakePath = Join-Path $packagePath "sdk/cmake"
$editorPath = Join-Path $packagePath "tools/ImWidgetV4Editor.exe"
$cliPath = Join-Path $packagePath "tools/imwidgetv4.exe"

if ([string]::IsNullOrWhiteSpace($Platform)) {
    if ($architectureName -eq "win32") {
        $Platform = "Win32"
    } else {
        $Platform = "x64"
    }
}

if (-not (Test-Path (Join-Path $sdkCmakePath "ImWidgetV4Config.cmake"))) {
    throw "ImWidgetV4Config.cmake was not found under '$sdkCmakePath'."
}

if (-not (Test-Path (Join-Path $sdkCmakePath "ImWidgetV4Targets-$architectureName.cmake"))) {
    throw "ImWidgetV4Targets-$architectureName.cmake was not found under '$sdkCmakePath'."
}

if (-not (Test-Path $editorPath)) {
    throw "ImWidgetV4Editor.exe was not found under '$editorPath'."
}

if (-not (Test-Path $cliPath)) {
    throw "imwidgetv4.exe was not found under '$cliPath'."
}

Write-Host "[smoke] Checking SDK CLI tool..."
& $cliPath --help | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "SDK CLI help command failed with exit code $LASTEXITCODE."
}

if (Test-Path $smokePath) {
    Write-Host "[smoke] Removing previous smoke directory..."
    Remove-Item -LiteralPath $smokePath -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $sourcePath | Out-Null

@"
cmake_minimum_required(VERSION 3.24)
project(ImWidgetV4PackageSmoke LANGUAGES CXX)

find_package(ImWidgetV4 CONFIG REQUIRED)

add_executable(package_smoke WIN32 main.cpp)
target_link_libraries(package_smoke PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_win32_dx11
    ImWidgetV4::app_host_win32_main
)
"@ | Set-Content -Path (Join-Path $sourcePath "CMakeLists.txt") -Encoding utf8

@"
#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/TextBlock.h>

#include <memory>

class PackageSmokeHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "SDK package smoke";
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        auto widget = std::make_shared<ImWidgetV4::ImTextBlock>();
        widget->SetText("SDK package smoke");
        application.SetRootWidget(widget);
    }
};

std::shared_ptr<ImWidgetV4::IApplicationHostDelegate> ImWidgetV4::CreateApplicationHostDelegate()
{
    return std::make_shared<PackageSmokeHostDelegate>();
}
"@ | Set-Content -Path (Join-Path $sourcePath "main.cpp") -Encoding utf8

$configureArgs = @(
    "-S", $sourcePath,
    "-B", $buildPath,
    "-DImWidgetV4_DIR=$sdkCmakePath"
)

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs += @("-G", $Generator)
}
if (-not [string]::IsNullOrWhiteSpace($Platform)) {
    $configureArgs += @("-A", $Platform)
}

Write-Host "[smoke] Configuring consumer project..."
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "Smoke configure failed with exit code $LASTEXITCODE."
}

Write-Host "[smoke] Building consumer project ($Configuration)..."
& cmake --build $buildPath --config $Configuration --target package_smoke
if ($LASTEXITCODE -ne 0) {
    throw "Smoke build failed with exit code $LASTEXITCODE."
}

Write-Host "[smoke] Package smoke test passed: $packagePath"
