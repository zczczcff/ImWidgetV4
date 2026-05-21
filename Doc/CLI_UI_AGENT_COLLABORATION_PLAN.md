# CLI UI Agent 协作能力规划

**最后更新时间**: 2026-05-21

## 背景

当前 `ImWidgetEditorCLI` 已具备项目、构建、代码生成、快照导出和环境探测能力，但这些能力主要围绕工程生命周期，不足以支撑 agent 对 `ui.json` 的结构化协作编辑。

为了让 agent 能够稳定参与 UI 应用开发，需要把 CLI 从“项目工具”扩展为“UI 文档操作层”，使其能够完成查询、编辑、验证、导出、批处理等闭环操作。

## 目标

- 让 agent 能发现可用控件及其属性约束。
- 让 agent 能对单个 `ui.json` 做增删查改。
- 让 agent 能对修改结果进行验证、格式化和快照导出。
- 让 CLI 输出机器可读结果，便于多轮自动化协作。

## 现有基础

现有代码已经具备一部分基础能力：

- `WidgetFactory` 可列出已注册控件类型。
- `WidgetSerializer` 可序列化和反序列化控件树。
- `EditorDocument` 可加载、保存和导入文档 JSON。
- `DocumentSnapshotExporter` 可对单个 `ui.json` 做 PNG 快照导出。

但当前仍缺少：

- 控件元数据层。
- 面向节点路径的 CRUD API。
- 结构化查询输出。
- 事务式批量修改能力。

## 需要补强的 CLI 能力

### 1. 控件发现

用于让 agent 先理解可用控件空间。

建议命令：

- `ui controls list`
- `ui controls describe <type>`
- `ui schema dump`

能力要求：

- 列出全部控件类型。
- 输出每个控件的属性、默认值、子节点规则、嵌套限制。
- 输出 JSON 结构，供 agent 直接读取。

### 2. 文档加载与验证

用于让 agent 在修改前后确认文档是否合法。

建议命令：

- `ui validate <file>`
- `ui format <file>`
- `ui tree <file>`

能力要求：

- 校验 JSON 格式和反序列化结果。
- 规范化字段顺序和空字段。
- 输出树结构，支持 JSON 和文本两种格式。

### 3. 查询能力

用于让 agent 定位目标节点或属性。

建议命令：

- `ui get <file> <path>`
- `ui find <file> --type <type> --name <name> --id <id>`
- `ui inspect <file> <path>`

能力要求：

- 按稳定路径定位节点。
- 按类型、名称、ID 过滤节点。
- 输出节点完整属性和子节点信息。

### 4. 编辑能力

用于让 agent 实际修改 UI 文档。

建议命令：

- `ui add <file> <parent-path> <widget-type>`
- `ui remove <file> <path>`
- `ui move <file> <path> <new-parent-path>`
- `ui set <file> <path> <property> <value>`
- `ui rename <file> <path> <name>`
- `ui duplicate <file> <path>`

能力要求：

- 支持单步修改。
- 具备清晰错误信息。
- 修改后自动保存或显式保存。
- 保留节点 ID，避免多轮协作时路径失效。

### 5. 批处理能力

用于让 agent 一次执行多步编辑，降低中间态污染。

建议命令：

- `ui patch <file> <patch.json>`
- `ui batch <script.json>`
- `ui transaction begin|commit|rollback`

能力要求：

- 支持多步原子修改。
- 失败时可回滚。
- 适合 agent 输出操作计划后一次执行。

### 6. 预览与回归验证

用于让 agent 快速确认视觉结果。

建议命令：

- `snapshot export <input.ui.json> <output.png>`
- `ui diff <before> <after>`
- `ui lint <file>`

能力要求：

- 导出 PNG 快照。
- 对比两个文档结构差异。
- 检查非法属性、缺失节点、类型不匹配等问题。

### 7. 资源解析

用于处理 UI 中的相对资源引用。

建议命令：

- `ui resolve-paths <file>`
- `ui assets list <file>`
- `ui assets validate <file>`

能力要求：

- 按 `ui.json` 所在目录解析相对路径。
- 输出依赖资源清单。
- 检查资源是否存在。

## 推荐的数据层能力

为了让 CLI 真正可用，建议增加一层文档操作内核，而不是把逻辑直接写在命令入口里。

建议抽象：

- `WidgetSchemaRegistry`
- `EditorDocumentQuery`
- `EditorDocumentMutation`
- `EditorDocumentPatch`
- `EditorDocumentValidator`

这层能力应提供：

- 节点路径解析。
- 节点属性读写。
- 节点增删改移动。
- 文档结构校验。
- 序列化回写。

## 输出规范建议

为了适配 agent 协作，CLI 输出最好遵循以下规则：

- 默认 human readable。
- 提供 `--format json`。
- 错误信息要明确到命令、路径和字段。
- 成功时返回稳定的结构化结果。

建议统一返回字段：

- `success`
- `error`
- `file`
- `node`
- `path`
- `changed`
- `warnings`

## 推荐实施顺序

### 第一阶段

- `ui controls list`
- `ui controls describe`
- `ui tree`
- `ui validate`

### 第二阶段

- `ui get`
- `ui find`
- `ui set`
- `ui add`
- `ui remove`

### 第三阶段

- `ui move`
- `ui rename`
- `ui duplicate`
- `ui patch`

### 第四阶段

- `ui batch`
- `ui transaction`
- `ui diff`
- `ui assets validate`

## 预期效果

完成后，agent 可以形成如下闭环：

1. 查询可用控件和属性。
2. 打开目标 `ui.json` 并定位节点。
3. 执行增删查改。
4. 校验文档是否合法。
5. 导出快照确认视觉结果。
6. 根据差异继续迭代。

这会把 UI 开发从“手工编辑文件”提升为“可脚本化、可回放、可验证”的协作流程。
