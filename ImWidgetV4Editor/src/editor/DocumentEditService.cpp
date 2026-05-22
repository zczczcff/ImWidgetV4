#include "DocumentEditService.h"

#include "LogicalWidgetTree.h"
#include "../serialization/WidgetSerializer.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/widgets/VerticalBox.h>

#include <algorithm>
#include <limits>
#include <memory>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::string StripImPrefix(const std::string& typeName)
{
    return typeName.rfind("Im", 0) == 0 ? typeName.substr(2) : typeName;
}

bool TryInsertIntoScrollBoxAt(
    const std::shared_ptr<ImScrollBox>& scrollBox,
    int insertIndex,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!scrollBox || !widget) {
        return false;
    }

    if (!scrollBox->GetContent()) {
        scrollBox->SetContent(widget);
        return true;
    }

    if (auto contentVerticalBox = std::dynamic_pointer_cast<ImVerticalBox>(scrollBox->GetContent())) {
        contentVerticalBox->InsertChild(insertIndex, widget);
        return true;
    }

    auto existingContent = scrollBox->GetContent();
    auto wrapper = std::make_shared<ImVerticalBox>();
    wrapper->SetName("ScrollContent");
    if (insertIndex <= 0) {
        wrapper->AddChild(widget);
        wrapper->AddChild(existingContent);
    } else {
        wrapper->AddChild(existingContent);
        wrapper->AddChild(widget);
    }
    scrollBox->SetContent(wrapper);
    return true;
}

} // namespace

std::string BuildDefaultWidgetName(const std::string& typeName)
{
    std::string name = typeName;
    if (name.size() > 2 && name[0] == 'I' && name[1] == 'm') {
        name = name.substr(2);
    }
    return name.empty() ? "Widget" : name;
}

std::string BuildTabTitleForWidget(
    const std::shared_ptr<ImWidget>& widget,
    bool bStripImPrefixForFallback)
{
    if (!widget) {
        return "Tab";
    }
    if (!widget->GetName().empty()) {
        return widget->GetName();
    }
    return bStripImPrefixForFallback ? StripImPrefix(widget->GetTypeName()) : widget->GetTypeName();
}

void InitializeNewWidgetDefaults(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return;
    }

    widget->SetName(BuildDefaultWidgetName(widget->GetTypeName()));
    if (auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(widget)) {
        textBlock->SetText("Text");
    } else if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        button->SetText("Button");
    } else if (auto comboBox = std::dynamic_pointer_cast<ImComboBox>(widget)) {
        comboBox->SetItems({"Option A", "Option B", "Option C"});
        comboBox->SetSelectedIndex(0);
    } else if (auto checkBox = std::dynamic_pointer_cast<ImCheckBox>(widget)) {
        checkBox->SetLabel("CheckBox");
    } else if (auto editableText = std::dynamic_pointer_cast<ImEditableText>(widget)) {
        editableText->SetText("EditableText");
    } else if (auto textList = std::dynamic_pointer_cast<ImTextList>(widget)) {
        textList->SetItems({"Item 1", "Item 2", "Item 3"});
    } else if (auto colorPicker = std::dynamic_pointer_cast<ImColorPicker>(widget)) {
        colorPicker->SetColor(FColor::FromBytes(86, 156, 214));
    } else if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        auto content = std::make_shared<ImTextBlock>();
        content->SetName("TabContent");
        content->SetText("Tab Content");
        tabView->AddTab("Tab", content);
    } else if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        auto header = std::make_shared<ImTextBlock>();
        header->SetName("Header");
        header->SetText("Expandable Header");
        auto body = std::make_shared<ImTextBlock>();
        body->SetName("Body");
        body->SetText("Expandable Body");
        expandableBox->SetHeader(header);
        expandableBox->SetBody(body);
        expandableBox->SetExpanded(true);
    }
}

std::shared_ptr<ImWidget> CloneWidgetTree(const std::shared_ptr<ImWidget>& widget, std::string& outError)
{
    if (!widget) {
        outError = "No widget selected.";
        return nullptr;
    }

    return CloneWidgetTreeFromJson(WidgetSerializer::SerializeWidgetTree(widget), outError);
}

std::shared_ptr<ImWidget> CloneWidgetTreeFromJson(const json& widgetJson, std::string& outError)
{
    if (widgetJson.is_null()) {
        outError = "Widget JSON is empty.";
        return nullptr;
    }

    FWidgetSerializationResult result = WidgetSerializer::DeserializeWidgetTree(widgetJson);
    if (!result.bSuccess || !result.Widget) {
        outError = result.ErrorMessage.empty()
            ? "Failed to clone widget tree."
            : result.ErrorMessage;
        return nullptr;
    }
    return result.Widget;
}

bool IsLogicalAncestorOf(
    EditorDocument& document,
    const std::shared_ptr<ImWidget>& possibleAncestor,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!possibleAncestor || !widget) {
        return false;
    }

    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    while (parent) {
        if (parent == possibleAncestor) {
            return true;
        }
        parent = document.FindLogicalParent(parent);
    }
    return false;
}

bool TryInsertWidgetIntoParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& child,
    std::string& outError,
    const FDocumentInsertOptions& options)
{
    if (!parent || !child) {
        outError = "Parent and child widgets are required.";
        return false;
    }

    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        if (options.bUseCanvasRelativeSize) {
            canvas->AddChildAt(child, options.CanvasRelativePosition, options.CanvasRelativeSize);
        } else {
            canvas->AddChildAt(child, options.CanvasRelativePosition);
        }
        return true;
    }
    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->AddChild(child);
        return true;
    }
    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->AddChild(child);
        return true;
    }
    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (button->GetContent()) {
            outError = "Button already has content.";
            return false;
        }
        button->SetContent(child);
        return true;
    }
    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        if (TryInsertIntoScrollBoxAt(scrollBox, std::numeric_limits<int>::max(), child)) {
            return true;
        }
        outError = "Failed to add widget to ScrollBox.";
        return false;
    }
    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (!expandableBox->GetHeader()) {
            expandableBox->SetHeader(child);
            return true;
        }
        if (!expandableBox->GetBody()) {
            expandableBox->SetBody(child);
            expandableBox->SetExpanded(true);
            return true;
        }
        outError = "ExpandableBox already has header and body.";
        return false;
    }
    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int tabIndex = tabView->AddTab(
            BuildTabTitleForWidget(child, options.bStripImPrefixForTabFallback),
            child);
        if (tabIndex < 0) {
            outError = "Failed to add tab content.";
            return false;
        }
        tabView->SetActiveTab(tabIndex);
        return true;
    }
    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        if (options.bUseTitleBarDropPosition) {
            const FGeometry geometry = titleBar->GetGeometry();
            const bool bTrailing = geometry.Size.X > 0.0f &&
                options.DropPosition.X >= geometry.Position.X + geometry.Size.X * 0.5f;
            if (bTrailing) {
                titleBar->AddTrailingItem(child);
            } else {
                titleBar->AddLeadingItem(child);
            }
        } else {
            titleBar->AddLeadingItem(child);
        }
        return true;
    }
    if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (userWidget->GetRootWidget()) {
            outError = "UserWidget already has a root widget.";
            return false;
        }
        userWidget->SetRootWidget(child);
        return true;
    }

    outError = "Parent widget does not accept child widgets: " + parent->GetTypeName();
    return false;
}

bool TryInsertWidgetIntoParentAt(
    const std::shared_ptr<ImWidget>& parent,
    int insertIndex,
    const std::shared_ptr<ImWidget>& child,
    std::string& outError,
    const FDocumentInsertOptions& options)
{
    if (!parent || !child) {
        outError = "Parent and child widgets are required.";
        return false;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->InsertChild(insertIndex, child);
        return true;
    }
    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->InsertChild(insertIndex, child);
        return true;
    }
    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int insertedIndex = tabView->InsertTab(
            insertIndex,
            BuildTabTitleForWidget(child, options.bStripImPrefixForTabFallback),
            child);
        if (insertedIndex >= 0) {
            tabView->SetActiveTab(insertedIndex);
            return true;
        }
        outError = "Failed to insert tab content.";
        return false;
    }
    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        if (insertIndex <= static_cast<int>(leadingCount)) {
            return titleBar->InsertLeadingItem(static_cast<std::size_t>(std::max(0, insertIndex)), child);
        }
        return titleBar->InsertTrailingItem(
            static_cast<std::size_t>(std::max(0, insertIndex - static_cast<int>(leadingCount))),
            child);
    }
    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        if (TryInsertIntoScrollBoxAt(scrollBox, insertIndex, child)) {
            return true;
        }
        outError = "Failed to insert widget into ScrollBox.";
        return false;
    }

    outError = "Parent widget does not support indexed insertion: " + parent->GetTypeName();
    return false;
}

bool RemoveWidgetFromParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& child)
{
    if (!parent || !child) {
        return false;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int tabIndex = LogicalWidgetTree::FindTabContentIndex(tabView, child);
        return tabIndex >= 0 && tabView->RemoveTab(tabIndex);
    }
    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        return titleBar->RemoveLeadingItem(child) || titleBar->RemoveTrailingItem(child);
    }
    if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (userWidget->GetRootWidget() == child) {
            userWidget->SetRootWidget(nullptr);
            return true;
        }
    }
    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (button->GetContent() == child) {
            button->SetContent(nullptr);
            return true;
        }
    }
    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (expandableBox->GetHeader() == child) {
            expandableBox->SetHeader(nullptr);
            return true;
        }
        if (expandableBox->GetBody() == child) {
            expandableBox->SetBody(nullptr);
            return true;
        }
    }
    if (auto panelParent = std::dynamic_pointer_cast<ImPanelWidget>(parent)) {
        return panelParent->RemoveChild(child);
    }
    return parent->RemoveChild(child);
}

bool TryDuplicateWidgetInParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& sourceWidget,
    const std::shared_ptr<ImWidget>& cloneWidget,
    std::string& outError,
    const FDocumentDuplicateOptions& options)
{
    if (!parent || !sourceWidget || !cloneWidget) {
        outError = "Parent, source, and clone widgets are required.";
        return false;
    }

    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        const auto* sourceSlot = dynamic_cast<const ImCanvasPanelSlot*>(canvas->GetSlotForChild(sourceWidget));
        if (!sourceSlot) {
            outError = "Source widget has no CanvasPanel slot.";
            return false;
        }

        std::unique_ptr<ImCanvasPanelSlot> slot;
        if (options.bCopySlots) {
            slot = std::make_unique<ImCanvasPanelSlot>();
            slot->FromJson(sourceSlot->ToJson());
            const FVector2 duplicatedPosition(
                std::clamp(
                    sourceSlot->GetRelativePosition().X + options.CanvasDuplicateOffset.X,
                    0.0f,
                    0.95f),
                std::clamp(
                    sourceSlot->GetRelativePosition().Y + options.CanvasDuplicateOffset.Y,
                    0.0f,
                    0.95f));
            slot->SetRelativePosition(duplicatedPosition);
        }
        canvas->AddChildWithSlot(cloneWidget, std::move(slot));
        return true;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        const ImSlot* sourceSlot = verticalBox->GetSlotForChild(sourceWidget);
        std::unique_ptr<ImBoxSlot> slot;
        if (options.bCopySlots && sourceSlot) {
            slot = std::make_unique<ImBoxSlot>();
            slot->FromJson(sourceSlot->ToJson());
        }
        verticalBox->AddChildWithSlot(cloneWidget, std::move(slot));
        return true;
    }

    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        const ImSlot* sourceSlot = horizontalBox->GetSlotForChild(sourceWidget);
        std::unique_ptr<ImBoxSlot> slot;
        if (options.bCopySlots && sourceSlot) {
            slot = std::make_unique<ImBoxSlot>();
            slot->FromJson(sourceSlot->ToJson());
        }
        horizontalBox->AddChildWithSlot(cloneWidget, std::move(slot));
        return true;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        FDocumentInsertOptions insertOptions;
        insertOptions.DropPosition = sourceWidget->GetGeometry().Position;
        insertOptions.bStripImPrefixForTabFallback = true;
        return TryInsertWidgetIntoParent(scrollBox, cloneWidget, outError, insertOptions);
    }

    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        button->SetContent(cloneWidget);
        return true;
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (expandableBox->GetHeader() == sourceWidget) {
            expandableBox->SetHeader(cloneWidget);
            return true;
        }
        if (expandableBox->GetBody() == sourceWidget) {
            expandableBox->SetBody(cloneWidget);
            expandableBox->SetExpanded(true);
            return true;
        }
        outError = "Source widget is not an ExpandableBox header or body.";
        return false;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int sourceTabIndex = LogicalWidgetTree::FindTabContentIndex(tabView, sourceWidget);
        if (sourceTabIndex < 0) {
            outError = "Source widget is not a TabView tab content.";
            return false;
        }

        const FTabViewItem* sourceTab = tabView->GetTab(sourceTabIndex);
        if (!sourceTab) {
            outError = "Source tab was not found.";
            return false;
        }

        const int duplicatedIndex = tabView->AddTab(sourceTab->Title + options.TabTitleSuffix, cloneWidget);
        if (duplicatedIndex < 0) {
            outError = "Failed to duplicate tab content.";
            return false;
        }

        tabView->SetTabEnabled(duplicatedIndex, sourceTab->bEnabled);
        tabView->SetTabClosable(duplicatedIndex, sourceTab->bClosable);
        tabView->SetTabDirty(duplicatedIndex, sourceTab->bDirty);
        tabView->SetActiveTab(duplicatedIndex);
        return true;
    }

    outError = "Parent widget does not support duplicate insertion: " + parent->GetTypeName();
    return false;
}

} // namespace ImWidgetV4Editor
