#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

constexpr int InvalidPopupMenuIndex = -1;

} // namespace

ImPopupMenu::ImPopupMenu()
    : ImWidget()
{
    SetHitTestVisible(true);
}

void ImPopupMenu::SetItems(const std::vector<FPopupMenuItem>& items)
{
    Items_ = items;
    ClearInteractionState();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::SetItems(std::vector<FPopupMenuItem>&& items)
{
    Items_ = std::move(items);
    ClearInteractionState();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::SetStyle(const FPopupMenuStyle& style)
{
    Style_ = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::Paint(const FPaintContext& paintContext)
{
    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        Style_.BackgroundColor,
        Style_.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        Style_.BorderColor,
        Style_.CornerRadius,
        Style_.BorderThickness);

    paintContext.DrawContext_.PushClipRect(m_Geometry.GetMin(), m_Geometry.GetMax(), true);

    float rowY = m_Geometry.Position.Y + Style_.OuterPaddingY;
    const float rowWidth = std::max(0.0f, m_Geometry.Size.X - Style_.OuterPaddingX * 2.0f);
    for (std::size_t index = 0; index < Items_.size(); ++index) {
        const FPopupMenuItem& item = Items_[index];
        if (item.bIsSeparator) {
            const float separatorY = rowY + Style_.RowHeight * 0.5f;
            paintContext.DrawContext_.DrawLine(
                FVector2(m_Geometry.Position.X + Style_.OuterPaddingX + Style_.HorizontalPadding, separatorY),
                FVector2(m_Geometry.Position.X + m_Geometry.Size.X - Style_.OuterPaddingX - Style_.HorizontalPadding, separatorY),
                Style_.SeparatorColor,
                1.0f);
            rowY += Style_.RowHeight;
            continue;
        }

        const FGeometry rowGeometry(
            FVector2(m_Geometry.Position.X + Style_.OuterPaddingX, rowY),
            FVector2(rowWidth, Style_.RowHeight));
        const bool bHovered = static_cast<int>(index) == HoveredItemIndex_;
        const bool bPressed = static_cast<int>(index) == PressedItemIndex_;
        if (!item.bEnabled) {
            // Disabled rows remain visible but do not show hover or pressed fills.
        } else if (bPressed) {
            paintContext.DrawContext_.DrawRectFilled(
                rowGeometry.GetMin(),
                rowGeometry.GetMax(),
                Style_.RowPressedColor,
                6.0f);
        } else if (bHovered) {
            paintContext.DrawContext_.DrawRectFilled(
                rowGeometry.GetMin(),
                rowGeometry.GetMax(),
                Style_.RowHoveredColor,
                6.0f);
        }

        float contentX = rowGeometry.Position.X + Style_.HorizontalPadding;
        if (item.Icon.IsValid()) {
            paintContext.DrawContext_.DrawImage(
                item.Icon.TextureId,
                FVector2(contentX, rowGeometry.Position.Y + (Style_.RowHeight - Style_.IconSize) * 0.5f),
                FVector2(contentX + Style_.IconSize, rowGeometry.Position.Y + (Style_.RowHeight + Style_.IconSize) * 0.5f),
                item.Icon.Uv0,
                item.Icon.Uv1,
                item.bEnabled ? item.Icon.TintColor : Style_.DisabledTextColor);
            contentX += Style_.IconSize + Style_.IconTextSpacing;
        }

        paintContext.DrawContext_.DrawText(
            FVector2(
                contentX,
                rowGeometry.Position.Y + std::max(0.0f, (Style_.RowHeight - Style_.FontSize) * 0.5f)),
            item.bEnabled ? Style_.TextColor : Style_.DisabledTextColor,
            item.Text,
            Style_.FontSize);
        rowY += Style_.RowHeight;
    }

    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImPopupMenu::GetMinSize() const
{
    float maxTextWidth = 0.0f;
    bool bHasAnyIcon = false;
    for (const FPopupMenuItem& item : Items_) {
        if (item.bIsSeparator) {
            continue;
        }

        maxTextWidth = std::max(maxTextWidth, MeasureTextWidth(item.Text));
        bHasAnyIcon = bHasAnyIcon || item.Icon.IsValid();
    }

    const float width =
        Style_.OuterPaddingX * 2.0f +
        Style_.HorizontalPadding * 2.0f +
        maxTextWidth +
        (bHasAnyIcon ? (Style_.IconSize + Style_.IconTextSpacing) : 0.0f);
    const float height = Style_.OuterPaddingY * 2.0f + static_cast<float>(Items_.size()) * Style_.RowHeight;

    return FVector2(
        std::max(Style_.MinDesiredSize.X, width),
        std::max(Style_.MinDesiredSize.Y, height));
}

FReply ImPopupMenu::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::MouseMove) {
        const int hoveredIndex = ResolveIndexAt(event.MousePosition);
        if (HoveredItemIndex_ != hoveredIndex) {
            HoveredItemIndex_ = hoveredIndex;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        if (HoveredItemIndex_ != InvalidPopupMenuIndex || PressedItemIndex_ != InvalidPopupMenuIndex) {
            ClearInteractionState();
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonDown && event.MouseButton == EMouseButton::Left) {
        const int pressedIndex = ResolveIndexAt(event.MousePosition);
        if (!IsInteractiveIndex(pressedIndex)) {
            return FReply::Unhandled();
        }

        PressedItemIndex_ = pressedIndex;
        HoveredItemIndex_ = pressedIndex;
        Invalidate(EInvalidateReason::Paint);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonUp && event.MouseButton == EMouseButton::Left) {
        const int releasedIndex = ResolveIndexAt(event.MousePosition);
        const int pressedIndex = PressedItemIndex_;
        PressedItemIndex_ = InvalidPopupMenuIndex;
        HoveredItemIndex_ = releasedIndex;
        Invalidate(EInvalidateReason::Paint);

        if (!IsInteractiveIndex(pressedIndex) || pressedIndex != releasedIndex) {
            return FReply::Handled();
        }

        FPopupMenuItem& item = Items_[static_cast<std::size_t>(pressedIndex)];
        if (item.OnInvoked) {
            item.OnInvoked();
        }
        OnItemInvoked.Broadcast(*this, pressedIndex);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

int ImPopupMenu::ResolveIndexAt(const FVector2& position) const
{
    if (!m_Geometry.Contains(position) || Items_.empty()) {
        return InvalidPopupMenuIndex;
    }

    const float localY = position.Y - m_Geometry.Position.Y - Style_.OuterPaddingY;
    const int index = static_cast<int>(localY / Style_.RowHeight);
    if (index < 0 || index >= static_cast<int>(Items_.size())) {
        return InvalidPopupMenuIndex;
    }

    return index;
}

bool ImPopupMenu::IsInteractiveIndex(int index) const
{
    return index >= 0 &&
        index < static_cast<int>(Items_.size()) &&
        !Items_[static_cast<std::size_t>(index)].bIsSeparator &&
        Items_[static_cast<std::size_t>(index)].bEnabled;
}

float ImPopupMenu::MeasureTextWidth(const std::string& text) const
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(Style_.FontSize, FLT_MAX, 0.0f, text.c_str()).x;
    }

    return Style_.FontSize * 0.55f * static_cast<float>(text.size());
}

void ImPopupMenu::ClearInteractionState()
{
    HoveredItemIndex_ = InvalidPopupMenuIndex;
    PressedItemIndex_ = InvalidPopupMenuIndex;
}

} // namespace ImWidgetV4
