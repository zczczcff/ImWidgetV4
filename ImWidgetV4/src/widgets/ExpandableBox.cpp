#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>

namespace ImWidgetV4 {

namespace {

bool IsWidgetInSubtree(const std::shared_ptr<ImWidget>& root, const std::shared_ptr<ImWidget>& widget)
{
    if (!root || !widget) {
        return false;
    }

    for (std::shared_ptr<ImWidget> current = widget; current; current = current->GetParent()) {
        if (current == root) {
            return true;
        }
    }

    return false;
}

FGeometry InsetGeometryByBorder(const FGeometry& geometry, float borderThickness)
{
    const float inset = std::max(0.0f, borderThickness);
    return FGeometry(
        FVector2(geometry.Position.X + inset, geometry.Position.Y + inset),
        FVector2(
            std::max(0.0f, geometry.Size.X - inset * 2.0f),
            std::max(0.0f, geometry.Size.Y - inset * 2.0f)));
}

} // namespace

ImExpandableBox::ImExpandableBox()
    : ImPanelWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

void ImExpandableBox::SetHeader(const Ptr& header)
{
    if (m_HeaderWidget == header) {
        return;
    }

    m_HeaderWidget = header;
    RefreshVisibleChildren();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImExpandableBox::SetBody(const Ptr& body)
{
    if (m_BodyWidget == body) {
        return;
    }

    if (m_BodyWidget && m_BodyWidget != body) {
        ClearBodyInteractionState();
    }

    m_BodyWidget = body;
    RefreshVisibleChildren();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImExpandableBox::SetExpanded(bool bExpanded)
{
    if (m_bExpanded == bExpanded) {
        return;
    }

    if (!bExpanded) {
        ClearBodyInteractionState();
    }

    m_bExpanded = bExpanded;
    RefreshVisibleChildren();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    OnExpandedStateChanged.Broadcast(*this, m_bExpanded);
}

void ImExpandableBox::ToggleExpanded()
{
    SetExpanded(!m_bExpanded);
}

void ImExpandableBox::SetStyle(const FExpandableBoxStyle& style)
{
    m_Style = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

std::unique_ptr<ImSlot> ImExpandableBox::CreateSlot()
{
    return std::make_unique<ImPaddingSlot>();
}

void ImExpandableBox::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    const FVector2 containerMin = m_Geometry.GetMin();
    const FVector2 containerMax = containerMin + FVector2(m_Geometry.Size.X, ComputeVisibleHeight());
    const float cornerRadius = std::max(0.0f, m_Style.CornerRadius);

    if (m_bExpanded && m_BodyWidget) {
        paintContext.DrawContext_.DrawRectFilled(
            containerMin,
            containerMax,
            m_Style.BodyBackgroundColor,
            cornerRadius);
    }

    const FColor headerColor = m_bPressed
        ? m_Style.HeaderPressedBackgroundColor
        : (m_bHovered ? m_Style.HeaderHoveredBackgroundColor : m_Style.HeaderBackgroundColor);

    if (m_bExpanded && m_BodyWidget) {
        paintContext.DrawContext_.DrawRectFilled(
            m_HeaderGeometry.GetMin(),
            m_HeaderGeometry.GetMax(),
            headerColor,
            cornerRadius);
    } else {
        paintContext.DrawContext_.DrawRectFilled(
            m_HeaderGeometry.GetMin(),
            m_HeaderGeometry.GetMax(),
            headerColor,
            cornerRadius);
    }

    if (m_IndicatorHotspotGeometry.IsValid()) {
        const FVector2 indicatorCenter = m_IndicatorHotspotGeometry.GetCenter();
        const float halfSize = m_Style.IndicatorSize * 0.5f;
        const FColor indicatorColor = m_bHovered ? m_Style.IndicatorHoveredColor : m_Style.IndicatorColor;

        if (m_bExpanded) {
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X - halfSize, indicatorCenter.Y - halfSize * 0.45f));
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X + halfSize, indicatorCenter.Y - halfSize * 0.45f));
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X, indicatorCenter.Y + halfSize * 0.75f));
        } else {
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X - halfSize * 0.45f, indicatorCenter.Y - halfSize));
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X - halfSize * 0.45f, indicatorCenter.Y + halfSize));
            paintContext.DrawContext_.PathLineTo(FVector2(indicatorCenter.X + halfSize * 0.75f, indicatorCenter.Y));
        }
        paintContext.DrawContext_.PathFill(indicatorColor);
    }

    paintContext.DrawContext_.DrawRect(
        containerMin,
        containerMax,
        HasKeyboardFocus() ? m_Style.FocusedOutlineColor : m_Style.BorderColor,
        cornerRadius,
        std::max(1.0f, m_Style.BorderThickness));

    RenderChildren(paintContext);
}

FVector2 ImExpandableBox::GetMinSize() const
{
    const float borderInset = std::max(0.0f, m_Style.BorderThickness);
    const float totalBorderWidth = borderInset * 2.0f;
    const float indicatorStripWidth = m_Style.IndicatorSize + m_Style.IndicatorSpacing;
    const FVector2 headerMin = m_HeaderWidget ? m_HeaderWidget->GetMinSize() : FVector2(0.0f, 0.0f);
    const float headerWidth =
        totalBorderWidth +
        m_Style.HeaderPadding.Left + indicatorStripWidth + headerMin.X + m_Style.HeaderPadding.Right;
    const float headerHeight =
        totalBorderWidth +
        m_Style.HeaderPadding.Top +
        std::max(m_Style.IndicatorSize, headerMin.Y) +
        m_Style.HeaderPadding.Bottom;

    float minWidth = std::max(m_Style.MinDesiredSize.X, headerWidth);
    float minHeight = std::max(m_Style.MinDesiredSize.Y, headerHeight);

    if (m_bExpanded && m_BodyWidget) {
        const FVector2 bodyMin = m_BodyWidget->GetMinSize();
        minWidth = std::max(
            minWidth,
            totalBorderWidth + m_Style.BodyPadding.Left + bodyMin.X + m_Style.BodyPadding.Right);
        minHeight += m_Style.BodyPadding.Top + bodyMin.Y + m_Style.BodyPadding.Bottom;
    }

    return FVector2(minWidth, minHeight);
}

FReply ImExpandableBox::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::MouseEnter || event.Type == EInputEventType::MouseMove) {
        SetHovered(m_HeaderGeometry.Contains(event.MousePosition));
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        SetHovered(false);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        ContainsIndicatorHotspot(event.MousePosition)) {
        SetPressed(true);
        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_bPressed) {
        const bool bInsideHotspot = ContainsIndicatorHotspot(event.MousePosition);
        SetPressed(false);
        if (bInsideHotspot) {
            ToggleExpanded();
        }
        return FReply::Handled().ReleaseMouseCapture();
    }

    if (event.Type == EInputEventType::KeyDown && HasKeyboardFocus()) {
        if (event.Key == EKey::Enter || event.Key == EKey::Space) {
            ToggleExpanded();
            return FReply::Handled();
        }
    }

    return FReply::Unhandled();
}

bool ImExpandableBox::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible) {
        return false;
    }

    Relayout();
    const FGeometry visibleGeometry(m_Geometry.Position, FVector2(m_Geometry.Size.X, ComputeVisibleHeight()));
    if (!visibleGeometry.Contains(position)) {
        return false;
    }

    outPath.push_back(shared_from_this());
    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if ((*it)->BuildHitTestPath(position, outPath)) {
            return true;
        }
    }

    return true;
}

void ImExpandableBox::Relayout()
{
    const FGeometry innerGeometry = InsetGeometryByBorder(m_Geometry, m_Style.BorderThickness);
    const float headerHeight = ComputeHeaderHeight();
    const float bodyHeight = ComputeBodyHeight();
    const float visibleHeight = m_bExpanded ? headerHeight + bodyHeight : headerHeight;
    const FVector2 visibleSize(innerGeometry.Size.X, visibleHeight);

    m_HeaderGeometry = FGeometry(innerGeometry.Position, FVector2(visibleSize.X, headerHeight));
    m_BodyGeometry = FGeometry(
        FVector2(innerGeometry.Position.X, innerGeometry.Position.Y + headerHeight),
        FVector2(visibleSize.X, bodyHeight));

    const float indicatorY = m_HeaderGeometry.Position.Y +
        std::max(0.0f, (headerHeight - m_Style.IndicatorSize) * 0.5f);
    m_IndicatorHotspotGeometry = FGeometry(
        FVector2(m_HeaderGeometry.Position.X + m_Style.HeaderPadding.Left, indicatorY),
        FVector2(m_Style.IndicatorSize, m_Style.IndicatorSize));

    const int headerSlotIndex = m_HeaderWidget ? 0 : -1;
    const int bodySlotIndex = m_bExpanded ? (m_HeaderWidget ? 1 : 0) : -1;

    ImPaddingSlot* headerSlot =
        headerSlotIndex >= 0 ? dynamic_cast<ImPaddingSlot*>(GetSlotAt(headerSlotIndex)) : nullptr;
    if (headerSlot && m_HeaderWidget) {
        headerSlot->PaddingLeft = m_Style.HeaderPadding.Left + m_Style.IndicatorSize + m_Style.IndicatorSpacing;
        headerSlot->PaddingRight = m_Style.HeaderPadding.Right;
        headerSlot->PaddingTop = m_Style.HeaderPadding.Top;
        headerSlot->PaddingBottom = m_Style.HeaderPadding.Bottom;
        headerSlot->SetSlotPosition(m_HeaderGeometry.Position);
        headerSlot->SetSlotSize(m_HeaderGeometry.Size);
        headerSlot->ApplyLayout(m_HeaderWidget.get());
    }

    ImPaddingSlot* bodySlot =
        bodySlotIndex >= 0 ? dynamic_cast<ImPaddingSlot*>(GetSlotAt(bodySlotIndex)) : nullptr;
    if (bodySlot && m_BodyWidget && m_bExpanded) {
        bodySlot->PaddingLeft = m_Style.BodyPadding.Left;
        bodySlot->PaddingRight = m_Style.BodyPadding.Right;
        bodySlot->PaddingTop = m_Style.BodyPadding.Top;
        bodySlot->PaddingBottom = m_Style.BodyPadding.Bottom;
        bodySlot->SetSlotPosition(m_BodyGeometry.Position);
        bodySlot->SetSlotSize(m_BodyGeometry.Size);
        bodySlot->ApplyLayout(m_BodyWidget.get());
    }
}

void ImExpandableBox::RefreshVisibleChildren()
{
    ImWidget::ClearChildren();
    m_Slots.clear();

    if (m_HeaderWidget) {
        AddSlot(m_HeaderWidget, std::make_unique<ImPaddingSlot>());
    }

    if (m_bExpanded && m_BodyWidget) {
        AddSlot(m_BodyWidget, std::make_unique<ImPaddingSlot>());
    }
}

void ImExpandableBox::ClearBodyInteractionState()
{
    if (!m_Application || !m_BodyWidget) {
        return;
    }

    if (IsDescendantOfBody(m_Application->GetKeyboardFocus())) {
        m_Application->ClearKeyboardFocus();
    }

    if (IsDescendantOfBody(m_Application->GetMouseCapture())) {
        m_Application->ReleaseMouseCapture();
    }
}

bool ImExpandableBox::IsDescendantOfBody(const std::shared_ptr<ImWidget>& widget) const
{
    return IsWidgetInSubtree(m_BodyWidget, widget);
}

void ImExpandableBox::SetHovered(bool bHovered)
{
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);

    if (m_bHovered) {
        OnHoverBegin.Broadcast(*this);
    } else {
        OnHoverEnd.Broadcast(*this);
    }
}

void ImExpandableBox::SetPressed(bool bPressed)
{
    if (m_bPressed == bPressed) {
        return;
    }

    m_bPressed = bPressed;
    Invalidate(EInvalidateReason::Paint);
}

bool ImExpandableBox::ContainsIndicatorHotspot(const FVector2& position) const
{
    return m_IndicatorHotspotGeometry.Contains(position);
}

float ImExpandableBox::ComputeHeaderHeight() const
{
    const FVector2 headerMin = m_HeaderWidget ? m_HeaderWidget->GetMinSize() : FVector2(0.0f, 0.0f);
    return m_Style.HeaderPadding.Top +
        std::max(m_Style.IndicatorSize, headerMin.Y) +
        m_Style.HeaderPadding.Bottom;
}

float ImExpandableBox::ComputeBodyHeight() const
{
    if (!m_bExpanded || !m_BodyWidget) {
        return 0.0f;
    }

    const FVector2 bodyMin = m_BodyWidget->GetMinSize();
    return m_Style.BodyPadding.Top + bodyMin.Y + m_Style.BodyPadding.Bottom;
}

float ImExpandableBox::ComputeVisibleHeight() const
{
    return m_bExpanded ? (ComputeHeaderHeight() + ComputeBodyHeight()) : ComputeHeaderHeight();
}

} // namespace ImWidgetV4
