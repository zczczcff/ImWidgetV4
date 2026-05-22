# 样式反序列化覆盖失效问题分析与解决计划

**最后更新时间**: 2026-05-22

## 背景

在使用 `ImWidgetV4EditorCLI snapshot export` 导出 `ImWidgetSDKBuilder` 快照时，发现 UI JSON 中已经将多个控件的圆角设置为 `0`，但导出的图像里仍然出现圆角。最初肉眼看起来像是 `ImVerticalSplitter` 上半部分或 `ImScrollBox` 的圆角没有清掉，进一步通过像素采样确认后，问题实际存在于多层控件：

- `ImScrollBox`
- `ImExpandableBox`
- `ImComboBox`

这些控件的 JSON 中都包含显式样式值，例如 `CornerRadius: 0`，但运行时仍可能回退到主题默认样式中的圆角值。

## 现象

典型表现：

- UI 文档中存在显式样式字段。
- 代码生成结果中也包含这些样式字段。
- 控件对象反序列化后，成员样式值已经被写入。
- 但控件绘制时调用 `GetEffectiveStyle()`，仍返回主题解析样式，而不是 JSON 中反序列化得到的样式。

以 `ImScrollBox` 为例：

```json
{
  "ImScrollBox::Style": {
    "Type": "FScrollBoxStyle",
    "Properties": {
      "FScrollBoxStyle::CornerRadius": 0
    }
  }
}
```

如果 `m_bHasExplicitStyle` 没有被置为 `true`，`GetEffectiveStyle()` 会继续使用 `ResolveScrollBoxStyle(application->GetStyleSet())`，最终圆角来自主题默认值，而不是 JSON。

## 根因

当前反射系统写属性时会直接写入成员变量，而不会经过控件的业务 setter。

例如控件手动调用：

```cpp
scrollBox->SetStyle(style);
```

会同时完成两件事：

- 写入 `m_Style`
- 设置 `m_bHasExplicitStyle = true`

但反序列化路径大致是：

```cpp
Reflection::FromJson(object, json)
  -> FPropertyHandle::Write(...)
  -> 直接写入 m_Style
```

这条路径只写入 `m_Style`，不会调用 `SetStyle()`，因此不会触发 setter 中的副作用。

结果就是控件内部状态变成：

```cpp
m_Style.CornerRadius = 0;
m_bHasExplicitStyle = false;
```

此时 `GetEffectiveStyle()` 仍然认为控件没有显式样式覆盖，所以继续使用主题样式。

## 暴露出的架构缺陷

### 1. 反射属性写入绕过业务不变量

当前反射系统把“字段可写”当作“属性已生效”。这在简单 POD 字段上可行，但对带副作用的控件属性并不成立。

常见副作用包括：

- 设置显式覆盖标记，例如 `m_bHasExplicitStyle`
- 刷新本地化缓存，例如 `FText` 相关字段
- 重建子控件或弹窗内容
- 标记布局/绘制脏状态
- 同步运行时缓存，例如选择项、滚动状态、窗口样式

只要反射直接写成员，就可能漏掉这些副作用。

### 2. 样式覆盖状态与样式值分离

很多控件采用如下模式：

```cpp
FSomeWidgetStyle m_Style;
bool m_bHasExplicitStyle = false;
```

这种双变量模式容易不同步。`m_Style` 有值并不代表它会被使用，是否使用取决于另一个布尔值。

这会导致三个问题：

- 反序列化必须额外记得设置布尔值。
- 新控件容易复制旧模式并漏掉 `PostDeserializeFromJson()`。
- 调试时看到 `m_Style` 值正确，但渲染结果仍然错误，定位成本高。

### 3. `PostDeserializeFromJson()` 是补丁点，不是统一机制

当前可以通过重写 `PostDeserializeFromJson()` 修补反序列化后的派生状态，但这依赖每个控件作者主动记得处理。

它的问题是：

- 分散在各个控件中，缺少统一约束。
- 无法表达“某个属性写入时需要触发某个 setter”。
- 只在对象反序列化末尾执行，难以处理属性间顺序依赖。
- 测试覆盖不足时很容易遗漏。

### 4. 主题系统和序列化系统边界不清

控件样式当前同时承担两种语义：

- 主题解析结果
- 用户或文档显式覆盖

但序列化并没有明确记录“这个样式是完整覆盖、部分覆盖，还是没有覆盖”。因此系统只能通过 `m_bHasExplicitStyle` 这样的运行时状态推断意图。

这对编辑器和 CLI 尤其危险，因为 UI 文档是长期存储格式，必须明确表达样式覆盖语义。

## 当前止血方案

已经采用的短期修复方式是：

- 对 `ImScrollBox`、`ImExpandableBox`、`ImComboBox` 增加 `PostDeserializeFromJson()`。
- 在反序列化后将 `m_bHasExplicitStyle` 置为 `true`。
- 对 `ImComboBox` 同时同步本地化 items 和 placeholder。
- 增加回归测试，覆盖 `FromJson -> SetActiveTheme -> GetEffectiveStyle` 路径。

该方案能修复当前问题，但不应视为最终架构方案。

## 计划解决方案

### 阶段 1: 全量排查现有风险

目标是找出所有存在“反射直接写成员 + setter 有副作用”的控件。

建议排查范围：

- 所有包含 `m_bHasExplicitStyle` 的控件。
- 所有 `SetXxx()` 中除了赋值以外还调用 `Invalidate()`、同步缓存、广播事件、重建结构的属性。
- 所有反射注册为 `MakeMemberProperty`，但实际业务上应该走 setter 的属性。

重点文件：

- `ImWidgetV4/include/imwidgetv4/widgets/*.h`
- `ImWidgetV4/src/widgets/*.cpp`
- `ImWidgetV4/src/reflection/ReflectionJson.cpp`

阶段产出：

- 一张控件属性风险表。
- 每个风险属性标注建议迁移方式：accessor、post hook、或重构为状态容器。

### 阶段 2: 样式属性改为 accessor 反射

对样式类属性，优先从成员反射迁移为 accessor 反射。

目标模式：

```cpp
Reflection::MakeObjectAccessorProperty<
    ImScrollBox,
    FScrollBoxStyle,
    &ImScrollBox::SetStyleProperty,
    &ImScrollBox::GetStyleProperty>(...)
```

其中 `SetStyleProperty()` 内部调用正式的 `SetStyle()`，从而保证：

- 显式样式标记被设置。
- 布局/绘制失效被触发。
- 后续新增副作用不会绕过。

阶段原则：

- 不一次性大改所有属性。
- 优先迁移样式属性，因为这次 bug 已经证明风险最高。
- 保持 JSON 格式兼容，不改变已有字段名。

### 阶段 3: 引入统一的样式覆盖容器

中期建议用统一结构替代 `m_Style + m_bHasExplicitStyle`。

可选设计：

```cpp
template<typename TStyle>
class TStyleOverride {
public:
    bool HasValue() const;
    const TStyle& GetValue() const;
    void SetValue(const TStyle& value);
    void Reset();
};
```

或直接使用：

```cpp
std::optional<FScrollBoxStyle> m_StyleOverride;
```

语义变为：

- 没有 override 时走主题。
- 有 override 时使用文档或用户显式样式。
- 不再需要独立布尔值和样式值保持同步。

该阶段可能影响较大，建议在阶段 1、2 后再推进。

### 阶段 4: 明确文档中的样式覆盖语义

当前 UI JSON 中只要出现 `ImXxx::Style`，运行时就应当认为这是显式覆盖。

建议在文档模型中明确约定：

- `Style` 字段缺失：使用主题。
- `Style` 字段存在：使用显式样式覆盖。
- 后续如果要支持部分覆盖，应引入专门格式，例如 `StyleOverrides`，而不是复用完整 `Style`。

这能让编辑器、CLI、代码生成、运行时保持一致。

### 阶段 5: 增强反射系统的生命周期能力

长期建议为反射反序列化提供更明确的生命周期机制。

可考虑：

- 属性写入前后 hook。
- 反序列化事务对象。
- 对象级 `BeginDeserializeFromJson()` / `EndDeserializeFromJson()`。
- 反射属性描述中标记是否必须走 setter。

目标不是增加复杂度，而是避免“直接写字段导致业务状态不同步”变成长期维护负担。

## 测试计划

新增或补齐以下测试类型：

1. 样式反序列化测试

   对所有可主题解析的控件测试：

   ```text
   FromJson(Style = custom)
   SetActiveTheme(...)
   GetEffectiveStyle()
   仍返回 custom
   ```

2. UI 文档快照测试

   使用实际 `ui.json` 导出 PNG，对关键边缘像素做采样，验证圆角、边框、背景颜色是否符合预期。

3. 代码生成测试

   确认生成代码中的 `FromJson(...)` 路径与编辑器运行时路径一致，不出现“编辑器显示正确、生成应用显示错误”的分叉。

4. 主题切换测试

   验证：

   - 无显式样式的控件随主题变化。
   - 有显式样式的控件不被主题覆盖。
   - 重置显式样式后重新跟随主题。

## 风险与注意事项

- 直接把所有反射属性改成 setter 可能触发布局、事件、窗口重建等副作用，需要分批迁移。
- 某些属性在反序列化过程中可能依赖顺序，例如 items 与 selected index。
- 如果未来支持“部分样式覆盖”，需要重新设计 JSON 表达，不能简单把完整 style 当作 patch。
- 不应为了修复样式问题禁用主题系统；主题默认值仍然是无显式覆盖控件的正确来源。

## 推荐执行顺序

1. 先完成风险扫描，列出所有 `m_bHasExplicitStyle` 控件。
2. 为每个样式控件补齐 `FromJson -> theme -> GetEffectiveStyle` 测试。
3. 将样式属性逐步迁移到 accessor 反射。
4. 引入统一 style override 容器，替代双变量模式。
5. 最后再评估是否需要扩展 UI JSON 的样式覆盖格式。

## 结论

本次问题不是单纯的圆角配置错误，而是反射系统、主题系统、控件状态管理三者之间存在边界不清的问题。

短期通过 `PostDeserializeFromJson()` 可以止血，但长期应将“属性写入如何生效”纳入反射系统设计，而不是依赖每个控件手工维护隐藏状态。否则 UI 编辑器、CLI、代码生成和运行时之间仍会持续出现“文档值正确但渲染结果不正确”的问题。
