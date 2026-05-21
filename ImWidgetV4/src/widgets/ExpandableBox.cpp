#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
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

const Reflection::FTypeDesc& FExpandableBoxStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::HeaderBackgroundColor>(
            "FExpandableBoxStyle",
            "HeaderBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Header background color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::HeaderHoveredBackgroundColor>(
            "FExpandableBoxStyle",
            "HeaderHoveredBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Header hovered background color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::HeaderPressedBackgroundColor>(
            "FExpandableBoxStyle",
            "HeaderPressedBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Header pressed background color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::BodyBackgroundColor>(
            "FExpandableBoxStyle",
            "BodyBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Body background color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::BorderColor>(
            "FExpandableBoxStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::FocusedOutlineColor>(
            "FExpandableBoxStyle",
            "FocusedOutlineColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Focused outline color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::IndicatorColor>(
            "FExpandableBoxStyle",
            "IndicatorColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Indicator color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FColor, &FExpandableBoxStyle::IndicatorHoveredColor>(
            "FExpandableBoxStyle",
            "IndicatorHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Indicator hovered color"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FMargin, &FExpandableBoxStyle::HeaderPadding>(
            "FExpandableBoxStyle",
            "HeaderPadding",
            Reflection::EPropertyKind::Struct,
            "FMargin",
            "Header padding",
            &FMargin::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FMargin, &FExpandableBoxStyle::BodyPadding>(
            "FExpandableBoxStyle",
            "BodyPadding",
            Reflection::EPropertyKind::Struct,
            "FMargin",
            "Body padding",
            &FMargin::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, float, &FExpandableBoxStyle::IndicatorSize>(
            "FExpandableBoxStyle",
            "IndicatorSize",
            Reflection::EPropertyKind::Float,
            "float",
            "Indicator size"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, float, &FExpandableBoxStyle::IndicatorSpacing>(
            "FExpandableBoxStyle",
            "IndicatorSpacing",
            Reflection::EPropertyKind::Float,
            "float",
            "Indicator spacing"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, float, &FExpandableBoxStyle::BorderThickness>(
            "FExpandableBoxStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, float, &FExpandableBoxStyle::CornerRadius>(
            "FExpandableBoxStyle",
            "CornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Corner radius"),
        Reflection::MakeMemberProperty<FExpandableBoxStyle, FVector2, &FExpandableBoxStyle::MinDesiredSize>(
            "FExpandableBoxStyle",
            "MinDesiredSize",
            Reflection::EPropertyKind::Vec2,
            "FVector2",
            "Minimum desired size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FExpandableBoxStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImExpandableBox::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeObjectAccessorProperty<ImExpandableBox, bool, &ImExpandableBox::SetExpandedProperty, &ImExpandableBox::GetExpandedProperty>(
            "ImExpandableBox",
            "Expanded",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the expandable box is expanded"),
        Reflection::MakeMemberProperty<ImExpandableBox, FExpandableBoxStyle, &ImExpandableBox::m_Style>(
            "ImExpandableBox",
            "Style",
            Reflection::EPropertyKind::Struct,
            "FExpandableBoxStyle",
            "Expandable box style",
            &FExpandableBoxStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImExpandableBox",
        &ImPanelWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FExpandableBoxStyleReflectionTypeDesc = FExpandableBoxStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImExpandableBoxReflectionTypeDesc = ImExpandableBox::StaticTypeDesc();

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
    m_bHasExplicitStyle = true;
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
    const FExpandableBoxStyle& style = GetEffectiveStyle();

    const FVector2 containerMin = m_Geometry.GetMin();
    const FVector2 containerMax = containerMin + FVector2(m_Geometry.Size.X, ComputeVisibleHeight());
    const float cornerRadius = std::max(0.0f, style.CornerRadius);

    if (m_bExpanded && m_BodyWidget) {
        paintContext.DrawContext_.DrawRectFilled(
            containerMin,
            containerMax,
            style.BodyBackgroundColor,
            cornerRadius);
    }

    const FColor headerColor = m_bPressed
        ? style.HeaderPressedBackgroundColor
        : (m_bHovered ? style.HeaderHoveredBackgroundColor : style.HeaderBackgroundColor);

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
        const float halfSize = style.IndicatorSize * 0.5f;
        const FColor indicatorColor = m_bHovered ? style.IndicatorHoveredColor : style.IndicatorColor;

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
        HasKeyboardFocus() ? style.FocusedOutlineColor : style.BorderColor,
        cornerRadius,
        style.BorderThickness);

    RenderChildren(paintContext);
}

FVector2 ImExpandableBox::GetMinSize() const
{
    const FExpandableBoxStyle& style = GetEffectiveStyle();
    const float borderInset = std::max(0.0f, style.BorderThickness);
    const float totalBorderWidth = borderInset * 2.0f;
    const float indicatorStripWidth = style.IndicatorSize + style.IndicatorSpacing;
    const FVector2 headerMin = m_HeaderWidget ? m_HeaderWidget->GetMinSize() : FVector2(0.0f, 0.0f);
    const float headerWidth =
        totalBorderWidth +
        style.HeaderPadding.Left + indicatorStripWidth + headerMin.X + style.HeaderPadding.Right;
    const float headerHeight =
        totalBorderWidth +
        style.HeaderPadding.Top +
        std::max(style.IndicatorSize, headerMin.Y) +
        style.HeaderPadding.Bottom;

    float minWidth = std::max(style.MinDesiredSize.X, headerWidth);
    float minHeight = std::max(style.MinDesiredSize.Y, headerHeight);

    if (m_bExpanded && m_BodyWidget) {
        const FVector2 bodyMin = m_BodyWidget->GetMinSize();
        minWidth = std::max(
            minWidth,
            totalBorderWidth + style.BodyPadding.Left + bodyMin.X + style.BodyPadding.Right);
        minHeight += style.BodyPadding.Top + bodyMin.Y + style.BodyPadding.Bottom;
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
    const FExpandableBoxStyle& style = GetEffectiveStyle();
    const FGeometry innerGeometry = InsetGeometryByBorder(m_Geometry, style.BorderThickness);
    const float headerHeight = ComputeHeaderHeight();
    const float bodyHeight = ComputeBodyHeight();
    const float visibleHeight = m_bExpanded ? headerHeight + bodyHeight : headerHeight;
    const FVector2 visibleSize(innerGeometry.Size.X, visibleHeight);

    m_HeaderGeometry = FGeometry(innerGeometry.Position, FVector2(visibleSize.X, headerHeight));
    m_BodyGeometry = FGeometry(
        FVector2(innerGeometry.Position.X, innerGeometry.Position.Y + headerHeight),
        FVector2(visibleSize.X, bodyHeight));

    const float indicatorY = m_HeaderGeometry.Position.Y +
        std::max(0.0f, (headerHeight - style.IndicatorSize) * 0.5f);
    m_IndicatorHotspotGeometry = FGeometry(
        FVector2(m_HeaderGeometry.Position.X + style.HeaderPadding.Left, indicatorY),
        FVector2(style.IndicatorSize, style.IndicatorSize));

    const int headerSlotIndex = m_HeaderWidget ? 0 : -1;
    const int bodySlotIndex = m_bExpanded ? (m_HeaderWidget ? 1 : 0) : -1;

    ImPaddingSlot* headerSlot =
        headerSlotIndex >= 0 ? dynamic_cast<ImPaddingSlot*>(GetSlotAt(headerSlotIndex)) : nullptr;
    if (headerSlot && m_HeaderWidget) {
        headerSlot->PaddingLeft = style.HeaderPadding.Left + style.IndicatorSize + style.IndicatorSpacing;
        headerSlot->PaddingRight = style.HeaderPadding.Right;
        headerSlot->PaddingTop = style.HeaderPadding.Top;
        headerSlot->PaddingBottom = style.HeaderPadding.Bottom;
        headerSlot->SetSlotPosition(m_HeaderGeometry.Position);
        headerSlot->SetSlotSize(m_HeaderGeometry.Size);
        headerSlot->ApplyLayout(m_HeaderWidget.get());
    }

    ImPaddingSlot* bodySlot =
        bodySlotIndex >= 0 ? dynamic_cast<ImPaddingSlot*>(GetSlotAt(bodySlotIndex)) : nullptr;
    if (bodySlot && m_BodyWidget && m_bExpanded) {
        bodySlot->PaddingLeft = style.BodyPadding.Left;
        bodySlot->PaddingRight = style.BodyPadding.Right;
        bodySlot->PaddingTop = style.BodyPadding.Top;
        bodySlot->PaddingBottom = style.BodyPadding.Bottom;
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

const FExpandableBoxStyle& ImExpandableBox::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveExpandableBoxStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

void ImExpandableBox::PostDeserializeFromJson()
{
    m_bHasExplicitStyle = true;
}

float ImExpandableBox::ComputeHeaderHeight() const
{
    const FExpandableBoxStyle& style = GetEffectiveStyle();
    const FVector2 headerMin = m_HeaderWidget ? m_HeaderWidget->GetMinSize() : FVector2(0.0f, 0.0f);
    return style.HeaderPadding.Top +
        std::max(style.IndicatorSize, headerMin.Y) +
        style.HeaderPadding.Bottom;
}

float ImExpandableBox::ComputeBodyHeight() const
{
    if (!m_bExpanded || !m_BodyWidget) {
        return 0.0f;
    }

    const FVector2 bodyMin = m_BodyWidget->GetMinSize();
    const FExpandableBoxStyle& style = GetEffectiveStyle();
    return style.BodyPadding.Top + bodyMin.Y + style.BodyPadding.Bottom;
}

float ImExpandableBox::ComputeVisibleHeight() const
{
    return m_bExpanded ? (ComputeHeaderHeight() + ComputeBodyHeight()) : ComputeHeaderHeight();
}

} // namespace ImWidgetV4
