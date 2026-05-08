# ImWidgetV4 编辑器跨平台应用计划

最后更新：2026-05-08

## 目标

将编辑器从“UI 设计器”继续推进为“可直接产出跨平台 GUI 应用工程”的作者工具，支持：

- 在编辑器中一键新建跨平台 GUI 项目
- 生成项目目录与初始源码骨架
- 生成基于 CMake 的工程文件
- 配置平台工具链与交叉编译环境
- 对支持的平台执行一键配置与一键构建

编辑器仍应作为 `ImWidgetV4` 运行库之上的一层编排工具存在。运行时控件、平台后端和可复用的构建基础设施继续放在库本体或工作区中；项目生成、模板、工程配置与构建调度放在 `ImWidgetV4Editor` 中。

## 产品方向

下一阶段的编辑器不再只是“编辑一个 `.ui` 文件”，而是要同时承担：

1. UI 设计
2. 应用工程创建
3. 平台配置
4. 代码与构建文件生成
5. 本地与跨平台构建执行

这意味着编辑器需要正式补上项目系统、模板系统、生成系统和构建编排系统。

## 目标用户流程

第一条完整可用链路应当是：

1. 点击 `Project -> New App Project...`
2. 选择项目根目录、项目名称、模板、命名空间和目标平台
3. 编辑器自动生成：
   - 项目目录结构
   - 初始 C++ 源码
   - `.ui.json` 资源
   - 顶层和分目标的 `CMakeLists.txt`
   - 可选的 preset / toolchain / config 文件
4. 用户继续在编辑器中编辑 UI 与项目配置
5. 用户点击 `Build -> Build Desktop` 或 `Build -> Build Android`
6. 编辑器按当前配置准备环境、执行构建，并把日志/结果回显到编辑器里

## 范围

### 本阶段纳入范围

- 应用工程创建向导
- 工作区/项目元数据模型
- 工程模板生成
- CMake 生成
- 目标平台选择
- 环境检测与配置辅助
- 一键 configure / build
- 输出、日志与错误展示

### 本阶段暂不纳入

- 完整 IDE 替代能力
- 远程构建农场
- iOS / macOS 签名自动化
- 可视化 C++ 类设计器
- 插件市场
- 云端模板仓库

## 设计原则

### 项目数据显式化

项目元信息应由编辑器维护的稳定 manifest 保存，而不是散落在若干构建文件中被动推导。

### 生成结果确定化

项目模板和生成文件必须可重复生成，便于版本控制、diff 和回归比对。

### 生成文件与手写文件分离

编辑器需要明确区分：

- 用户手写代码
- 自动生成的胶水代码
- 自动生成的构建文件
- 自动生成的 UI 绑定代码

这样后续再生成时才不会轻易破坏用户改动。

### 平台能力尽量数据化

平台配置应尽量以描述数据表达，而不是大量硬编码在业务逻辑中，方便后续继续补其他后端。

## 系统总览

这一阶段至少需要补齐五个子系统。

### 1. 项目系统

目的：
让编辑器表达“一个真实应用工程”，而不是只表达“一个打开中的 UI 文档”。

核心职责：

- 当前项目根目录
- 项目名、命名空间
- 应用目标列表
- 启动 UI 文档
- 输出/构建目录
- 各平台构建配置

建议类型：

- `ImWidgetV4Editor/src/project/EditorProject.h/.cpp`
- `ImWidgetV4Editor/src/project/ProjectManifest.h/.cpp`
- `ImWidgetV4Editor/src/project/ProjectPaths.h/.cpp`

建议持久化文件：

- `imwidgetv4.project.json`
- `CMakePresets.json`
- 自动生成的 `CMakeLists.txt`

### 2. 模板系统

目的：
生成可直接启动的应用骨架和初始项目内容。

模板职责：

- 默认目录布局
- 初始运行时入口
- 初始文档加载路径
- `main.cpp` / `AndroidMain.cpp` / 各平台引导代码
- 可选示例 UI 文档

建议类型：

- `ImWidgetV4Editor/src/templates/ProjectTemplateRegistry.h/.cpp`
- `ImWidgetV4Editor/src/templates/ProjectScaffolder.h/.cpp`
- `ImWidgetV4Editor/src/templates/TextTemplateEngine.h/.cpp`

初始模板建议：

- Blank App
- Single Window App
- Multi-Document Tool App
- Mobile Demo App

### 3. 构建生成系统

目的：
根据项目元数据自动生成可构建工程。

生成职责：

- 根 `CMakeLists.txt`
- 应用目标 `CMakeLists.txt`
- 后端与运行库链接关系
- 可选 `CMakePresets.json`
- toolchain 引用
- 资源复制 / 安装规则

建议类型：

- `ImWidgetV4Editor/src/build/CMakeProjectGenerator.h/.cpp`
- `ImWidgetV4Editor/src/build/CMakePresetGenerator.h/.cpp`
- `ImWidgetV4Editor/src/build/BuildTargetModel.h/.cpp`

规则：

- 未经确认不覆盖用户手写文件
- 生成文件应包含稳定标记/说明
- 再生成过程应幂等且增量化

### 4. 工具链 / 环境配置系统

目的：
把跨平台构建变成编辑器内可见、可诊断、可配置的能力。

职责：

- 检测已安装编译器、SDK、NDK、工具链
- 校验关键环境项
- 保存平台配置
- 给出可执行的修复提示

建议类型：

- `ImWidgetV4Editor/src/toolchains/ToolchainRegistry.h/.cpp`
- `ImWidgetV4Editor/src/toolchains/EnvironmentProbe.h/.cpp`
- `ImWidgetV4Editor/src/toolchains/PlatformConfiguration.h/.cpp`

第一批目标矩阵：

- Windows Desktop
- Android

后续预留：

- Linux Desktop
- macOS
- iOS
- WebAssembly

### 5. 构建编排系统

目的：
从编辑器内发起 configure/build，并把结果组织成可消费信息。

职责：

- 配置 build 目录
- 调用 `cmake -S/-B`
- 调用 `cmake --build`
- 选择 preset / config / 平台
- 捕获 stdout / stderr
- 将输出回流到底部日志区
- 记录最近一次构建状态

建议类型：

- `ImWidgetV4Editor/src/build/BuildController.h/.cpp`
- `ImWidgetV4Editor/src/build/BuildJob.h/.cpp`
- `ImWidgetV4Editor/src/build/BuildOutputParser.h/.cpp`

## 分阶段里程碑

### 里程碑 1：项目 Manifest 与工作区模型

目的：
先让编辑器拥有稳定的“应用工程身份”。

交付物：

- `EditorProject`
- `imwidgetv4.project.json` 的加载/保存
- 项目级元数据模型
- 项目根目录到文档的映射关系

要求：

- 一个工程可包含多个 UI 文档
- 一个工程至少可声明一个应用目标
- 活动 UI 文档可以标记为启动文档

### 里程碑 2：新建项目向导

目的：
提供真实可用的“创建应用工程”入口。

交付物：

- `New App Project` 动作
- 项目创建向导/对话流
- 名称/路径/命名空间校验
- 平台目标选择
- 模板选择

向导字段建议：

- 项目名称
- 根目录
- 命名空间
- 应用模板
- 目标平台
- 启动文档名称

### 里程碑 3：目录骨架生成

目的：
先把工程目录和初始文件创建起来。

交付物：

- 项目目录创建器
- 初始源码目录
- 初始 UI / 资源目录
- 初始 `.ui.json`
- 初始 manifest

建议目录：

- `src/`
- `include/`
- `ui/`
- `generated/`
- `cmake/`
- `build/` 或明确要求 out-of-source build

### 里程碑 4：CMake 生成

目的：
让新建项目能立刻成为“可配置、可编译”的工程。

交付物：

- 根 `CMakeLists.txt`
- 应用目标 `CMakeLists.txt`
- 可选 presets
- 运行库链接配置

要求：

- 先支持仅桌面目标的工程生成
- 再扩展到包含 Android 的多目标工程
- 再生成时不能破坏用户代码布局

### 里程碑 5：应用运行时引导代码生成

目的：
生成应用可直接启动所需的最小引导代码。

交付物：

- 启动入口源码
- UI 文档加载/绑定代码
- 生成代码与运行时整合钩子
- 应用标题、图标、默认窗口配置

要求：

- 生成后的工程无需手工修改即可编译
- 生成代码应能消费编辑器输出的 `.ui` 或后续生成的 C++ UI 代码

### 里程碑 6：工具链检测与配置

目的：
将“本机是否能构建目标平台”这件事显式化。

交付物：

- 环境探测服务
- 工具链状态面板
- 平台配置持久化
- 失败提示与修复建议

桌面侧至少检查：

- CMake
- generator 可用性
- 编译器可用性

Android 侧至少检查：

- Android SDK
- NDK
- Java / Gradle
- ABI / SDK 版本设置

### 里程碑 7：一键 Configure / Build

目的：
把构建真正接到编辑器里。

交付物：

- `Build` 菜单或标题栏构建动作
- 配置当前工程
- 构建当前目标
- 构建日志输出
- 成功/失败摘要

要求：

- 构建日志实时进入输出区
- 调用命令可见且可复现
- build 目录选择明确

### 里程碑 8：多目标构建 Profile

目的：
支持一个工程对应多套输出目标。

交付物：

- Build Profile 模型
- Debug / Release 选择
- 分平台输出目录
- 活动构建配置切换器

初始 Profile：

- Windows Debug
- Windows Release
- Android Debug

### 里程碑 9：安全再生成

目的：
允许用户在初次生成之后继续维护工程，而不是“一次性脚手架”。

交付物：

- 仅重生成可生成文件
- 覆盖冲突检测
- 用户代码保护区
- 覆盖前预览差异

要求：

- 不允许静默覆盖用户改动
- 生成文件中明确说明再生成策略

### 里程碑 10：构建体验打磨

目的：
让这套链路可以被日常使用。

交付物：

- 最近项目列表
- 打开项目目录 / build 目录
- 构建历史
- 最近失败步骤摘要
- 快速跳转启动文档与目标配置

## 编辑器界面需要补充的内容

### Project 菜单

建议新增：

- New App Project...
- Open Project...
- Regenerate Project Files
- Project Settings
- Configure Toolchains

### Build 菜单

建议新增：

- Configure
- Build Active Target
- Rebuild Active Target
- Clean Active Target
- Select Build Profile

### Project 面板

需要从“文件树”升级为“项目感知树”，至少包含：

- project manifest
- UI 文档
- generated 文件
- 源码目录
- 平台 / target 节点

### Details / Settings 面板

需要新增针对以下对象的编辑界面：

- 项目元数据
- 应用元数据
- target 元数据
- 工具链配置

## 数据模型建议

### Project Manifest

建议顶层分区：

- `Project`
- `Documents`
- `Targets`
- `BuildProfiles`
- `Toolchains`
- `Generation`

### Target 模型

每个 target 至少包含：

- 目标名称
- 平台类型
- backend / runtime 选择
- 启动 UI 文档
- 生成源码位置

### Build Profile 模型

每个 profile 至少包含：

- profile 名称
- 对应 target
- build configuration
- generator / preset
- build 目录

## 推荐实施顺序

按这个顺序推进：

1. 项目 manifest 模型
2. 新建项目向导
3. 目录骨架生成
4. CMake 生成
5. 运行时引导代码生成
6. 工具链 / 环境探测
7. 一键 configure / build
8. 多目标 profile
9. 安全再生成
10. 体验打磨

## 第一版可用标准

第一版可用的“应用作者工具”至少要做到：

- 在编辑器中创建一个新应用工程
- 选择至少 Desktop 和 Android 目标
- 生成项目目录与初始文件
- 保存/加载项目 manifest
- 生成构建文件
- 检测本地构建环境
- 一键 configure
- 一键 build
- 在编辑器中展示结构化构建日志

## 当前最优先的下一步

最值得先做的是：

1. 引入 `EditorProject` 与 `imwidgetv4.project.json`
2. 做出 `New App Project` 向导
3. 先打通“目录 + manifest”的初始工程生成

这样编辑器就先拥有了稳定的“项目层”，后面再往上叠 CMake 生成和工具链自动化会顺很多。
