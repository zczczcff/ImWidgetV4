#include "RemoveWidgetCommand.h"

#include "../editor/EditorSession.h"
#include "../editor/LogicalWidgetTree.h"

#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/widgets/VerticalBox.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

RemoveWidgetCommand::RemoveWidgetCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& reinsertionTarget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
    bool bBeforeDirty,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_Widget(widget)
    , m_ReinsertionTarget(reinsertionTarget)
    , m_PreferredSelection(preferredSelection)
    , m_bBeforeDirty(bBeforeDirty)
    , m_bAfterDirty(bAfterDirty)
{
}

bool RemoveWidgetCommand::Execute()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget) {
        return false;
    }

    if (!CaptureRemovalState(session)) {
        return false;
    }

    return session->ApplyWidgetRemoval(
        m_Widget,
        preferredSelection,
        m_bAfterDirty);
}

bool RemoveWidgetCommand::Undo()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    if (!session || !m_Widget || !m_bCapturedRemovalState) {
        return false;
    }

    return RestoreRemovedWidget(session);
}

bool RemoveWidgetCommand::CaptureRemovalState(const std::shared_ptr<EditorSession>& session)
{
    if (!session || !m_Widget || m_bCapturedRemovalState) {
        return session && m_Widget && m_bCapturedRemovalState;
    }

    const auto& document = session->GetDocument();
    if (!document) {
        return false;
    }

    const auto root = document->GetRootWidget();
    m_bRemovedRootWidget = m_Widget == root;
    if (m_bRemovedRootWidget) {
        m_ReinsertionIndex = 0;
        m_SlotJson = json();
        m_bCapturedRemovalState = true;
        return true;
    }

    const std::shared_ptr<ImWidget> parent = document->FindLogicalParent(m_Widget);
    if (!parent) {
        return false;
    }

    m_ReinsertionTarget = parent;
    m_ReinsertionIndex = LogicalWidgetTree::FindLogicalChildIndex(parent, m_Widget);
    if (m_ReinsertionIndex < 0) {
        return false;
    }

    if (auto panelParent = std::dynamic_pointer_cast<ImPanelWidget>(parent)) {
        if (const ImSlot* slot = panelParent->GetSlotForChild(m_Widget)) {
            m_SlotJson = slot->ToJson();
        }
    } else if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        m_SlotJson = json::object();
        m_SlotJson["Role"] = m_ReinsertionIndex < static_cast<int>(leadingCount) ? "Leading" : "Trailing";
    } else {
        m_SlotJson = json();
    }

    m_bCapturedRemovalState = true;
    return true;
}

bool RemoveWidgetCommand::RestoreRemovedWidget(const std::shared_ptr<EditorSession>& session)
{
    if (!session || !m_Widget) {
        return false;
    }

    const auto& document = session->GetDocument();
    if (!document) {
        return false;
    }

    if (m_bRemovedRootWidget) {
        if (document->GetRootWidget()) {
            return false;
        }

        document->SetRootWidget(m_Widget);
        session->SetDocumentDirtyState(m_bBeforeDirty);
        session->RefreshDocumentViews(m_Widget);
        return true;
    }

    const std::shared_ptr<ImWidget> parent = m_ReinsertionTarget.lock();
    if (!parent || m_ReinsertionIndex < 0) {
        return false;
    }

    bool bInserted = false;
    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        auto slot = std::make_unique<ImCanvasPanelSlot>();
        if (!m_SlotJson.is_null()) {
            slot->FromJson(m_SlotJson);
        }
        canvas->InsertSlot(m_ReinsertionIndex, m_Widget, std::move(slot));
        bInserted = true;
    } else if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->InsertChild(m_ReinsertionIndex, m_Widget);
        if (auto* slot = verticalBox->GetSlotForChild(m_Widget); slot && !m_SlotJson.is_null()) {
            slot->FromJson(m_SlotJson);
        }
        bInserted = true;
    } else if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->InsertChild(m_ReinsertionIndex, m_Widget);
        if (auto* slot = horizontalBox->GetSlotForChild(m_Widget); slot && !m_SlotJson.is_null()) {
            slot->FromJson(m_SlotJson);
        }
        bInserted = true;
    } else if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        if (!scrollBox->GetContent()) {
            scrollBox->SetContent(m_Widget);
            bInserted = true;
        }
    } else if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (!button->GetContent()) {
            button->SetContent(m_Widget);
            if (auto* slot = button->GetSlotForChild(m_Widget); slot && !m_SlotJson.is_null()) {
                slot->FromJson(m_SlotJson);
            }
            bInserted = true;
        }
    } else if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (m_ReinsertionIndex == 0 && !expandableBox->GetHeader()) {
            expandableBox->SetHeader(m_Widget);
            bInserted = true;
        } else if (m_ReinsertionIndex == 1 && !expandableBox->GetBody()) {
            expandableBox->SetBody(m_Widget);
            bInserted = true;
        }
    } else if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int insertedIndex = tabView->InsertTab(m_ReinsertionIndex, m_Widget->GetName().empty() ? m_Widget->GetTypeName() : m_Widget->GetName(), m_Widget);
        if (insertedIndex >= 0) {
            tabView->SetActiveTab(insertedIndex);
            bInserted = true;
        }
    } else if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        const std::string role = m_SlotJson.is_object() ? m_SlotJson.value("Role", "Leading") : "Leading";
        if (role == "Trailing") {
            const int trailingIndex = std::max(0, m_ReinsertionIndex - static_cast<int>(titleBar->GetLeadingItemCount()));
            bInserted = titleBar->InsertTrailingItem(static_cast<std::size_t>(trailingIndex), m_Widget);
        } else {
            bInserted = titleBar->InsertLeadingItem(static_cast<std::size_t>(std::max(0, m_ReinsertionIndex)), m_Widget);
        }
    } else if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (!userWidget->GetRootWidget()) {
            userWidget->SetRootWidget(m_Widget);
            bInserted = true;
        }
    } else {
        parent->InsertChildAt(m_ReinsertionIndex, m_Widget);
        bInserted = true;
    }

    if (!bInserted) {
        return false;
    }

    session->SetDocumentDirtyState(m_bBeforeDirty);
    session->RefreshDocumentViews(m_Widget);
    return true;
}

} // namespace ImWidgetV4Editor
