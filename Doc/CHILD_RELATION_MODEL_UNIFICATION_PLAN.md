# Child Relation Model Unification Plan

## Summary

SDKBuilder title bar currently misses its visible title because `TitleBar.ui.json` stores the title text under the generic `Children` array, while `ImTitleBar` is serialized, deserialized, and code-generated through the special `LeadingItems` / `TrailingItems` arrays.

This is not only a local data bug. It exposes a broader design issue: editor documents, CLI commands, serializers, and code generation do not share a single model for child-widget relationships. Each widget with semantic child regions introduces a new bespoke JSON shape and a new code path.

The recommended direction is to introduce a unified child relation model at the editor/document layer first, then gradually move serialization, CLI mutation, tree views, validation, and code generation onto that model.

## Current State

Runtime widgets still mostly share the same physical widget tree:

- `ImWidget` owns `m_Children`.
- `ImTitleBar::AddLeadingItem()` and `AddTrailingItem()` attach children through `ImWidget::AddChild()`.
- `ImButton::SetContent()`, `ImScrollBox::SetContent()`, `ImTabView::AddTab()`, and similar APIs also register their visual child widgets into the runtime tree.

However, editor-facing relationship data is split across several special formats:

- Generic containers use `Children`.
- Single-content widgets use `Content`.
- `ImTabView` uses `TabItems[].Content` with per-tab metadata.
- `ImTitleBar` uses `LeadingItems` and `TrailingItems`.
- `ImExpandableBox` uses header/body relations.

The code generator mirrors these special cases through `EGeneratedChildRelation`, with values such as:

- `GenericChild`
- `ButtonContent`
- `ScrollContent`
- `ExpandableHeader`
- `ExpandableBody`
- `TabContent`
- `TitleBarLeadingItem`
- `TitleBarTrailingItem`

This means every new semantic child region requires edits in multiple places:

- document serialization
- document deserialization
- code generation
- CLI add/remove/move commands
- document tree display
- designer drag/drop rules
- lint and diagnostics
- template/scaffold generation

## Problem Exposed By SDKBuilder

`ImWidgetSDKBuilder/ui/TitleBar.ui.json` contains a title text widget inside `Children`.

For an `ImTitleBar`, that field is ignored by the special title-bar serializer/deserializer path. The generated `TitleBarView.cpp` therefore only creates the root title bar and does not recreate the title text.

The correct current-format shape is to store that text widget in `LeadingItems`.

This bug was easy to create because `Children` looks like a valid universal child field, but it is not valid for every widget. The system does not currently provide a central schema that can reject or repair this mismatch.

## Why This Hurts Extensibility

The current approach makes each semantic container a toolchain-wide feature instead of a widget-local capability.

Adding future controls such as toolbars, menu bars, status bars, dock layouts, property grids, breadcrumb bars, or complex inspector rows would likely keep expanding the same special-case list. That creates several risks:

- New controls become expensive to integrate into the editor and CLI.
- Some tools may support a relation while others silently ignore it.
- UI JSON can become structurally valid but semantically wrong.
- Agent-driven CLI workflows become harder because commands need widget-specific knowledge.
- Generated code may diverge from runtime behavior when a special case is missed.
- Refactors become risky because relationship semantics are duplicated across unrelated modules.

## Design Goal

Represent child-widget relationships through one editor/document model:

- A child relation has a role name.
- A role declares whether it accepts one child or many children.
- A role can carry metadata, such as tab title, icon, slot padding, alignment, or splitter weight.
- The parent widget type declares which roles it supports.
- Tools attach, remove, move, serialize, and generate code through role metadata instead of hard-coded widget checks.

The runtime widget API can remain unchanged initially. The unification should start in the editor/document layer to reduce migration risk.

## Proposed Document Model

One possible shape:

```json
{
  "Type": "ImTitleBar",
  "Properties": {},
  "ChildRelations": [
    {
      "Role": "LeadingItems",
      "Kind": "Collection",
      "Widget": {
        "Type": "ImTextBlock",
        "Properties": {
          "ImTextBlock::Text": "ImWidget SDK Builder"
        }
      }
    },
    {
      "Role": "TrailingItems",
      "Kind": "Collection",
      "Widget": {
        "Type": "ImButton",
        "Properties": {}
      }
    }
  ]
}
```

Another possible shape keeps relation groups:

```json
{
  "Type": "ImTitleBar",
  "Properties": {},
  "ChildSlots": {
    "LeadingItems": [
      {
        "Type": "ImTextBlock",
        "Properties": {}
      }
    ],
    "TrailingItems": []
  }
}
```

The exact JSON shape can be chosen later. The important part is that every non-property child relationship is expressed as a named role rather than as unrelated bespoke top-level fields.

## Proposed Registry

Introduce an editor-side child relation registry, for example `WidgetChildSchema`.

Each widget type can declare:

- supported child roles
- display name for each role
- whether the role is single-child or multi-child
- whether the role is the default insertion target
- optional metadata schema
- attach function for runtime document editing
- remove function
- code-generation function
- legacy serialization aliases

Example conceptual declaration:

```cpp
RegisterWidgetChildSchema("ImTitleBar")
    .AddCollectionRole("LeadingItems")
    .SetDefaultRole("LeadingItems")
    .SetAttach([](ImWidget& Parent, Ptr Child) {
        static_cast<ImTitleBar&>(Parent).AddLeadingItem(Child);
    })
    .AddCollectionRole("TrailingItems")
    .SetAttach([](ImWidget& Parent, Ptr Child) {
        static_cast<ImTitleBar&>(Parent).AddTrailingItem(Child);
    });
```

For current widgets:

- `ImVerticalBox` / `ImHorizontalBox` / `ImCanvasPanel`: collection role `Children`.
- `ImButton`: single role `Content`.
- `ImScrollBox`: single role `Content`.
- `ImExpandableBox`: single roles `Header` and `Body`.
- `ImTabView`: collection role `Tabs`, with metadata `Title`, `Icon`, `bDirty`.
- `ImTitleBar`: collection roles `LeadingItems` and `TrailingItems`.

## Migration Plan

### Phase 1: Diagnostics And Guardrails

- Add document lint diagnostics for unsupported child fields.
- Warn or fail when `ImTitleBar` contains `Children`.
- Warn or fail when a widget receives a child relation it does not support.
- Make CLI `ui lint` report relation-level errors with exact JSON paths.
- Add tests covering malformed title-bar documents.

This phase prevents more silent data loss without changing file format.

### Phase 2: Centralize Existing Special Cases

- Add `WidgetChildSchema` in the editor layer.
- Move child-attachment decisions from scattered `dynamic_pointer_cast` checks into the schema.
- Keep current JSON format for compatibility.
- Make serializers and CLI commands consult the schema for default insertion roles.
- Keep existing code generation output, but select generation behavior through relation metadata.

This phase reduces future duplication while preserving existing documents.

### Phase 3: Introduce Unified Relation Format

- Add a new document version that stores child relations in a unified format.
- Support reading old fields such as `Children`, `Content`, `TabItems`, `LeadingItems`, and `TrailingItems`.
- Save new documents in the unified relation format.
- Provide a migration command:

```text
ImWidgetEditorCLI ui migrate-relations <input.ui.json> <output.ui.json>
```

This phase makes the file format consistent for agents and tooling.

### Phase 4: Improve CLI And Editor Workflows

- Add role-aware CLI commands:

```text
ImWidgetEditorCLI ui roles <file.ui.json> <widget-id>
ImWidgetEditorCLI ui add <file.ui.json> <parent-id> <type> --role LeadingItems
ImWidgetEditorCLI ui move <file.ui.json> <widget-id> <parent-id> --role Body --index 0
```

- Show child roles explicitly in document tree views.
- Make drag/drop pick the correct default role or ask the user when multiple roles are valid.
- Allow snapshots and generated code to report missing required child roles.

### Phase 5: Optional Runtime API Cleanup

After the editor/document layer is stable, consider adding runtime helpers:

```cpp
bool ImWidget::CanAcceptChildRole(FName Role) const;
bool ImWidget::AddChildToRole(FName Role, const Ptr& Child);
std::vector<FChildRoleInfo> ImWidget::GetChildRoles() const;
```

This should be optional and staged carefully. The current runtime APIs are already working, and the immediate pain is mostly in editor/tooling consistency.

## Compatibility Strategy

- Keep reading existing documents.
- Preserve old JSON fields during an initial compatibility window if needed.
- Prefer emitting diagnostics before changing save behavior.
- Provide one-shot migration tooling instead of silently rewriting files in unrelated operations.
- Keep generated C++ behavior stable during the transition.

## Recommended Next Step

For the immediate SDKBuilder title-bar issue:

- Move the title text from `Children` to `LeadingItems`.
- Add an explicit icon item if a visible title-bar icon is required.
- Regenerate code.

For the architecture issue:

- Start with Phase 1 diagnostics.
- Then implement `WidgetChildSchema` and migrate current special cases behind it before adding more complex controls.

