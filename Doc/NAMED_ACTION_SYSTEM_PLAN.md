# NamedAction 系统开发计划

最后更新：2026-05-18

## 目标

在 `ImWidgetV4/core` 中建立一套面向运行时与编辑器共用的命名动作系统，作为后续“控件委托绑定可视化事件”和“`.ui -> C++` 自动生成绑定代码”的基础设施。

该系统的定位不是通用消息总线，也不是脚本虚拟机，而是：

- 提供稳定、可查询、可复用的命名动作注册中心
- 提供静态绑定、低开销的调用路径
- 支持校验、顺序处理、最终处理、完成通知等多阶段执行模型
- 支持编辑器以结构化方式描述“某个委托触发时执行哪个命名动作”

## 当前状态

目前 `NamedAction` 已经完成第一批 runtime 能力：

- 命名动作注册与执行
- `Validator / SequentialHandler / FinalHandler / CompletionListener`
- 全局 completion listener
- typed invoker
- overload 模式
- 基于 `uectti` 的签名区分
- `borrow stages + final consume` 参数语义

换句话说，运行时内核已经具备了继续向上长出“编辑器配置模型”和“代码生成绑定”的基础。

## 分层定位

建议把 `NamedAction` 的后续开发分成三层：

### 1. Core Runtime 层

负责动作注册、查询、执行、签名、参数语义、统计和错误报告。

### 2. Editor Metadata 层

负责把“某个控件委托下有哪些可绑定动作”表达成编辑器可编辑、可持久化的模型。

### 3. Codegen / Integration 层

负责把编辑器中配置的命名动作绑定生成到 C++，并在运行时正确注册和调用。

## 设计原则

### 命名动作优先，事件总线后置

先把“命名动作”做好，再决定是否需要单独的命名事件/广播总线。编辑器当前的第一需求是“委托触发后执行一个命名动作”，这并不要求先做独立的事件总线。

### 签名严格但执行模型清晰

签名需要能区分 handler 侧的 `const& / & / &&`，但执行模型必须清楚：

- validator / sequential / completion：只读借用或值复制
- final：允许可变引用和右值消费

这样可以避免旧系统那种“多阶段重复转发导致右值提前被吃掉”的问题。

### 编辑器配置是数据，不是代码片段

编辑器里配置的动作绑定应是结构化数据，而不是任意脚本字符串。这样才便于校验、序列化、代码生成和后续重构。

### 生成代码只做胶水，不做业务解释器

目标是生成类型安全、可读、可调试的 C++ 绑定代码，而不是在运行时解释一堆动态配置。

## 现阶段缺口

虽然 runtime 已有基础，但距离编辑器真正可用还有以下明显缺口。

### 1. 缺少动作注册表视图

当前可以“注册并执行”，但还没有针对编辑器/调试友好的只读枚举与查询接口，例如：

- 当前有哪些 action key
- 每个 action key 有哪些 overload
- 每个 overload 的参数签名是什么
- 每个动作下注册了哪些 validator / handler / listener

这会直接影响后续的调试面板、生成校验和诊断。

### 2. 缺少结构化参数描述

当前参数信息主要体现在模板签名里，还没有可直接被编辑器消费的“参数元数据”模型，例如：

- 参数数量
- 参数类型名
- 参数值类别
- 是否允许编辑器提供常量输入
- 是否要求从委托实参透传

编辑器如果要让用户“选择参数来源”和“填写固定值”，这层元数据是必要的。

### 3. 缺少委托绑定配置模型

编辑器还没有一套正式的数据模型来表达：

- 某个控件的某个委托
- 绑定到一个或多个命名动作
- 每个命名动作的参数如何映射

这会是后续 details 面板和序列化的核心。

### 4. 缺少代码生成接入点

还没有把 `NamedAction` 与现有 `.ui -> .h/.cpp` 生成链连起来。要落到产品层，必须明确：

- 用户在哪里注册动作实现
- 生成代码如何拿到 action system
- 生成代码如何在 widget delegate 上绑定执行逻辑

## 分阶段计划

## 阶段 1：补齐 runtime 查询与诊断能力

目标：让 `NamedAction` 从“可执行”变成“可被上层工具理解”。

建议新增：

- `GetRegisteredActionKeys()`
- `GetActionVariants(actionKey)`
- `GetActionSignatureDescription(...)`
- `GetRegisteredHandlerDescriptions(...)`
- 更结构化的 statistics / diagnostics 数据，而不是只返回拼接字符串

建议新增类型：

- `FNamedActionSignatureInfo`
- `FNamedActionHandlerInfo`
- `FNamedActionDescriptor`

验收标准：

- 上层无需解析字符串即可知道动作的 key、参数、handler 分布
- 查询接口不暴露内部可变状态

## 阶段 2：建立编辑器侧委托绑定模型

目标：让编辑器能持久化“控件委托 -> 命名动作”的配置。

建议新增编辑器模型：

- `EditorNamedActionBinding`
- `EditorDelegateBinding`
- `EditorDelegateArgumentBinding`

建议表达的信息：

- `DelegateName`
- `ActionKey`
- `ActionSignature`
- 参数绑定列表
- 参数来源类型

参数来源建议首版支持三类：

- `DelegateArgument`：从委托原始参数透传
- `LiteralValue`：编辑器中填写常量
- `ImplicitContext`：如 `this`、selected widget、document context 之类的隐式上下文

验收标准：

- 一份 `.ui` 或 editor document 状态里能稳定保存这些绑定信息
- 改名、删除动作时有基本校验反馈

## 阶段 3：补齐参数元数据与可编辑类型约束

目标：让编辑器知道“哪些参数类型可以配置，怎么配置”。

首版建议只开放少量稳定类型：

- `bool`
- `int`
- `float`
- `std::string`
- `FStringId` 或等价轻量键类型
- `FVector2`
- `FColor`

不建议首版开放：

- 任意复杂结构体
- 容器类型
- move-only 类型的编辑器常量输入
- 需要对象图引用解析的复杂指针类型

建议新增类型：

- `ENamedActionArgumentValueKind`
- `FNamedActionArgumentSchema`
- `FNamedActionParameterBindingValue`

验收标准：

- 编辑器可基于 schema 自动选择合适的属性编辑控件
- 非法组合可在保存/生成前被校验出来

## 阶段 4：接入 details 面板

目标：在控件 details 里真正看到委托和绑定配置入口。

具体方向：

- 读取控件公开 delegate 列表
- 在 details 中列出每个可绑定 delegate
- 支持“添加命名动作绑定”
- 支持为每个参数选择来源或填写常量

这里的关键前置是：控件委托需要有一层可枚举的“委托描述元数据”。

如果当前反射系统还不能列出 delegate，需要先补一层 editor-only 描述注册。

验收标准：

- 在 details 中可以看到可用委托
- 可以创建至少一个命名动作绑定
- 关闭再打开文档后绑定配置能恢复

## 阶段 5：接入代码生成

目标：让编辑器配置真正变成可运行行为。

建议生成策略：

- 在生成的 `.h` 中声明动作注册/绑定辅助接口
- 在生成的 `.cpp` 中：
  - 注册控件委托 lambda
  - lambda 中调用 `NamedActionSystem.Execute(...)`
  - 必要时做参数映射与常量填充

建议保持：

- 业务动作实现写在用户可维护区域
- 自动生成代码只负责绑定，不写业务逻辑

验收标准：

- 用户在编辑器中为按钮 `OnClicked` 配一个命名动作后
- 生成代码编译通过
- 运行时点击按钮能真正触发该动作

## 阶段 6：增强诊断、调试与回归测试

目标：让这套系统可维护、可排错。

建议补齐：

- 生成前校验：绑定的 action 是否存在
- 参数映射校验：类型、顺序、值类别是否合法
- 运行时日志：找不到动作、执行失败、validator 拒绝等
- editor test / core test / codegen test

重点测试方向：

- 同名动作 overload 选择正确
- `const& / & / &&` 混合参数列表稳定
- final 修改对 completion 的可见性
- 非法 lvalue -> `&&` 绑定会得到稳定错误
- 委托参数到动作参数的映射顺序正确

## 推荐推进顺序

为了尽快让编辑器看到结果，建议顺序如下：

1. `NamedAction` runtime 查询接口
2. 委托绑定配置模型
3. 参数 schema / 可编辑类型白名单
4. details 面板接入
5. `.ui -> .cpp` 绑定代码生成
6. 调试与诊断面板

## 当前已知决策

为了后续避免反复摇摆，先把目前已确定的约束记下来。

- `NamedAction` 以命名动作为中心，不先扩成大而全事件总线
- overload 仍然允许，但匹配基于 canonical 参数类型
- borrow stages 只读/值复制，final 保留消费语义
- `completion listener` 目前仍然在 final 之后执行
- `completion listener` 对消费型 final 看到的是消费后的状态

如果未来产品上认为 completion 需要看到“final 前快照”，那将是一个新的语义设计，不建议在本阶段混入。

## 风险点

- 如果 delegate 元数据没有统一暴露方式，details 面板接入会很散
- 如果允许编辑器任意填写复杂类型参数，校验和生成都会迅速失控
- 如果一开始就试图支持脚本化表达式，会显著增加实现复杂度
- 如果运行时查询接口仍然靠字符串拼接，不利于调试和编辑器复用

## 里程碑定义

### 里程碑 A

runtime 查询接口补齐，上层可枚举动作与签名。

### 里程碑 B

编辑器可持久化 delegate -> named action 绑定模型。

### 里程碑 C

details 面板可编辑绑定。

### 里程碑 D

生成代码后运行时能真实触发配置的动作。

### 里程碑 E

诊断、测试和调试工具补齐，系统进入可长期迭代状态。
