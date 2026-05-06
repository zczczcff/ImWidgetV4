#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/core/Application.h>

namespace ImWidgetV4 {
namespace {

bool ContainsWidgetInSubtree(const std::shared_ptr<ImWidget>& subtreeRoot, const std::shared_ptr<ImWidget>& target)
{
    if (!subtreeRoot || !target) {
        return false;
    }

    if (subtreeRoot == target) {
        return true;
    }

    for (const auto& child : subtreeRoot->GetChildren()) {
        if (ContainsWidgetInSubtree(child, target)) {
            return true;
        }
    }

    return false;
}

void CollectNamedWidgetsDepthFirst(
    const std::shared_ptr<ImWidget>& widget,
    std::unordered_map<std::string, std::weak_ptr<ImWidget>>& namedWidgets)
{
    if (!widget) {
        return;
    }

    if (!widget->GetName().empty()) {
        namedWidgets.try_emplace(widget->GetName(), widget);
    }

    for (const auto& child : widget->GetChildren()) {
        CollectNamedWidgetsDepthFirst(child, namedWidgets);
    }
}

} // namespace

ImUserWidget::ImUserWidget()
    : ImWidget()
{
    SetHitTestVisible(true);
}

void ImUserWidget::SetRootWidget(const Ptr& rootWidget)
{
    if (m_ConfiguredRootWidget == rootWidget &&
        m_RootWidget == rootWidget &&
        ((rootWidget != nullptr) == m_bRootBuilt)) {
        return;
    }

    m_ConfiguredRootWidget = rootWidget;
    m_bRootBuilt = rootWidget != nullptr;
    ResetRootWidget(rootWidget);
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint | EInvalidateReason::ChildOrder);
}

std::shared_ptr<ImWidget> ImUserWidget::GetRootWidget() const
{
    const_cast<ImUserWidget*>(this)->EnsureRootBuilt();
    return m_RootWidget;
}

void ImUserWidget::Rebuild()
{
    const Ptr rebuiltRoot = RebuildWidget();
    ResetRootWidget(rebuiltRoot);
    m_bRootBuilt = true;
    OnRootWidgetRebuilt();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint | EInvalidateReason::ChildOrder);
}

std::shared_ptr<ImWidget> ImUserWidget::FindWidgetByName(const std::string& name) const
{
    if (name.empty()) {
        return nullptr;
    }

    const_cast<ImUserWidget*>(this)->EnsureRootBuilt();

    const auto it = m_NamedWidgets.find(name);
    if (it == m_NamedWidgets.end()) {
        return nullptr;
    }

    return it->second.lock();
}

void ImUserWidget::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    EnsureRootBuilt();
    SyncRootGeometry();

    if (m_RootWidget) {
        m_RootWidget->Paint(paintContext);
    }
}

FVector2 ImUserWidget::GetMinSize() const
{
    const_cast<ImUserWidget*>(this)->EnsureRootBuilt();
    return m_RootWidget ? m_RootWidget->GetMinSize() : FVector2(0.0f, 0.0f);
}

bool ImUserWidget::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    EnsureRootBuilt();
    SyncRootGeometry();

    outPath.push_back(shared_from_this());
    if (m_RootWidget && m_RootWidget->BuildHitTestPath(position, outPath)) {
        return true;
    }

    return true;
}

void ImUserWidget::AddChild(const Ptr& child)
{
    SetRootWidget(child);
}

void ImUserWidget::ClearChildren()
{
    SetRootWidget(nullptr);
}

ImWidget::Ptr ImUserWidget::RebuildWidget()
{
    return m_ConfiguredRootWidget;
}

void ImUserWidget::OnRootWidgetRebuilt()
{
}

void ImUserWidget::EnsureRootBuilt()
{
    if (m_bRootBuilt) {
        return;
    }

    Rebuild();
}

void ImUserWidget::ResetRootWidget(const Ptr& newRootWidget)
{
    CleanupInteractionStateForSubtree(m_RootWidget);
    ImWidget::ClearChildren();
    m_NamedWidgets.clear();
    m_RootWidget = newRootWidget;

    if (m_RootWidget) {
        ImWidget::AddChild(m_RootWidget);
        SyncRootGeometry();
        RebuildNamedWidgetCache();
    }
}

void ImUserWidget::SyncRootGeometry()
{
    if (m_RootWidget) {
        m_RootWidget->SetGeometry(m_Geometry);
    }
}

void ImUserWidget::RebuildNamedWidgetCache()
{
    m_NamedWidgets.clear();
    CollectNamedWidgetsDepthFirst(m_RootWidget, m_NamedWidgets);
}

void ImUserWidget::CleanupInteractionStateForSubtree(const Ptr& subtreeRoot)
{
    if (!subtreeRoot || m_Application == nullptr) {
        return;
    }

    if (ContainsWidgetInSubtree(subtreeRoot, m_Application->GetKeyboardFocus())) {
        m_Application->ClearKeyboardFocus();
    }

    if (ContainsWidgetInSubtree(subtreeRoot, m_Application->GetMouseCapture())) {
        m_Application->ReleaseMouseCapture();
    }
}

} // namespace ImWidgetV4
