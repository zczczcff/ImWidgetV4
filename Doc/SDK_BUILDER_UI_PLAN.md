# ImWidgetSDK 构建器 UI 计划

**最后更新时间**: 2026-05-21

## 背景

当前 SDK 已经具备脚本化构建、分架构打包、安装包生成和 CLI 辅助 UI 开发能力。下一步需要用这些能力开发一个真实工具，验证 `ImWidgetV4EditorCLI` 是否足以支撑 agent 协作式 UI 应用开发。

`ImWidgetSDKBuilder` 作为第一个实战项目，目标是提供一个轻量 UI，用于可视化配置、构建和验证 ImWidgetV4 SDK 包。它应当位于仓库根目录，与 `ImWidgetV4`、`ImWidgetV4Editor` 并列，作为第三个独立构建目标。

## 目标

- 提供面向用户的 SDK 构建器 UI，覆盖常用 SDK 打包参数。
- 使用已编译 SDK 中的 `ImWidgetV4EditorCLI` 初始化和维护 UI 文档，真实检验 CLI 能力。
- 构建时直接链接本仓库源码目标，避免开发者必须先拥有完整 SDK。
- 通过快照导出功能辅助 UI 设计和视觉回归验证。
- 为后续更多工具型应用沉淀一套 “CLI 生成 UI + 源码内构建” 的工作流。

## 非目标

- 首版不替代现有 PowerShell 脚本，UI 只作为脚本的可视化编排层。
- 首版不做复杂任务队列和远程构建。
- 首版不引入新的项目格式，优先复用现有 `ui.json`、生成代码和脚本。
- 首版不支持 Android 或非 MSVC 平台的 SDK 构建配置。

## 项目结构

建议新增目录：

```text
ImWidgetSDKBuilder/
  CMakeLists.txt
  ui/
    Main.ui.json
  generated/
    MainView.h
    MainView.cpp
    AppProjectConfig.h
    AppProjectConfig.cpp
  src/
    main.cpp
    SdkBuilderApp.h
    SdkBuilderApp.cpp
    SdkBuildController.h
    SdkBuildController.cpp
    SdkBuildProfile.h
    SdkBuildProfile.cpp
  snapshots/
    Main.png
```

根目录 `CMakeLists.txt` 增加独立选项：

```cmake
option(IMWIDGETV4_BUILD_SDK_BUILDER "Build ImWidget SDK Builder UI" ON)

if(IMWIDGETV4_BUILD_SDK_BUILDER)
    add_subdirectory(ImWidgetSDKBuilder)
endif()
```

## 构建定位

`ImWidgetSDKBuilder` 是仓库内源码构建目标：

- 链接仓库中的 `imwidgetv4_core`、平台层、app host 等目标。
- 不通过 `find_package(ImWidgetV4)` 消费 SDK。
- 生成的 UI 代码由 CLI 维护，但 CMake 工程由仓库源码工程管理。
- 默认目标应尽量轻量，便于用户克隆仓库后优先构建该工具。

建议后续把该工具的构建依赖控制在：

- ImWidgetV4 core
- Win32 + DX11 平台运行层
- app host
- 必要的编辑器共享基础能力

## CLI Dogfood 流程

UI 文档的初始化、增删查改、校验和快照导出应使用 SDK 包内的 CLI，而不是直接使用源码构建目录里的 CLI。

推荐流程：

```powershell
.\scripts\build_sdk.ps1 -SkipSmoke

.\build\package\ImWidgetV4-SDK\tools\ImWidgetV4EditorCLI.exe ui controls list
.\build\package\ImWidgetV4-SDK\tools\ImWidgetV4EditorCLI.exe ui schema dump
.\build\package\ImWidgetV4-SDK\tools\ImWidgetV4EditorCLI.exe ui lint ImWidgetSDKBuilder\ui\Main.ui.json
.\build\package\ImWidgetV4-SDK\tools\ImWidgetV4EditorCLI.exe snapshot export ImWidgetSDKBuilder\ui\Main.ui.json ImWidgetSDKBuilder\snapshots\Main.png --width 1280 --height 720
```

如果 CLI 的项目创建命令生成的是独立 SDK 消费项目，则只使用它生成初始 UI 和 generated 文件；随后手动调整 `ImWidgetSDKBuilder/CMakeLists.txt`，使其接入根工程源码构建。

## 首版 UI 功能

### 构建参数区

- SDK 输出目录，默认 `build/package/ImWidgetV4-SDK`。
- Windows 架构选择：`win64`、`win32`，支持双选。
- 构建配置选择：`Debug`、`Release`，支持双选。
- 包类型选择：SDK staging、ZIP、NSIS。
- 清理旧构建目录开关。
- 构建完成后运行 smoke test 开关。

### 工具链信息区

- CMake 路径和版本。
- Visual Studio generator。
- MSVC toolset。
- 当前仓库路径。
- 当前 SDK 输出路径。

### 操作区

- Configure SDK。
- Build SDK。
- Build Installer。
- Run Smoke Test。
- Open Package Folder。
- Copy Command Line。

### 日志和结果区

- 展示当前运行脚本。
- 展示实时或分段脚本输出。
- 展示最后一次执行状态、耗时和错误摘要。
- 支持清空日志。

## 脚本复用

首版不重新实现 SDK 构建逻辑，而是调用现有脚本：

- `scripts/build_sdk.ps1`
- `scripts/package_sdk.ps1`
- `scripts/package_installer.ps1`
- `scripts/smoke_sdk_package.ps1`

`SdkBuildController` 负责：

- 根据 UI profile 组装 PowerShell 参数。
- 启动子进程并收集输出。
- 将进度、日志、退出码回传给 UI。
- 统一处理工作目录、路径转义和错误消息。

## 数据模型

建议新增 `SdkBuildProfile`：

```cpp
struct FSdkBuildProfile
{
    std::string OutputDirectory;
    bool bBuildWin32 = false;
    bool bBuildWin64 = true;
    bool bBuildDebug = true;
    bool bBuildRelease = true;
    bool bClean = true;
    bool bRunSmokeTest = true;
    bool bBuildZip = true;
    bool bBuildNsis = false;
};
```

后续可以扩展为可保存的 JSON 配置，用于复用构建配置。

## 快照验证

每次较大 UI 调整后执行：

```powershell
.\build\package\ImWidgetV4-SDK\tools\ImWidgetV4EditorCLI.exe snapshot export ImWidgetSDKBuilder\ui\Main.ui.json ImWidgetSDKBuilder\snapshots\Main.png --width 1280 --height 720
```

快照用途：

- 检查布局是否溢出。
- 检查控件层级是否符合预期。
- 为 agent 迭代提供视觉反馈。
- 后续可接入视觉回归测试。

## 分阶段计划

### 阶段 1：脚手架和构建接入

- 新增 `ImWidgetSDKBuilder` 目录。
- 接入根目录 CMake 选项和 `add_subdirectory`。
- 提供最小 `main.cpp`，能启动空窗口。
- 确认目标可单独构建。

验收标准：

- `cmake --build build --config Debug --target ImWidgetSDKBuilder` 成功。
- 不依赖已安装 SDK。

### 阶段 2：使用 SDK CLI 创建 UI

- 使用 SDK 内 `ImWidgetV4EditorCLI` 创建或维护 `Main.ui.json`。
- 使用 `ui lint`、`ui tree`、`ui validate` 检查文档。
- 生成 `MainView` 相关代码。
- 导出首张 `snapshots/Main.png`。

验收标准：

- UI 文档由 CLI 可读、可校验、可快照导出。
- 快照 PNG 存在且非空。

### 阶段 3：构建配置 UI

- 实现输出目录、架构、配置、包类型和常用开关。
- UI 状态映射到 `FSdkBuildProfile`。
- 支持复制等价命令行。

验收标准：

- 用户可在 UI 中表达 `package_sdk.ps1` 常用参数。
- 命令预览与 UI 状态一致。

### 阶段 4：脚本执行和日志

- 实现 `SdkBuildController`。
- 调用 SDK 构建、安装包构建和 smoke test 脚本。
- 展示日志、退出码和耗时。

验收标准：

- 能从 UI 触发 SDK 构建脚本。
- 失败时有明确错误信息。

### 阶段 5：实测反馈 CLI

在开发 `ImWidgetSDKBuilder` 的过程中记录 CLI 短板，优先补齐：

- UI 初始化命令是否足够适配源码内目标。
- generated 代码更新是否可独立触发。
- `ui patch` 是否能高效完成复杂布局编辑。
- `snapshot export` 对资源路径和默认尺寸是否可靠。
- CLI 错误信息是否足够指导 agent 修复。

验收标准：

- 至少形成一轮 CLI 改进清单。
- 必要的 CLI 缺口在同迭代内修复。

## 测试计划

- 构建测试：单独构建 `ImWidgetSDKBuilder` Debug。
- CLI 实测：使用 SDK 包内 CLI 对 `Main.ui.json` 执行 lint、tree、snapshot export。
- 脚本 smoke：通过 UI 或 controller 触发 `build_sdk.ps1 -SkipSmoke`。
- 错误路径：输出目录非法、架构未选择、脚本不存在、构建失败。
- 快照检查：确认 `snapshots/Main.png` 非空，主要布局未溢出。

## 风险与对策

| 风险 | 对策 |
| --- | --- |
| SDK CLI 生成的是独立 SDK 消费项目 | UI 文档和 generated 代码由 CLI 生成，CMake 手动接入源码工程 |
| 构建器依赖过重 | 首版只链接运行 UI 必需目标，避免依赖完整编辑器 |
| 脚本输出阻塞 UI | controller 使用后台进程和增量日志缓冲 |
| UI 快照与实际窗口不一致 | 固定快照尺寸，并保留人工运行验证 |
| CLI 缺少关键编辑能力 | 在开发中记录并优先补齐，形成 dogfood 闭环 |

## 推荐首个实现切片

第一轮只实现“能打开的构建器壳子 + 静态配置 UI + 快照导出”：

1. 新增源码内构建目标。
2. 用 SDK CLI 创建 `Main.ui.json`。
3. 做出参数表单和日志区域的静态 UI。
4. 导出 1280x720 快照。
5. 构建并运行空逻辑应用。

这一切片可以最快验证三件事：根工程接入方式是否合理、CLI 是否能支撑真实 UI 编辑、快照导出是否足以辅助视觉设计。
