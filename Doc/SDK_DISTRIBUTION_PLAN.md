# ImWidgetV4 SDK 发布与项目供库计划

## Summary

本计划用于规范 ImWidgetV4 及编辑器正式发布后，用户项目应如何获得并链接 ImWidgetV4 库。

当前编辑器生成的新项目通过 `IMWIDGETV4_ROOT` 指向源码目录，并在项目自身构建目录中 `add_subdirectory(...)` 编译 ImWidgetV4。这种方式适合库本体开发和源码调试，但不适合作为正式发布后的默认项目集成方式。

正式发布后推荐将 ImWidgetV4 作为独立 SDK 提供：

- 用户项目默认链接预构建静态库，不重复编译 ImWidgetV4 本体。
- 通过 CMake package 暴露 imported targets，隐藏 include path、第三方依赖和系统库细节。
- 编辑器生成项目默认面向 SDK，而不是源码树。
- 源码集成模式保留为开发/高级选项。

## Goals

- 降低新项目配置和构建时间。
- 避免每个用户项目重复 FetchContent / 编译 ImGui、json、stb 和平台后端。
- 将 ImWidgetV4 的发布物与用户项目源码解耦。
- 让编辑器生成的项目更接近真实应用工程，而不是库源码子工程。
- 为 Windows、Android 以及后续平台后端预留统一分发模型。

## Non-Goals

- 本计划暂不立即修改当前构建系统。
- 本计划不要求马上支持动态库发行。
- 本计划不改变库开发者在源码树内直接构建 samples/tests/editor 的工作流。
- 本计划不解决插件系统、在线包管理、远程 SDK 下载或自动升级。

## Recommended Distribution Model

### Installed SDK

正式发布物建议采用 SDK 目录结构：

```text
ImWidgetV4-SDK/
  include/
    imwidgetv4/
      app/
      core/
      input/
      platform/
      rendering/
      snapshot/
      style/
      widgets/
  lib/
    win32-msvc-x64/
      Debug/
      Release/
    android-arm64-v8a/
      Debug/
      Release/
  cmake/
    ImWidgetV4Config.cmake
    ImWidgetV4ConfigVersion.cmake
    ImWidgetV4Targets.cmake
  tools/
    ImWidgetV4Editor.exe
  templates/
```

其中：

- `include/` 提供公共头文件。
- `lib/` 提供按平台、架构、配置区分的预构建静态库。
- `cmake/` 提供 CMake package。
- `tools/` 可选附带编辑器可执行文件。
- `templates/` 可选存放编辑器创建项目时使用的工程模板。

### CMake Package

用户项目应通过 CMake package 使用 SDK：

```cmake
find_package(ImWidgetV4 CONFIG REQUIRED)

target_link_libraries(MyApp PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_win32_dx11
)
```

对于 Android：

```cmake
find_package(ImWidgetV4 CONFIG REQUIRED)

target_link_libraries(MyApp PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_android_gles3
)
```

package 应负责传递：

- ImWidgetV4 公共 include 目录。
- ImGui / json / stb 等第三方依赖 include 或 imported targets。
- 平台后端依赖库。
- Windows 下的 `d3d11.lib`、`dxgi.lib`、`d3dcompiler.lib`、`Ole32.lib`、`Shell32.lib`。
- Android 下的 `android`、`log`、`EGL`、`GLESv3`。
- 必要的编译定义。

## Editor Integration Model

### Default Behavior

编辑器正式版创建新项目时，默认生成 SDK 模式项目：

```cmake
set(ImWidgetV4_DIR "" CACHE PATH "Path to ImWidgetV4 SDK cmake package")
find_package(ImWidgetV4 CONFIG REQUIRED)
```

如果编辑器与 SDK 一起发布，编辑器可以自动填充默认 SDK 路径：

```text
EditorInstall/
  ImWidgetV4Editor.exe
  sdk/
    include/
    lib/
    cmake/
```

新项目默认使用：

```cmake
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../sdk")
find_package(ImWidgetV4 CONFIG REQUIRED)
```

实际路径应由编辑器项目配置写入，而不是硬编码到用户业务代码中。

### Project Settings

编辑器项目配置中建议增加：

- SDK 路径。
- SDK 版本。
- 库接入模式：`SDK` / `Source`。
- 目标平台。
- 构建配置：Debug / Release。
- 架构：x64 / arm64-v8a 等。

### Generated Project

编辑器生成的新项目应满足：

- 默认不再 `add_subdirectory(${IMWIDGETV4_ROOT})`。
- 默认不再触发 ImWidgetV4 本体编译。
- 只链接 SDK 暴露的 imported targets。
- 用户业务代码不直接依赖 SDK 安装目录细节。
- 代码生成文件仍保持在 `generated/` 下。

## Source Mode

源码模式仍应保留，但作为开发/高级选项：

```cmake
option(IMWIDGETV4_USE_SOURCE "Build ImWidgetV4 from source" OFF)

if(IMWIDGETV4_USE_SOURCE)
    add_subdirectory("${IMWIDGETV4_ROOT}" "${CMAKE_BINARY_DIR}/ImWidgetV4")
else()
    find_package(ImWidgetV4 CONFIG REQUIRED)
endif()
```

源码模式适用于：

- ImWidgetV4 本体开发。
- 用户需要修改库源码。
- 没有可用预构建 SDK 的平台。
- CI 需要从源码完整构建。

正式编辑器创建项目时，默认应使用 SDK 模式。

## Static vs Dynamic Libraries

v1 发布建议以静态库为主：

- 部署简单。
- 用户项目生成和运行路径更稳定。
- 避免 DLL 搜索路径和 ABI 暴露问题。
- 更适合当前 Windows / Android 双平台起步阶段。

动态库可作为后续扩展：

- 插件化运行时。
- 多应用共享同一库二进制。
- 减少最终包体。

但动态库不应阻塞首版 SDK 方案。

## Versioning

SDK 应包含版本文件：

```text
cmake/ImWidgetV4ConfigVersion.cmake
```

编辑器项目可记录：

- 创建项目时使用的 SDK 版本。
- 当前项目期望的最低 SDK 版本。
- 当前已解析到的 SDK 路径。

未来打开项目时，如果 SDK 版本不匹配，编辑器应提示用户升级或切换 SDK。

## Platform Matrix

### Windows Desktop

首版 SDK 重点支持：

- MSVC x64 Debug
- MSVC x64 Release

后续可扩展：

- clang-cl
- MinGW
- x86

### Android

Android SDK 需要按 ABI 区分：

- arm64-v8a
- armeabi-v7a
- x86_64

首版可优先支持 `arm64-v8a`。

Android package 还需要明确：

- NDK 版本。
- minSdkVersion。
- targetSdkVersion。
- 是否需要 native app glue 源文件由用户项目编译。

## Migration Strategy

### Phase 1: Planning Only

- 保留当前源码模式生成项目。
- 完成本计划文档。
- 不改构建逻辑。

### Phase 2: Build-Tree Package

- 为 ImWidgetV4 主构建生成 build-tree CMake package。
- 允许本地开发时通过 `find_package(ImWidgetV4 CONFIG)` 复用当前构建树中的 targets。
- 仍不作为最终发布格式。

### Phase 3: Installable SDK

- 增加 `install(TARGETS ...)`、`install(FILES ...)` 和 `install(EXPORT ...)`。
- 生成 install-tree `ImWidgetV4Config.cmake`。
- 输出标准 SDK 目录。

### Phase 4: Editor Project Template Update

- 编辑器生成项目默认切换到 SDK 模式。
- 项目配置页增加 SDK 路径和库接入模式。
- 保留源码模式作为高级选项。

### Phase 5: Release Packaging

- 打包编辑器与 SDK。
- 增加版本校验。
- 增加示例项目和模板。
- 增加发布验证脚本。

## Risks

- Debug / Release、编译器、架构不匹配会导致链接或运行期问题。
- 静态库 package 必须正确传递第三方依赖和系统库，否则用户项目会链接失败。
- Android ABI 和 NDK 版本管理需要单独约束。
- 直接复用编辑器自身 build 目录里的 `.lib` 不稳定，不应作为正式方案。
- 如果 SDK 与编辑器版本不匹配，生成代码可能调用不存在的 API。

## Recommended Final Direction

最终推荐路线：

1. ImWidgetV4 以 SDK 形式发布。
2. SDK 提供 CMake package 和 imported targets。
3. 编辑器默认使用随包 SDK 创建项目。
4. 用户项目默认链接预构建静态库。
5. 源码模式保留为开发/高级选项。

这个方向可以同时兼顾正式用户体验和库本体开发效率。
