# ImWidgetV4 SDK 发布与项目供库计划

## Summary

本计划用于规范 ImWidgetV4 和编辑器正式发布后，用户项目如何获得并链接 ImWidgetV4。

当前推荐路线是：

- ImWidgetV4 以独立 SDK 形式发布。
- SDK 提供 CMake package 与 imported targets。
- 编辑器生成项目优先面向 SDK，源码模式保留为开发/高级选项。
- 用户业务项目不直接依赖 ImWidgetV4 源码目录结构。

## 目标

- 降低新项目配置和构建时间。
- 避免每个用户项目重复编译 ImWidgetV4、ImGui、json、stb 和平台后端。
- 将 ImWidgetV4 发布物与用户项目源码解耦。
- 为 Windows、Android 和后续平台后端预留统一分发模型。
- 保留 Source 模式，方便库本体开发和调试。

## 非目标

- 首版不要求动态库发布。
- 首版不解决在线包管理、远程 SDK 下载或自动升级。
- 首版不实现插件系统。
- 首版不要求 Android 多 ABI 完整打包。

## 推荐 SDK 目录结构

```text
ImWidgetV4-Release/
  sdk/
    include/
      imwidgetv4/
    lib/
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

说明：

- `sdk/include` 提供公共头文件和必要第三方头文件。
- `sdk/lib/<Config>` 提供预构建静态库。
- `sdk/cmake` 提供 CMake package。
- `tools` 放置编辑器可执行文件。
- 编辑器会优先探测自身目录或上一级目录下的 `sdk/cmake`。

## CMake 使用方式

用户项目通过 CMake package 使用 SDK：

```cmake
find_package(ImWidgetV4 0.1.0 CONFIG REQUIRED)

target_link_libraries(MyApp PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_win32_dx11
    ImWidgetV4::app_host_win32_main
)
```

Android 后续使用类似目标：

```cmake
find_package(ImWidgetV4 0.1.0 CONFIG REQUIRED)

target_link_libraries(MyApp PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_android_gles3
    ImWidgetV4::app_host_android_main
)
```

## 已完成能力

- 已生成 build-tree CMake package。
- 已支持 install-tree CMake package。
- 已安装公共头文件、第三方头文件、静态库和 CMake targets。
- 已导出 host main targets：
  - `ImWidgetV4::app_host_win32_main`
  - `ImWidgetV4::app_host_android_main`
- 编辑器项目配置已支持 `SDK` / `Source` 两种库接入模式。
- 编辑器项目配置已支持 SDK package 路径。
- 编辑器项目配置已支持最低 SDK 版本。
- 生成项目的 SDK 模式会使用带版本约束的 `find_package(ImWidgetV4 <version> CONFIG REQUIRED)`。
- 新建项目会自动探测随编辑器发布的 `sdk/cmake`，存在时默认使用 SDK 模式。
- 打开 SDK 模式项目时会读取 `ImWidgetV4ConfigVersion.cmake` 并在 Output 中提示版本兼容状态。
- 已验证 build-tree package smoke test。
- 已验证 install-tree package smoke test。
- 已验证 `sdk/cmake` 子目录发布布局可被外部最小工程消费。
- 已增加 `scripts/package_sdk.ps1` 一键生成 `ImWidgetV4-Release/` 发布目录。
- 已增加 `scripts/smoke_sdk_package.ps1` 验证发布包可被外部最小工程消费。
- 已在发布包中安装 `templates/MinimalDesktopApp` 最小桌面应用模板。

## Source 模式

Source 模式仍保留：

```cmake
add_subdirectory("${IMWIDGETV4_ROOT}" "${CMAKE_BINARY_DIR}/ImWidgetV4")
```

适用场景：

- ImWidgetV4 本体开发。
- 用户需要修改库源码。
- 某个平台暂时没有可用预构建 SDK。
- CI 需要从源码完整构建。

## 分阶段计划

### Phase 1：规划

状态：已完成。

- 明确 SDK 发布方向。
- 明确 Source 模式作为开发/高级选项保留。

### Phase 2：Build-Tree Package

状态：已完成。

- 主构建树生成 `ImWidgetV4Config.cmake`。
- 外部工程可通过 build-tree package 消费当前构建产物。

### Phase 3：Installable SDK

状态：已完成。

- 增加 `install(TARGETS ...)`、`install(FILES ...)`、`install(EXPORT ...)`。
- 生成 install-tree `ImWidgetV4Config.cmake` 和版本文件。
- 支持 `sdk/` 子目录安装布局。

### Phase 4：编辑器项目模板更新

状态：基本完成。

- 项目配置页支持 SDK 路径、最低 SDK 版本和接入模式。
- 生成项目支持 SDK/Source 条件切换。
- SDK 模式使用 imported targets，不引用库源码目录。

剩余事项：

- 新建项目对 SDK 模式的默认启用策略需要在发布包中做一次端到端验证。
- 项目设置页可增加“选择 SDK package 路径”按钮。
- Output 中的 SDK 版本提示后续可升级为更明显的状态栏/配置页诊断。

### Phase 5：Release Packaging

状态：基本完成。

已完成：

- CMake install 可生成 `sdk/` 和 `tools/` 布局。
- 编辑器可自动识别随包 SDK。
- `scripts/package_sdk.ps1` 可生成包含 `sdk/`、`tools/`、`templates/` 的发布目录。
- `scripts/smoke_sdk_package.ps1` 可从发布目录配置并构建最小外部消费工程。
- 发布包包含 `templates/MinimalDesktopApp`，用于手工验证和用户入门。

待完成：

- 增加 Release/Debug 双配置打包验证。
- 明确 Android ABI、NDK 版本和 native app glue 分发策略。
- 增加发布包压缩、版本命名和校验清单。

## 风险

- Debug / Release、编译器、架构不匹配会导致链接或运行问题。
- 静态库 package 必须正确传递第三方依赖和系统库。
- Android ABI 和 NDK 版本管理需要单独约束。
- SDK 与编辑器版本不匹配时，生成代码可能调用不存在的 API。
- 直接复用编辑器构建目录中的 `.lib` 不应作为正式发布方案。

## 下一步建议

1. 增加 Debug/Release 双配置发布包验证，避免用户在 Release 工程里只拿到 Debug 静态库。
2. 设计 Android SDK 包布局与 ABI 选择规则。
3. 增加发布包压缩、版本命名和校验清单。
4. 将 SDK 兼容状态从 Output 提示升级为配置页/状态栏诊断。
