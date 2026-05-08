#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/WindowManager.h>
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

ImPopupMenu::~ImPopupMenu()
{
    ActiveChildWindow_.reset();
    ActiveChildMenu_.reset();
    ActiveSubMenuIndex_ = InvalidPopupMenuIndex;
}

void ImPopupMenu::SetItems(const std::vector<FPopupMenuItem>& items)
{
    CloseChildSubmenuChain();
    Items_ = items;
    ClearInteractionState();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::SetItems(std::vector<FPopupMenuItem>&& items)
{
    CloseChildSubmenuChain();
    Items_ = std::move(items);
    ClearInteractionState();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::SetStyle(const FPopupMenuStyle& style)
{
    Style_ = style;
    if (ActiveChildMenu_) {
        ActiveChildMenu_->SetStyle(style);
    }
    if (ActiveChildWindow_ && ActiveChildWindow_->IsOpen() && ActiveChildMenu_) {
        ActiveChildWindow_->SetSize(ActiveChildMenu_->GetMinSize());
    }
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImPopupMenu::Paint(const FPaintContext& paintContext)
{
    SyncChildSubmenuState();

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

    const float submenuIndicatorWidth = ResolveSubmenuIndicatorWidth();
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
            ImTextureID textureId = item.Icon.TextureId;
            if (GetApplication() != nullptr) {
                textureId = GetApplication()->ResolveTextureForPaint(textureId);
            }
            if (textureId != nullptr) {
                paintContext.DrawContext_.DrawImage(
                    textureId,
                    FVector2(contentX, rowGeometry.Position.Y + (Style_.RowHeight - Style_.IconSize) * 0.5f),
                    FVector2(contentX + Style_.IconSize, rowGeometry.Position.Y + (Style_.RowHeight + Style_.IconSize) * 0.5f),
                    item.Icon.Uv0,
                    item.Icon.Uv1,
                    item.bEnabled ? item.Icon.TintColor : Style_.DisabledTextColor);
                contentX += Style_.IconSize + Style_.IconTextSpacing;
            }
        }

        const float rightReserve = item.HasSubMenu() ? (submenuIndicatorWidth + Style_.SubmenuIndicatorSpacing) : 0.0f;
        const FVector2 textClipMin(contentX, rowGeometry.Position.Y);
        const FVector2 textClipMax(
            rowGeometry.Position.X + rowGeometry.Size.X - Style_.HorizontalPadding - rightReserve,
            rowGeometry.Position.Y + rowGeometry.Size.Y);
        paintContext.DrawContext_.PushClipRect(textClipMin, textClipMax, true);
        paintContext.DrawContext_.DrawText(
            FVector2(
                contentX,
                rowGeometry.Position.Y + std::max(0.0f, (Style_.RowHeight - Style_.FontSize) * 0.5f)),
            item.bEnabled ? Style_.TextColor : Style_.DisabledTextColor,
            item.Text,
            Style_.FontSize);
        paintContext.DrawContext_.PopClipRect();

        if (item.HasSubMenu()) {
            const FColor arrowColor = item.bEnabled ? Style_.SubmenuArrowColor : Style_.DisabledTextColor;
            const float arrowWidth = submenuIndicatorWidth;
            const float arrowHeight = std::max(6.0f, Style_.FontSize * 0.5f);
            const float arrowCenterX =
                rowGeometry.Position.X + rowGeometry.Size.X - Style_.HorizontalPadding - arrowWidth * 0.5f;
            const float arrowCenterY = rowGeometry.Position.Y + rowGeometry.Size.Y * 0.5f;
            paintContext.DrawContext_.PathLineTo(FVector2(arrowCenterX - arrowWidth * 0.35f, arrowCenterY - arrowHeight * 0.5f));
            paintContext.DrawContext_.PathLineTo(FVector2(arrowCenterX - arrowWidth * 0.35f, arrowCenterY + arrowHeight * 0.5f));
            paintContext.DrawContext_.PathLineTo(FVector2(arrowCenterX + arrowWidth * 0.35f, arrowCenterY));
            paintContext.DrawContext_.PathFill(arrowColor);
        }

        rowY += Style_.RowHeight;
    }

    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImPopupMenu::GetMinSize() const
{
    const FMenuMetrics metrics = ComputeMetrics();

    const float width =
        Style_.OuterPaddingX * 2.0f +
        Style_.HorizontalPadding * 2.0f +
        metrics.MaxTextWidth +
        (metrics.bHasAnyIcon ? (Style_.IconSize + Style_.IconTextSpacing) : 0.0f) +
        (metrics.bHasAnySubMenu ? (ResolveSubmenuIndicatorWidth() + Style_.SubmenuIndicatorSpacing) : 0.0f);
    const float height = Style_.OuterPaddingY * 2.0f + static_cast<float>(Items_.size()) * Style_.RowHeight;

    return FVector2(
        std::max(Style_.MinDesiredSize.X, width),
        std::max(Style_.MinDesiredSize.Y, height));
}

FReply ImPopupMenu::OnInputEvent(const FInputEvent& event)
{
    SyncChildSubmenuState();

    if (event.Type == EInputEventType::MouseMove) {
        const int hoveredIndex = ResolveIndexAt(event.MousePosition);
        if (HoveredItemIndex_ != hoveredIndex) {
            HoveredItemIndex_ = hoveredIndex;
            if (HasSubMenuAt(hoveredIndex) && IsInteractiveIndex(hoveredIndex)) {
                OpenChildSubmenu(hoveredIndex);
            } else if (ActiveSubMenuIndex_ != hoveredIndex) {
                CloseChildSubmenuChain();
            }
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
            if (ActiveSubMenuIndex_ != InvalidPopupMenuIndex) {
                CloseChildSubmenuChain();
            }
            return FReply::Unhandled();
        }

        PressedItemIndex_ = pressedIndex;
        HoveredItemIndex_ = pressedIndex;
        if (HasSubMenuAt(pressedIndex)) {
            OpenChildSubmenu(pressedIndex);
        } else if (ActiveSubMenuIndex_ != InvalidPopupMenuIndex) {
            CloseChildSubmenuChain();
        }
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
        if (item.HasSubMenu()) {
            OpenChildSubmenu(pressedIndex);
            return FReply::Handled();
        }

        if (item.OnInvoked) {
            item.OnInvoked();
        }
        OnItemInvoked.Broadcast(*this, pressedIndex);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FGeometry ImPopupMenu::GetRowGeometry(int index) const
{
    if (index < 0 || index >= static_cast<int>(Items_.size())) {
        return FGeometry();
    }

    return FGeometry(
        FVector2(
            m_Geometry.Position.X + Style_.OuterPaddingX,
            m_Geometry.Position.Y + Style_.OuterPaddingY + Style_.RowHeight * static_cast<float>(index)),
        FVector2(
            std::max(0.0f, m_Geometry.Size.X - Style_.OuterPaddingX * 2.0f),
            Style_.RowHeight));
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

bool ImPopupMenu::HasSubMenuAt(int index) const
{
    return index >= 0 &&
        index < static_cast<int>(Items_.size()) &&
        Items_[static_cast<std::size_t>(index)].HasSubMenu();
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

float ImPopupMenu::ResolveSubmenuIndicatorWidth() const
{
    return std::max(8.0f, Style_.FontSize * 0.55f);
}

ImPopupMenu::FMenuMetrics ImPopupMenu::ComputeMetrics() const
{
    FMenuMetrics metrics;
    for (const FPopupMenuItem& item : Items_) {
        if (item.bIsSeparator) {
            continue;
        }

        metrics.MaxTextWidth = std::max(metrics.MaxTextWidth, MeasureTextWidth(item.Text));
        metrics.bHasAnyIcon = metrics.bHasAnyIcon || item.Icon.IsValid();
        metrics.bHasAnySubMenu = metrics.bHasAnySubMenu || item.HasSubMenu();
    }

    return metrics;
}

void ImPopupMenu::SyncChildSubmenuState()
{
    if (ActiveChildWindow_ && !ActiveChildWindow_->IsOpen()) {
        ActiveChildWindow_.reset();
        ActiveChildMenu_.reset();
        ActiveSubMenuIndex_ = InvalidPopupMenuIndex;
        return;
    }

    if (ActiveChildWindow_ && ActiveChildMenu_ && ActiveSubMenuIndex_ != InvalidPopupMenuIndex) {
        const FGeometry rowGeometry = GetRowGeometry(ActiveSubMenuIndex_);
        ActiveChildWindow_->SetPosition(FVector2(rowGeometry.Position.X + rowGeometry.Size.X, rowGeometry.Position.Y));
        ActiveChildWindow_->SetSize(ActiveChildMenu_->GetMinSize());
    }
}

void ImPopupMenu::OpenChildSubmenu(int index)
{
    if (!HasSubMenuAt(index) || !IsInteractiveIndex(index) || GetApplication() == nullptr) {
        return;
    }

    SyncChildSubmenuState();
    if (ActiveSubMenuIndex_ == index && ActiveChildWindow_ && ActiveChildWindow_->IsOpen()) {
        return;
    }

    CloseChildSubmenuChain();

    std::shared_ptr<ImPopupMenu> childMenu = std::make_shared<ImPopupMenu>();
    childMenu->SetStyle(Style_);
    childMenu->SetItems(Items_[static_cast<std::size_t>(index)].SubItems);
    childMenu->ParentMenu_ = std::static_pointer_cast<ImPopupMenu>(shared_from_this());

    std::weak_ptr<ImPopupMenu> weakThis = std::static_pointer_cast<ImPopupMenu>(shared_from_this());
    childMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu& sender, int invokedIndex) {
        if (const std::shared_ptr<ImPopupMenu> self = weakThis.lock()) {
            self->RelayDescendantInvocation(sender, invokedIndex);
        }
    });

    ImWindowManager& windowManager = GetApplication()->GetWindowManager();
    const std::shared_ptr<ImWindow> parentWindow = windowManager.FindWindowForWidget(
        std::static_pointer_cast<ImWidget>(shared_from_this()));
    if (!parentWindow) {
        return;
    }

    const FGeometry rowGeometry = GetRowGeometry(index);
    const FVector2 childSize = childMenu->GetMinSize();

    FPopupOptions popupOptions;
    popupOptions.Title = "PopupSubMenu";
    popupOptions.Position = FVector2(rowGeometry.Position.X + rowGeometry.Size.X, rowGeometry.Position.Y);
    popupOptions.Size = childSize;
    popupOptions.RootWidget = childMenu;
    popupOptions.ParentWindow = parentWindow;
    popupOptions.Style.BackgroundColor = Style_.BackgroundColor;
    popupOptions.Style.InactiveBackgroundColor = Style_.BackgroundColor;
    popupOptions.Style.BorderColor = Style_.BorderColor;
    popupOptions.Style.ActiveBorderColor = Style_.BorderColor;
    popupOptions.Style.CornerRadius = Style_.CornerRadius;
    popupOptions.Style.BorderThickness = Style_.BorderThickness;
    popupOptions.Style.bDrawShadow = true;
    popupOptions.Style.ShadowColor = FColor(0.0f, 0.0f, 0.0f, 0.18f);
    popupOptions.Style.ShadowOffset = FVector2(0.0f, 10.0f);

    ActiveChildMenu_ = childMenu;
    ActiveChildWindow_ = windowManager.CreatePopup(popupOptions);
    ActiveSubMenuIndex_ = index;
}

void ImPopupMenu::CloseChildSubmenuChain()
{
    if (ActiveChildWindow_) {
        if (ImApplication* application = GetApplication()) {
            application->GetWindowManager().CloseWindow(ActiveChildWindow_);
        } else if (ActiveChildWindow_->IsOpen()) {
            ActiveChildWindow_->Close();
        }
    }

    ActiveChildWindow_.reset();
    ActiveChildMenu_.reset();
    ActiveSubMenuIndex_ = InvalidPopupMenuIndex;
}

void ImPopupMenu::RelayDescendantInvocation(ImPopupMenu& sender, int index)
{
    OnItemInvoked.Broadcast(sender, index);
}

void ImPopupMenu::ClearInteractionState()
{
    HoveredItemIndex_ = InvalidPopupMenuIndex;
    PressedItemIndex_ = InvalidPopupMenuIndex;
}

} // namespace ImWidgetV4
