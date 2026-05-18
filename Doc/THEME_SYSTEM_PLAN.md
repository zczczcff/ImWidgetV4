# ImWidgetV4 主题系统开发计划

最后更新：2026-05-18

## 目标

为 `ImWidgetV4` 建立一套可长期演进的正式主题系统，使库本体、编辑器和后续生成应用都能共享一致的主题能力。

主题系统的目标不是简单提供几套颜色表，而是完整支持：

- 应用级主题注册与切换
- 全局设计 token 管理
- token 到控件强类型样式的解析
- 控件实例级样式覆盖
- 编辑器主界面与运行时控件统一换肤
- 后续主题持久化、导入导出和可视化编辑的基础

## 当前现状

库里已经存在主题系统的雏形，但还没有形成真正闭环。

### 已有能力

- `FStyleSet`：全局样式值容器
- `FThemePack`：应用级主题包
- `ImApplication::RegisterThemePack / SetActiveTheme`
- `FPaintContext` 会把 `StyleSet` 往子控件传递
- 默认、Dark、Light 三套内建主题工厂

相关位置：

- [StyleSet.h](/E:/project/ImWidgetV4/ImWidgetV4/include/imwidgetv4/style/StyleSet.h)
- [StyleSet.cpp](/E:/project/ImWidgetV4/ImWidgetV4/src/style/StyleSet.cpp)
- [Application.h](/E:/project/ImWidgetV4/ImWidgetV4/include/imwidgetv4/core/Application.h)
- [Application.cpp](/E:/project/ImWidgetV4/ImWidgetV4/src/core/Application.cpp)

### 当前问题

虽然 `SetActiveTheme()` 已经存在，但绝大多数控件仍直接持有自身完整样式结构，并不真正从全局主题解析，因此主题切换并不能自然影响所有控件。

这意味着：

- 有主题容器
- 有主题切换入口
- 但缺少“主题 token -> 控件样式”的统一解析层

## 问题拆解

当前主题系统缺口主要有五类。

### 1. `FStyleSet` 表达力不足

目前只支持：

- `Color`
- `Float`
- `Vector2`

这足够做最原始的全局参数表，但不足以支撑一个完整主题系统：

- 无层级命名规范
- 无语义 token 约束
- 无别名/继承/fallback 语义
- 无结构化 token 能力

### 2. 控件样式与主题脱节

许多控件直接使用：

- `FButtonStyle`
- `FTabViewStyle`
- `FListViewStyle`
- `FTitleBarStyle`
- `FEditableTextStyle`

这些样式通常通过 `SetStyle(...)` 显式写入控件，而不是在绘制时从主题解析。

结果就是：

- 主题切换不自动生效
- 编辑器里大量颜色还得手工塞
- 后续要支持主题编辑器会非常痛苦

### 3. 缺少统一解析层

当前没有明确的中间层来负责：

- 从 token 解析按钮样式
- 从 token 解析列表样式
- 从 token 解析标题栏样式

没有这一层，主题就无法安全映射到强类型控件样式。

### 4. 缺少主题变更传播机制

现在 `SetActiveTheme()` 主要只是替换 `StyleSet_`。

还缺：

- 主题切换事件
- UI 统一失效重绘
- 主题相关缓存刷新
- 编辑器工具栏/详情区等静态构建区域的同步更新策略

### 5. 编辑器与库本体没有统一主题接入方式

编辑器主界面中仍有大量手工写死颜色和局部样式。这样即使库内部主题系统升级，编辑器也无法自然受益。

## 设计目标

正式主题系统建议分四层。

## 第一层：Theme Token 层

职责：保存全局主题设计值。

这一层不直接关心具体控件，只关心语义化 token。

建议保留并升级 `FStyleSet`，使其承担：

- 全局颜色 token
- 全局尺寸 token
- 间距 token
- 半径 token
- 文本色 token
- 边框色 token

建议 token 命名采用稳定的层级式 key，例如：

- `Color.Surface.Window`
- `Color.Surface.Panel`
- `Color.Surface.Input`
- `Color.Text.Primary`
- `Color.Text.Secondary`
- `Color.Text.Disabled`
- `Color.Border.Default`
- `Color.Border.Focused`
- `Color.Accent.Primary`
- `Float.BorderThickness.Default`
- `Float.CornerRadius.Small`
- `Float.CornerRadius.Medium`
- `Float.Padding.ControlX`
- `Float.Padding.ControlY`

### 建议扩展的数据类型

首版建议在现有基础上增加：

- `Bool`
- `String`
- `Margin`

其中 `Margin` 很适合表达复杂控件内边距，避免把许多间距拆成大量零散 float。

## 第二层：Widget Style Resolver 层

职责：把全局 token 解析成控件真正使用的强类型样式结构。

这是主题系统最关键的一层。

建议新增一组 resolver，例如：

- `ResolveButtonStyle(const FStyleSet&) -> FButtonStyle`
- `ResolveEditableTextStyle(const FStyleSet&) -> FEditableTextStyle`
- `ResolveTabViewStyle(const FStyleSet&) -> FTabViewStyle`
- `ResolveListViewStyle(const FStyleSet&) -> FListViewStyle`
- `ResolveTitleBarStyle(const FStyleSet&) -> FTitleBarStyle`
- `ResolvePopupMenuStyle(const FStyleSet&) -> FPopupMenuStyle`

这层的价值在于：

- 全局 token 仍是通用数据
- 控件最终仍拿到强类型样式
- 主题系统与控件系统解耦
- 编译期类型安全不丢

### 解析层设计原则

- 每类控件只暴露一个明确 resolver
- resolver 内部自己处理 fallback
- resolver 可以消费多个 token
- token 缺失时回退到稳定默认值

## 第三层：Instance Override 层

职责：允许单个控件实例偏离当前主题。

建议固定优先级：

1. 实例显式覆盖
2. 当前主题解析结果
3. 控件内建默认样式

这意味着控件需要从“只有 `SetStyle(...)`”演进为更清晰的三态：

- 未覆盖：完全跟随主题
- 部分覆盖：只覆盖少数字段
- 全覆盖：完全使用实例样式

首版可以先不做字段级 patch 覆盖，而采用较简单的策略：

- 有实例样式就整包覆盖
- 没有实例样式就走主题解析

后续如果需要，再引入更细粒度 override。

## 第四层：Theme Runtime 层

职责：管理应用当前激活主题及其传播。

建议这层由 `ImApplication` 主持，并逐步补齐以下能力：

- 当前主题名/ID
- 主题注册表
- 激活主题切换
- 主题切换通知
- 统一失效与重绘
- 可选主题持久化

建议新增：

- `OnThemeChanged`
- `GetActiveThemePack()`
- `FindThemePack(name)`
- `SetActiveThemeById(...)` 或等价接口

## 主题系统的建议边界

为了避免系统失控，需要先明确哪些事不在首版里做。

### 首版不做

- CSS 式动态选择器系统
- 任意字符串路径自动映射到所有控件字段
- 热更新主题编辑器
- 主题文件导入导出格式标准化
- 字体资源主题化
- 动画曲线和运动系统主题化

### 首版要做好的事

- token 命名规范
- resolver 分层
- 应用级主题切换
- 核心控件接入
- 编辑器主界面接入

## 推荐控件接入顺序

不建议一次性把所有控件都接主题系统。推荐分批推进。

### 第一批

- `ImButton`
- `ImEditableText`
- `ImCheckBox`
- `ImComboBox`
- `ImPopupMenu`
- `ImTitleBar`

原因：这些控件几乎覆盖编辑器顶部工具区和常规表单区。

### 第二批

- `ImTabView`
- `ImListView`
- `ImOutlineView`
- `ImTextOutlineView`
- `ImScrollBox`
- `ImSwitch`

原因：这些控件决定编辑器主体观感。

### 第三批

- `ImDesignerSurface`
- `ImColorPicker`
- `ImSlider`
- `ImExpandableBox`
- 其他剩余复杂控件

原因：这些控件交互更复杂，适合在前两批跑通后再接。

## 编辑器接入计划

主题系统的一个重要目标，是让编辑器不再手工维护大量颜色。

建议编辑器分三步接入。

### 1. 去除硬编码颜色

梳理 [ImWidgetV4Editor/src/main.cpp](/E:/project/ImWidgetV4/ImWidgetV4Editor/src/main.cpp) 中直接写死的：

- 标题栏按钮颜色
- 标题栏文字色
- 状态文本颜色
- 构建状态区域颜色
- 各类工具按钮的 hover / pressed 色

### 2. 改为消费主题 token 或 resolver

例如：

- 标题栏按钮走 `ResolveTitleBarStyle`
- 普通图标按钮走 `ResolveButtonStyle`
- 详情区输入框走 `ResolveEditableTextStyle`

### 3. 提供编辑器级主题切换入口

后续可以在编辑器菜单或设置里增加：

- `Theme -> Default`
- `Theme -> Dark`
- `Theme -> Light`

再后续可以扩展用户自定义主题。

## `FStyleSet` 演进建议

为了后续稳定演进，建议不要直接把 `FStyleSet` 变成一个完全动态的 `std::variant<anything>` 容器，而是逐步增加有限类型支持。

建议顺序：

1. 保持 `Color / Float / Vector2`
2. 增加 `Bool`
3. 增加 `Margin`
4. 必要时再加 `String`

这样既够用，也不至于把所有类型都塞进一套过度动态的数据容器里。

## 分阶段里程碑

### 里程碑 1：主题模型定稿

目标：把主题系统的结构和边界先定下来。

交付物：

- token 命名规范
- `FStyleSet` 扩展方案
- resolver 层目录与接口草案
- 实例覆盖优先级规则

验收标准：

- 不同控件接主题时不会各写各的逻辑

### 里程碑 2：`FStyleSet` 升级

目标：让 token 容器达到主题系统可用程度。

交付物：

- 新 token 类型支持
- 更完整的查询接口
- 更稳定的 key 组织方式

验收标准：

- token 能表达第一批核心控件所需的大部分主题值

### 里程碑 3：resolver 层落地

目标：建立主题与控件样式之间的正式桥梁。

交付物：

- 若干 `ResolveXxxStyle(...)`
- resolver 单测
- resolver fallback 规则

验收标准：

- 第一批控件能完全由主题解析样式

### 里程碑 4：核心控件接入

目标：让主题切换真正影响 UI。

交付物：

- Button / EditableText / ComboBox / TitleBar / PopupMenu 接入
- 实例样式覆盖与主题样式并存

验收标准：

- 切换 `Default / Dark / Light` 能看到明确视觉变化

### 里程碑 5：应用主题切换传播

目标：主题切换成为一个稳定运行时能力。

交付物：

- 主题切换事件
- UI 统一失效与重绘
- 编辑器标题栏和主界面同步更新

验收标准：

- 切换主题后，无需手工重建主要界面

### 里程碑 6：编辑器主界面接入

目标：让编辑器主界面正式使用库主题系统。

交付物：

- 去除一批主界面硬编码颜色
- 编辑器顶部、详情、列表、树视图统一接入

验收标准：

- 编辑器主界面视觉主要由主题驱动，不再靠 scattered hardcode

### 里程碑 7：调试与扩展

目标：让主题系统进入可长期维护状态。

交付物：

- 主题调试视图
- token dump / inspector
- 更完整的测试覆盖

验收标准：

- 可以快速定位某个控件样式来自默认值、主题还是实例覆盖

## 测试建议

主题系统测试建议覆盖三类。

### 1. `FStyleSet` 层

- key 设置、查询、删除
- fallback 正确
- 新 token 类型序列化/复制/移动稳定

### 2. resolver 层

- 缺 token 时回退到默认值
- 完整 token 输入时输出稳定样式
- Dark / Light 主题解析结果明显不同

### 3. 应用与控件层

- `SetActiveTheme()` 后控件绘制变化
- 实例 `SetStyle(...)` 能覆盖主题
- 编辑器主界面切换主题无回归

## 关键风险

- 如果跳过 resolver 直接让控件自己从 `StyleSet` 读 token，后面会迅速变散
- 如果一开始就追求字段级 patch 覆盖，复杂度会飙升
- 如果编辑器继续大量写死颜色，主题系统价值会被抵消
- 如果 token 命名没有统一规范，后续会难以维护

## 推荐下一步

建议立刻从这三步开始：

1. 先写一版 token 命名规范
2. 建立 resolver 层雏形
3. 选 `Button + EditableText + TitleBar` 做第一批接入样板

这是最容易尽快看到成效、又不容易返工的推进路径。
