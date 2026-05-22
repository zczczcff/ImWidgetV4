#pragma once

#include "EditorDocument.h"

#include <imwidgetv4/core/Types.h>

#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

struct FDocumentInsertOptions {
    ImWidgetV4::FVector2 CanvasRelativePosition {0.05f, 0.05f};
    ImWidgetV4::FVector2 CanvasRelativeSize {0.0f, 0.0f};
    ImWidgetV4::FVector2 DropPosition {0.0f, 0.0f};
    bool bUseCanvasRelativeSize = false;
    bool bUseTitleBarDropPosition = false;
    bool bStripImPrefixForTabFallback = false;
};

std::string BuildDefaultWidgetName(const std::string& typeName);
std::string BuildTabTitleForWidget(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    bool bStripImPrefixForFallback = false);
void InitializeNewWidgetDefaults(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);

std::shared_ptr<ImWidgetV4::ImWidget> CloneWidgetTree(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::string& outError);
bool IsLogicalAncestorOf(
    EditorDocument& document,
    const std::shared_ptr<ImWidgetV4::ImWidget>& possibleAncestor,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
bool TryInsertWidgetIntoParent(
    const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
    const std::shared_ptr<ImWidgetV4::ImWidget>& child,
    std::string& outError,
    const FDocumentInsertOptions& options = {});
bool TryInsertWidgetIntoParentAt(
    const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
    int insertIndex,
    const std::shared_ptr<ImWidgetV4::ImWidget>& child,
    std::string& outError,
    const FDocumentInsertOptions& options = {});
bool RemoveWidgetFromParent(
    const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
    const std::shared_ptr<ImWidgetV4::ImWidget>& child);

} // namespace ImWidgetV4Editor
