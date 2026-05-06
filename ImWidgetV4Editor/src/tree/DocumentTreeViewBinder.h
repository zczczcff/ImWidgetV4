#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace ImWidgetV4 {
class ImDesignerSurface;
class ImTextOutlineItem;
class ImTextOutlineView;
class ImWidget;
}

namespace ImWidgetV4Editor {

class DocumentTreeViewBinder {
public:
    DocumentTreeViewBinder() = default;

    void Bind(
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& treeView,
        const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface);

    void RebuildFromRoot(
        const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);

    void SyncSelectionFromDesigner(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    std::shared_ptr<ImWidgetV4::ImWidget> ResolveWidget(ImWidgetV4::ImTextOutlineItem* item) const;
    ImWidgetV4::ImTextOutlineItem* ResolveItem(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;

private:
    void RebuildChildren(
        ImWidgetV4::ImTextOutlineItem* parentItem,
        const std::shared_ptr<ImWidgetV4::ImWidget>& parentWidget);
    std::string BuildItemLabel(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;

    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_TreeView;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
    std::unordered_map<ImWidgetV4::ImTextOutlineItem*, std::shared_ptr<ImWidgetV4::ImWidget>> m_ItemToWidget;
    std::unordered_map<const ImWidgetV4::ImWidget*, ImWidgetV4::ImTextOutlineItem*> m_WidgetToItem;
    bool m_bSyncingSelection = false;
};

} // namespace ImWidgetV4Editor
