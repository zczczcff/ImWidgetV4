#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Button.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

constexpr float BorderThickness = 1.0f;

float ClampNonNegative(float value)
{
    return std::max(0.0f, value);
}

bool NearlyEqual(float left, float right)
{
    return std::fabs(left - right) <= 0.01f;
}

void ClearHoverStateInSubtree(const std::shared_ptr<ImWidget>& widget, const FVector2& mousePosition)
{
    if (!widget) {
        return;
    }

    if (std::dynamic_pointer_cast<ImButton>(widget)) {
        FInputEvent leaveEvent;
        leaveEvent.Type = EInputEventType::MouseLeave;
        leaveEvent.MousePosition = mousePosition;
        widget->OnInputEvent(leaveEvent);
    }

    for (const std::shared_ptr<ImWidget>& child : widget->GetChildren()) {
        ClearHoverStateInSubtree(child, mousePosition);
    }
}

FGeometry InsetGeometry(const FGeometry& geometry, const FMargin& padding)
{
    const float left = padding.Left;
    const float top = padding.Top;
    const float right = padding.Right;
    const float bottom = padding.Bottom;
    return FGeometry(
        FVector2(geometry.Position.X + left, geometry.Position.Y + top),
        FVector2(
            std::max(0.0f, geometry.Size.X - left - right),
            std::max(0.0f, geometry.Size.Y - top - bottom)));
}

} // namespace

ImTitleBar::ImTitleBar()
    : ImWidget()
{
    SetHitTestVisible(true);
}

void ImTitleBar::AddLeadingItem(const std::shared_ptr<ImWidget>& widget)
{
    AttachItem(LeadingItems_, widget);
}

void ImTitleBar::AddTrailingItem(const std::shared_ptr<ImWidget>& widget)
{
    AttachItem(TrailingItems_, widget);
}

void ImTitleBar::ClearLeadingItems()
{
    DetachItems(LeadingItems_);
    MarkLayoutDirty();
}

void ImTitleBar::ClearTrailingItems()
{
    DetachItems(TrailingItems_);
    MarkLayoutDirty();
}

void ImTitleBar::SetShowSystemButtons(bool value)
{
    if (bShowSystemButtons_ == value) {
        return;
    }

    bShowSystemButtons_ = value;
    MarkLayoutDirty();
}

void ImTitleBar::SetShowMinimizeButton(bool value)
{
    if (bShowMinimizeButton_ == value) {
        return;
    }

    bShowMinimizeButton_ = value;
    MarkLayoutDirty();
}

void ImTitleBar::SetShowMaximizeButton(bool value)
{
    if (bShowMaximizeButton_ == value) {
        return;
    }

    bShowMaximizeButton_ = value;
    MarkLayoutDirty();
}

void ImTitleBar::SetShowCloseButton(bool value)
{
    if (bShowCloseButton_ == value) {
        return;
    }

    bShowCloseButton_ = value;
    MarkLayoutDirty();
}

void ImTitleBar::SetDragRegionMinWidth(float width)
{
    const float clampedWidth = ClampNonNegative(width);
    if (NearlyEqual(ReflectedDragRegionMinWidth_, clampedWidth)) {
        return;
    }

    ReflectedDragRegionMinWidth_ = clampedWidth;
    MarkLayoutDirty();
}

void ImTitleBar::SetStyle(const FTitleBarStyle& style)
{
    Style_ = style;
    bHasExplicitStyle_ = true;
    MarkLayoutDirty();
}

void ImTitleBar::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    const FTitleBarStyle& style = GetEffectiveStyle();
    const FVector2 min = m_Geometry.GetMin();
    const FVector2 max = m_Geometry.GetMax();
    paintContext.DrawContext_.DrawRectFilled(min, max, style.BackgroundColor);
    paintContext.DrawContext_.DrawLine(
        FVector2(min.X, max.Y - BorderThickness),
        FVector2(max.X, max.Y - BorderThickness),
        style.BorderColor,
        BorderThickness);

    PaintChildren(paintContext);
    PaintSystemButtons(paintContext);
}

FVector2 ImTitleBar::GetMinSize() const
{
    const FTitleBarStyle& style = GetEffectiveStyle();
    float width = style.Padding.Left + style.Padding.Right + GetResolvedDragRegionMinWidth();
    float contentHeight = 0.0f;

    const auto accumulateItems = [&](const std::vector<Ptr>& items) {
        float localWidth = 0.0f;
        float localHeight = 0.0f;
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (!items[index]) {
                continue;
            }

            const FVector2 itemMinSize = items[index]->GetMinSize();
            localWidth += itemMinSize.X;
            localHeight = std::max(localHeight, itemMinSize.Y);
            if (index + 1 < items.size()) {
                localWidth += style.ItemSpacing;
            }
        }

        width += localWidth;
        contentHeight = std::max(contentHeight, localHeight);
    };

    accumulateItems(LeadingItems_);
    if (!LeadingItems_.empty() && !TrailingItems_.empty()) {
        width += style.ItemSpacing;
    }
    accumulateItems(TrailingItems_);

    if (HasSystemButtons()) {
        std::size_t visibleButtonCount = 0;
        for (ESystemButton button : {ESystemButton::Minimize, ESystemButton::Maximize, ESystemButton::Close}) {
            visibleButtonCount += IsSystemButtonVisible(button) ? 1U : 0U;
        }

        if (visibleButtonCount > 0) {
            width += style.SystemButtonSize * static_cast<float>(visibleButtonCount);
            width += style.SystemButtonSpacing * static_cast<float>(visibleButtonCount - 1U);
        }
    }

    const float height = std::max(
        std::max(style.Height, style.MinDesiredSize.Y),
        contentHeight + style.Padding.Top + style.Padding.Bottom);
    width = std::max(width, style.MinDesiredSize.X);
    return FVector2(width, height);
}

FReply ImTitleBar::OnPreviewInputEvent(const FInputEvent& event)
{
    (void)event;
    return FReply::Unhandled();
}

FReply ImTitleBar::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    if (event.Type == EInputEventType::MouseEnter || event.Type == EInputEventType::MouseMove) {
        UpdateHoveredState(event.MousePosition);
        return PressedSystemButton_ != ESystemButton::None
            ? FReply::Handled()
            : FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        if (GetApplication() == nullptr || GetApplication()->GetMouseCapture().get() != this) {
            ClearHoveredState();
        }
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        const ESystemButton systemButton = HitTestSystemButton(event.MousePosition);
        if (systemButton != ESystemButton::None) {
            PressedSystemButton_ = systemButton;
            HoveredSystemButton_ = systemButton;
            UpdateToolTipForHoveredState();
            Invalidate(EInvalidateReason::Paint);
            return FReply::Handled().CaptureMouse(shared_from_this(), EMouseButton::Left);
        }

        if (IsPointInHostDragArea(event.MousePosition)) {
            for (const FChildLayout& layout : LeadingLayouts_) {
                ClearHoverStateInSubtree(layout.Widget, event.MousePosition);
            }
            for (const FChildLayout& layout : TrailingLayouts_) {
                ClearHoverStateInSubtree(layout.Widget, event.MousePosition);
            }

            if (ImApplication* application = GetApplication()) {
                application->ClearKeyboardFocus();
            }

            ImApplicationBackend* backend = GetBackend();
            if (backend == nullptr || !backend->SupportsHostWindowDrag()) {
                return FReply::Handled();
            }

            backend->BeginHostWindowDrag();
            return FReply::Handled();
        }
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        PressedSystemButton_ != ESystemButton::None) {
        const ESystemButton pressedButton = PressedSystemButton_;
        PressedSystemButton_ = ESystemButton::None;
        const bool bShouldInvoke = HitTestSystemButton(event.MousePosition) == pressedButton;
        if (bShouldInvoke) {
            HandleSystemButtonClick(pressedButton);
        }
        UpdateHoveredState(event.MousePosition);
        Invalidate(EInvalidateReason::Paint);
        return FReply::Handled().ReleaseMouseCapture();
    }

    return FReply::Unhandled();
}

bool ImTitleBar::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());
    if (HitTestsChildWidgets(position, &outPath)) {
        return true;
    }

    return true;
}

void ImTitleBar::Relayout() const
{
    const bool bGeometryChanged = !bHasLastLayoutGeometry_ ||
        !NearlyEqual(m_Geometry.Position.X, LastLayoutGeometry_.Position.X) ||
        !NearlyEqual(m_Geometry.Position.Y, LastLayoutGeometry_.Position.Y) ||
        !NearlyEqual(m_Geometry.Size.X, LastLayoutGeometry_.Size.X) ||
        !NearlyEqual(m_Geometry.Size.Y, LastLayoutGeometry_.Size.Y);

    if (!bLayoutDirty_ && !bGeometryChanged) {
        return;
    }

    LeadingLayouts_.clear();
    TrailingLayouts_.clear();
    DragRegionGeometry_ = FGeometry();
    MinimizeButtonGeometry_ = FGeometry();
    MaximizeButtonGeometry_ = FGeometry();
    CloseButtonGeometry_ = FGeometry();

    const FTitleBarStyle& style = GetEffectiveStyle();
    const FGeometry innerGeometry = InsetGeometry(m_Geometry, style.Padding);
    const float innerHeight = innerGeometry.Size.Y;
    const float buttonSize = std::min(ClampNonNegative(style.SystemButtonSize), innerHeight);
    const float systemButtonSpacing = ClampNonNegative(style.SystemButtonSpacing);

    auto layoutItemsLeftToRight = [&](const std::vector<Ptr>& items, float startX, std::vector<FChildLayout>& outLayouts) {
        float cursorX = startX;
        for (std::size_t index = 0; index < items.size(); ++index) {
            const Ptr& item = items[index];
            if (!item) {
                continue;
            }

            const FVector2 itemMinSize = item->GetMinSize();
            const float itemWidth = ClampNonNegative(itemMinSize.X);
            const float itemHeight = innerHeight;
            const float itemY = innerGeometry.Position.Y;

            FChildLayout layout;
            layout.Widget = item;
            layout.Geometry = FGeometry(FVector2(cursorX, itemY), FVector2(itemWidth, itemHeight));
            outLayouts.push_back(layout);
            cursorX += itemWidth;
            if (index + 1 < items.size()) {
                cursorX += style.ItemSpacing;
            }
        }

        return cursorX;
    };

    auto measureItemsWidth = [&](const std::vector<Ptr>& items) {
        float width = 0.0f;
        bool bHasAny = false;
        for (const Ptr& item : items) {
            if (!item) {
                continue;
            }

            if (bHasAny) {
                width += style.ItemSpacing;
            }
            width += ClampNonNegative(item->GetMinSize().X);
            bHasAny = true;
        }
        return width;
    };

    std::vector<ESystemButton> visibleButtons;
    for (ESystemButton button : {ESystemButton::Minimize, ESystemButton::Maximize, ESystemButton::Close}) {
        if (IsSystemButtonVisible(button)) {
            visibleButtons.push_back(button);
        }
    }

    const float systemButtonsWidth = visibleButtons.empty()
        ? 0.0f
        : buttonSize * static_cast<float>(visibleButtons.size()) +
            systemButtonSpacing * static_cast<float>(visibleButtons.size() - 1U);

    const float trailingWidth = measureItemsWidth(TrailingItems_);
    float leadingEndX = layoutItemsLeftToRight(LeadingItems_, innerGeometry.Position.X, LeadingLayouts_);
    if (!LeadingLayouts_.empty()) {
        leadingEndX = LeadingLayouts_.back().Geometry.Position.X + LeadingLayouts_.back().Geometry.Size.X;
    }

    float trailingStartX = innerGeometry.Position.X + innerGeometry.Size.X - systemButtonsWidth - trailingWidth;
    if (!TrailingItems_.empty()) {
        float cursorX = trailingStartX;
        for (std::size_t index = 0; index < TrailingItems_.size(); ++index) {
            const Ptr& item = TrailingItems_[index];
            if (!item) {
                continue;
            }

            const FVector2 itemMinSize = item->GetMinSize();
            const float itemWidth = ClampNonNegative(itemMinSize.X);
            const float itemHeight = innerHeight;
            const float itemY = innerGeometry.Position.Y;
            FChildLayout layout;
            layout.Widget = item;
            layout.Geometry = FGeometry(FVector2(cursorX, itemY), FVector2(itemWidth, itemHeight));
            TrailingLayouts_.push_back(layout);
            cursorX += itemWidth;
            if (index + 1 < TrailingItems_.size()) {
                cursorX += style.ItemSpacing;
            }
        }
    }

    const float dragStartX = leadingEndX;
    const float dragEndX = trailingStartX;
    DragRegionGeometry_ = FGeometry(
        FVector2(dragStartX, innerGeometry.Position.Y),
        FVector2(std::max(0.0f, dragEndX - dragStartX), innerHeight));

    float buttonX = innerGeometry.Position.X + innerGeometry.Size.X - systemButtonsWidth;
    const float buttonY = innerGeometry.Position.Y + std::max(0.0f, (innerHeight - buttonSize) * 0.5f);
    for (std::size_t index = 0; index < visibleButtons.size(); ++index) {
        const FGeometry buttonGeometry(
            FVector2(buttonX, buttonY),
            FVector2(buttonSize, buttonSize));
        switch (visibleButtons[index]) {
        case ESystemButton::Minimize:
            MinimizeButtonGeometry_ = buttonGeometry;
            break;
        case ESystemButton::Maximize:
            MaximizeButtonGeometry_ = buttonGeometry;
            break;
        case ESystemButton::Close:
            CloseButtonGeometry_ = buttonGeometry;
            break;
        default:
            break;
        }
        buttonX += buttonSize + systemButtonSpacing;
    }

    SyncChildGeometries();
    bLayoutDirty_ = false;
    LastLayoutGeometry_ = m_Geometry;
    bHasLastLayoutGeometry_ = true;
}

void ImTitleBar::DetachItems(std::vector<Ptr>& items)
{
    for (const Ptr& item : items) {
        if (item) {
            ImWidget::RemoveChild(item);
        }
    }
    items.clear();
}

void ImTitleBar::AttachItem(std::vector<Ptr>& destination, const Ptr& widget)
{
    if (!widget) {
        return;
    }

    auto detachFromList = [&](std::vector<Ptr>& items) {
        auto it = std::find(items.begin(), items.end(), widget);
        if (it != items.end()) {
            items.erase(it);
        }
    };
    detachFromList(LeadingItems_);
    detachFromList(TrailingItems_);

    if (std::shared_ptr<ImWidget> parent = widget->GetParent()) {
        if (parent.get() != this) {
            parent->RemoveChild(widget);
        }
    }

    if (widget->GetParent().get() != this) {
        ImWidget::AddChild(widget);
    }

    destination.push_back(widget);
    MarkLayoutDirty();
}

void ImTitleBar::SyncChildGeometries() const
{
    for (const FChildLayout& layout : LeadingLayouts_) {
        if (layout.Widget) {
            layout.Widget->SetGeometry(layout.Geometry);
        }
    }

    for (const FChildLayout& layout : TrailingLayouts_) {
        if (layout.Widget) {
            layout.Widget->SetGeometry(layout.Geometry);
        }
    }
}

void ImTitleBar::ClearHoveredState()
{
    if (HoveredSystemButton_ == ESystemButton::None && !HasToolTip()) {
        return;
    }

    HoveredSystemButton_ = ESystemButton::None;
    ClearToolTip();
    Invalidate(EInvalidateReason::Paint);
}

void ImTitleBar::UpdateHoveredState(const FVector2& mousePosition)
{
    const ESystemButton hoveredButton = HitTestSystemButton(mousePosition);
    if (HoveredSystemButton_ == hoveredButton) {
        return;
    }

    HoveredSystemButton_ = hoveredButton;
    UpdateToolTipForHoveredState();
    Invalidate(EInvalidateReason::Paint);
}

void ImTitleBar::UpdateToolTipForHoveredState()
{
    switch (HoveredSystemButton_) {
    case ESystemButton::Minimize:
        SetToolTipText("Minimize");
        break;
    case ESystemButton::Maximize: {
        ImApplicationBackend* backend = GetBackend();
        SetToolTipText((backend != nullptr && backend->IsHostWindowMaximized()) ? "Restore" : "Maximize");
        break;
    }
    case ESystemButton::Close:
        SetToolTipText("Close");
        break;
    default:
        ClearToolTip();
        break;
    }
}

void ImTitleBar::PaintChildren(const FPaintContext& paintContext) const
{
    for (const FChildLayout& layout : LeadingLayouts_) {
        if (layout.Widget && layout.Widget->IsVisible()) {
            layout.Widget->Paint(paintContext);
        }
    }

    for (const FChildLayout& layout : TrailingLayouts_) {
        if (layout.Widget && layout.Widget->IsVisible()) {
            layout.Widget->Paint(paintContext);
        }
    }
}

void ImTitleBar::PaintSystemButtons(const FPaintContext& paintContext) const
{
    for (const auto& pair : {
             std::pair<ESystemButton, FGeometry>(ESystemButton::Minimize, MinimizeButtonGeometry_),
             std::pair<ESystemButton, FGeometry>(ESystemButton::Maximize, MaximizeButtonGeometry_),
             std::pair<ESystemButton, FGeometry>(ESystemButton::Close, CloseButtonGeometry_)}) {
        if (pair.first == ESystemButton::None || pair.second.Size.X <= 0.0f || pair.second.Size.Y <= 0.0f) {
            continue;
        }

        PaintSystemButton(paintContext, pair.first, pair.second);
    }
}

void ImTitleBar::PaintSystemButton(const FPaintContext& paintContext, ESystemButton button, const FGeometry& geometry) const
{
    FColor fillColor = FColor(0.0f, 0.0f, 0.0f, 0.0f);
    const bool bHovered = HoveredSystemButton_ == button;
    const bool bPressed = PressedSystemButton_ == button;

    if (button == ESystemButton::Close) {
        if (bPressed) {
            fillColor = GetEffectiveStyle().CloseButtonPressedColor;
        } else if (bHovered) {
            fillColor = GetEffectiveStyle().CloseButtonHoveredColor;
        }
    } else if (bPressed) {
        fillColor = GetEffectiveStyle().PressedSystemButtonColor;
    } else if (bHovered) {
        fillColor = GetEffectiveStyle().HoveredSystemButtonColor;
    }

    if (fillColor.A > 0.0f) {
        paintContext.DrawContext_.DrawRectFilled(geometry.GetMin(), geometry.GetMax(), fillColor);
    }

    DrawSystemButtonGlyph(paintContext, button, geometry);
}

void ImTitleBar::DrawSystemButtonGlyph(const FPaintContext& paintContext, ESystemButton button, const FGeometry& geometry) const
{
    const ImApplicationBackend* backend = GetBackend();
    const FColor glyphColor = FColor::FromBytes(244, 247, 251);
    const FVector2 center(
        geometry.Position.X + geometry.Size.X * 0.5f,
        geometry.Position.Y + geometry.Size.Y * 0.5f);
    const float halfExtent = std::max(4.0f, std::min(geometry.Size.X, geometry.Size.Y) * 0.18f);
    const float stroke = std::max(1.25f, std::min(geometry.Size.X, geometry.Size.Y) * 0.05f);

    switch (button) {
    case ESystemButton::Minimize:
        paintContext.DrawContext_.DrawLine(
            FVector2(center.X - halfExtent, center.Y + halfExtent * 0.55f),
            FVector2(center.X + halfExtent, center.Y + halfExtent * 0.55f),
            glyphColor,
            stroke);
        break;

    case ESystemButton::Maximize:
        if (backend != nullptr && backend->IsHostWindowMaximized()) {
            const float offset = std::max(1.5f, halfExtent * 0.42f);
            paintContext.DrawContext_.DrawRect(
                FVector2(center.X - halfExtent + offset, center.Y - halfExtent - offset),
                FVector2(center.X + halfExtent + offset, center.Y + halfExtent - offset),
                glyphColor,
                0.0f,
                stroke);
            paintContext.DrawContext_.DrawRect(
                FVector2(center.X - halfExtent - offset, center.Y - halfExtent + offset),
                FVector2(center.X + halfExtent - offset, center.Y + halfExtent + offset),
                glyphColor,
                0.0f,
                stroke);
        } else {
            paintContext.DrawContext_.DrawRect(
                FVector2(center.X - halfExtent, center.Y - halfExtent),
                FVector2(center.X + halfExtent, center.Y + halfExtent),
                glyphColor,
                0.0f,
                stroke);
        }
        break;

    case ESystemButton::Close:
        paintContext.DrawContext_.DrawLine(
            FVector2(center.X - halfExtent, center.Y - halfExtent),
            FVector2(center.X + halfExtent, center.Y + halfExtent),
            glyphColor,
            stroke);
        paintContext.DrawContext_.DrawLine(
            FVector2(center.X + halfExtent, center.Y - halfExtent),
            FVector2(center.X - halfExtent, center.Y + halfExtent),
            glyphColor,
            stroke);
        break;

    default:
        break;
    }
}

bool ImTitleBar::HasSystemButtons() const
{
    return IsSystemButtonVisible(ESystemButton::Minimize) ||
           IsSystemButtonVisible(ESystemButton::Maximize) ||
           IsSystemButtonVisible(ESystemButton::Close);
}

bool ImTitleBar::IsSystemButtonVisible(ESystemButton button) const
{
    if (!bShowSystemButtons_) {
        return false;
    }

    ImApplicationBackend* backend = GetBackend();
    if (backend == nullptr) {
        return false;
    }

    switch (button) {
    case ESystemButton::Minimize:
        return bShowMinimizeButton_ && backend->SupportsHostWindowMinimize();
    case ESystemButton::Maximize:
        return bShowMaximizeButton_ && backend->SupportsHostWindowMaximize();
    case ESystemButton::Close:
        return bShowCloseButton_ && backend->SupportsHostWindowClose();
    default:
        return false;
    }
}

float ImTitleBar::GetResolvedDragRegionMinWidth() const
{
    const FTitleBarStyle& style = GetEffectiveStyle();
    return ReflectedDragRegionMinWidth_ >= 0.0f
        ? ReflectedDragRegionMinWidth_
        : ClampNonNegative(style.DragRegionMinWidth);
}

const FTitleBarStyle& ImTitleBar::GetEffectiveStyle() const
{
    if (bHasExplicitStyle_) {
        return Style_;
    }

    if (const ImApplication* application = GetApplication()) {
        ResolvedThemeStyle_ = ResolveTitleBarStyle(application->GetStyleSet());
        return ResolvedThemeStyle_;
    }

    return Style_;
}

ImTitleBar::ESystemButton ImTitleBar::HitTestSystemButton(const FVector2& position) const
{
    if (MinimizeButtonGeometry_.Contains(position)) {
        return ESystemButton::Minimize;
    }
    if (MaximizeButtonGeometry_.Contains(position)) {
        return ESystemButton::Maximize;
    }
    if (CloseButtonGeometry_.Contains(position)) {
        return ESystemButton::Close;
    }
    return ESystemButton::None;
}

bool ImTitleBar::HitTestsChildWidgets(const FVector2& position, std::vector<Ptr>* outPath) const
{
    const auto visit = [&](const std::vector<FChildLayout>& layouts) {
        for (auto it = layouts.rbegin(); it != layouts.rend(); ++it) {
            if (!it->Widget) {
                continue;
            }

            if (outPath != nullptr) {
                if (it->Widget->BuildHitTestPath(position, *outPath)) {
                    return true;
                }
            } else if (it->Geometry.Contains(position)) {
                return true;
            }
        }
        return false;
    };

    return visit(TrailingLayouts_) || visit(LeadingLayouts_);
}

bool ImTitleBar::IsPointInHostDragArea(const FVector2& position) const
{
    return m_Geometry.Contains(position) &&
           HitTestSystemButton(position) == ESystemButton::None;
}

ImApplicationBackend* ImTitleBar::GetBackend() const
{
    ImApplication* application = GetApplication();
    return application != nullptr ? application->GetBackend() : nullptr;
}

bool ImTitleBar::HandleSystemButtonClick(ESystemButton button)
{
    ImApplicationBackend* backend = GetBackend();
    if (backend == nullptr) {
        return false;
    }

    switch (button) {
    case ESystemButton::Minimize:
        return backend->MinimizeHostWindow();
    case ESystemButton::Maximize:
        return backend->ToggleHostWindowMaximize();
    case ESystemButton::Close:
        return backend->CloseHostWindow();
    default:
        return false;
    }
}

void ImTitleBar::MarkLayoutDirty()
{
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint | EInvalidateReason::ChildOrder);
}

} // namespace ImWidgetV4
