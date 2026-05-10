#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

namespace ImWidgetV4 {

namespace {

constexpr double TabDoubleClickThresholdSeconds = 0.35;
constexpr float LayoutGeometryEpsilon = 0.01f;

FGeometry InsetGeometry(const FGeometry& geometry, const FMargin& margin, float borderThickness)
{
    const float left = borderThickness + margin.Left;
    const float top = borderThickness + margin.Top;
    const float right = borderThickness + margin.Right;
    const float bottom = borderThickness + margin.Bottom;

    return FGeometry(
        FVector2(geometry.Position.X + left, geometry.Position.Y + top),
        FVector2(
            std::max(0.0f, geometry.Size.X - left - right),
            std::max(0.0f, geometry.Size.Y - top - bottom)));
}

bool NearlyEqual(float left, float right)
{
    return std::fabs(left - right) <= LayoutGeometryEpsilon;
}

float SnapToPixel(float value)
{
    return std::round(value);
}

bool AreGeometriesEquivalent(const FGeometry& left, const FGeometry& right)
{
    return NearlyEqual(left.Position.X, right.Position.X) &&
           NearlyEqual(left.Position.Y, right.Position.Y) &&
           NearlyEqual(left.Size.X, right.Size.X) &&
           NearlyEqual(left.Size.Y, right.Size.Y);
}

} // namespace

ImTabView::ImTabView()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

int ImTabView::AddTab(const std::string& title, const std::shared_ptr<ImWidget>& content)
{
    return AddTab(FText::FromString(title), content);
}

int ImTabView::AddTab(const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content)
{
    return AddTab(FText::FromString(title), icon, content);
}

int ImTabView::AddTab(const FText& title, const std::shared_ptr<ImWidget>& content)
{
    return InsertTab(static_cast<int>(Tabs_.size()), title, FImageBrush(), content);
}

int ImTabView::AddTab(const FText& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content)
{
    return InsertTab(static_cast<int>(Tabs_.size()), title, icon, content);
}

int ImTabView::InsertTab(int index, const std::string& title, const std::shared_ptr<ImWidget>& content)
{
    return InsertTab(index, FText::FromString(title), content);
}

int ImTabView::InsertTab(int index, const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content)
{
    return InsertTab(index, FText::FromString(title), icon, content);
}

int ImTabView::InsertTab(int index, const FText& title, const std::shared_ptr<ImWidget>& content)
{
    return InsertTab(index, title, FImageBrush(), content);
}

int ImTabView::InsertTab(int index, const FText& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content)
{
    FTabViewItem item;
    item.TitleText = title;
    if (title.IsLocalized()) {
        item.Title = title.GetDefaultText().empty() ? title.GetKey() : title.GetDefaultText();
    } else {
        item.Title = title.GetInvariantText();
    }
    item.Icon = icon;
    item.Content = content;
    const int insertIndex = std::clamp(index, 0, static_cast<int>(Tabs_.size()));
    Tabs_.insert(Tabs_.begin() + insertIndex, std::move(item));

    for (int& historyIndex : ActivationHistory_) {
        if (historyIndex >= insertIndex) {
            ++historyIndex;
        }
    }

    if (HoveredTabIndex_ >= insertIndex) {
        ++HoveredTabIndex_;
    }
    if (PressedTabIndex_ >= insertIndex) {
        ++PressedTabIndex_;
    }
    if (HoveredCloseTabIndex_ >= insertIndex) {
        ++HoveredCloseTabIndex_;
    }
    if (PressedCloseTabIndex_ >= insertIndex) {
        ++PressedCloseTabIndex_;
    }
    if (LastClickedTabIndex_ >= insertIndex) {
        ++LastClickedTabIndex_;
    }
    if (ActiveTabIndex_ >= insertIndex) {
        ++ActiveTabIndex_;
    }

    bLayoutDirty_ = true;

    if (ActiveTabIndex_ < 0 && Tabs_[static_cast<std::size_t>(insertIndex)].bEnabled) {
        SetActiveTab(insertIndex);
    } else {
        Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    }

    return insertIndex;
}

bool ImTabView::RemoveTab(int index)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    const int previousActiveIndex = ActiveTabIndex_;
    const bool bRemovingActive = index == ActiveTabIndex_;
    const std::shared_ptr<ImWidget> removedContent = Tabs_[static_cast<std::size_t>(index)].Content;
    if (bRemovingActive) {
        CleanupInteractionStateForContent(removedContent);
    }

    Tabs_.erase(Tabs_.begin() + index);
    ActivationHistory_.erase(
        std::remove(ActivationHistory_.begin(), ActivationHistory_.end(), index),
        ActivationHistory_.end());
    for (int& historyIndex : ActivationHistory_) {
        if (historyIndex > index) {
            --historyIndex;
        }
    }

    if (Tabs_.empty()) {
        ActiveTabIndex_ = -1;
        TabScrollOffset_ = 0.0f;
    } else if (bRemovingActive) {
        ActiveTabIndex_ = ResolveReplacementActiveIndex(index);
    } else if (ActiveTabIndex_ > index) {
        --ActiveTabIndex_;
    }

    HoveredTabIndex_ = HoveredTabIndex_ == index ? -1 : (HoveredTabIndex_ > index ? HoveredTabIndex_ - 1 : HoveredTabIndex_);
    PressedTabIndex_ = PressedTabIndex_ == index ? -1 : (PressedTabIndex_ > index ? PressedTabIndex_ - 1 : PressedTabIndex_);
    HoveredCloseTabIndex_ = HoveredCloseTabIndex_ == index ? -1 : (HoveredCloseTabIndex_ > index ? HoveredCloseTabIndex_ - 1 : HoveredCloseTabIndex_);
    PressedCloseTabIndex_ = PressedCloseTabIndex_ == index ? -1 : (PressedCloseTabIndex_ > index ? PressedCloseTabIndex_ - 1 : PressedCloseTabIndex_);
    if (LastClickedTabIndex_ == index) {
        LastClickedTabIndex_ = -1;
        LastClickTimestamp_ = -1.0;
    } else if (LastClickedTabIndex_ > index) {
        --LastClickedTabIndex_;
    }
    bEnsureActiveTabVisible_ = bRemovingActive && ActiveTabIndex_ >= 0;
    bLayoutDirty_ = true;
    UpdateRegisteredActiveContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (ActiveTabIndex_ != previousActiveIndex) {
        OnActiveTabChanged.Broadcast(*this, ActiveTabIndex_);
    }
    OnTabClosed.Broadcast(*this, index);
    return true;
}

bool ImTabView::RequestCloseTab(int index)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    bool bAllowClose = true;
    OnTabCloseRequested.Broadcast(*this, index, bAllowClose);
    if (!bAllowClose) {
        return false;
    }

    return RemoveTab(index);
}

void ImTabView::ClearTabs()
{
    if (Tabs_.empty()) {
        return;
    }

    const int previousActiveIndex = ActiveTabIndex_;
    if (IsValidIndex(ActiveTabIndex_)) {
        CleanupInteractionStateForContent(Tabs_[static_cast<std::size_t>(ActiveTabIndex_)].Content);
    }

    Tabs_.clear();
    ActiveTabIndex_ = -1;
    HoveredTabIndex_ = -1;
    PressedTabIndex_ = -1;
    HoveredCloseTabIndex_ = -1;
    PressedCloseTabIndex_ = -1;
    HoveredOverflowDirection_ = 0;
    PressedOverflowDirection_ = 0;
    LastClickedTabIndex_ = -1;
    LastClickTimestamp_ = -1.0;
    TabScrollOffset_ = 0.0f;
    bEnsureActiveTabVisible_ = false;
    bLayoutDirty_ = true;
    UpdateRegisteredActiveContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (previousActiveIndex != -1) {
        OnActiveTabChanged.Broadcast(*this, -1);
    }
}

bool ImTabView::SetActiveTab(int index)
{
    if (!IsValidIndex(index) || !IsTabEnabled(index)) {
        return false;
    }

    if (ActiveTabIndex_ == index) {
        Invalidate(EInvalidateReason::Paint);
        return true;
    }

    CleanupInteractionStateForContent(GetActiveContent());
    ActiveTabIndex_ = index;
    NoteTabActivated(index);
    bEnsureActiveTabVisible_ = true;
    bLayoutDirty_ = true;
    UpdateRegisteredActiveContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    OnActiveTabChanged.Broadcast(*this, ActiveTabIndex_);
    return true;
}

std::shared_ptr<ImWidget> ImTabView::GetActiveContent() const
{
    return IsValidIndex(ActiveTabIndex_)
        ? Tabs_[static_cast<std::size_t>(ActiveTabIndex_)].Content
        : nullptr;
}

const FTabViewItem* ImTabView::GetTab(int index) const
{
    if (!IsValidIndex(index)) {
        return nullptr;
    }

    return &Tabs_[static_cast<std::size_t>(index)];
}

bool ImTabView::SetTabEnabled(int index, bool bEnabled)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    const int previousActiveIndex = ActiveTabIndex_;
    FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
    if (item.bEnabled == bEnabled) {
        return true;
    }

    item.bEnabled = bEnabled;
    if (!item.bEnabled && ActiveTabIndex_ == index) {
        CleanupInteractionStateForContent(item.Content);
        ActiveTabIndex_ = ResolveReplacementActiveIndex(index);
        if (ActiveTabIndex_ >= 0) {
            NoteTabActivated(ActiveTabIndex_);
        }
        bEnsureActiveTabVisible_ = ActiveTabIndex_ >= 0;
        UpdateRegisteredActiveContent();
    } else if (item.bEnabled && ActiveTabIndex_ < 0) {
        ActiveTabIndex_ = index;
        NoteTabActivated(index);
        bEnsureActiveTabVisible_ = true;
        UpdateRegisteredActiveContent();
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (ActiveTabIndex_ != previousActiveIndex) {
        OnActiveTabChanged.Broadcast(*this, ActiveTabIndex_);
    }
    return true;
}

bool ImTabView::SetTabClosable(int index, bool bClosable)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
    if (item.bClosable == bClosable) {
        return true;
    }

    item.bClosable = bClosable;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return true;
}

bool ImTabView::IsTabClosable(int index) const
{
    return IsValidIndex(index) && Tabs_[static_cast<std::size_t>(index)].bClosable;
}

bool ImTabView::SetTabDirty(int index, bool bDirty)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
    if (item.bDirty == bDirty) {
        return true;
    }

    item.bDirty = bDirty;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return true;
}

bool ImTabView::IsTabDirty(int index) const
{
    return IsValidIndex(index) && Tabs_[static_cast<std::size_t>(index)].bDirty;
}

bool ImTabView::SetTabTitle(int index, const std::string& title)
{
    return SetTabTitle(index, FText::FromString(title));
}

bool ImTabView::SetTabTitle(int index, const FText& title)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
    if (item.TitleText == title) {
        return true;
    }

    item.TitleText = title;
    if (title.IsLocalized()) {
        item.Title = title.GetDefaultText().empty() ? title.GetKey() : title.GetDefaultText();
    } else {
        item.Title = title.GetInvariantText();
    }
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return true;
}

bool ImTabView::SetTabIcon(int index, const FImageBrush& icon)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
    item.Icon = icon;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return true;
}

void ImTabView::SetCloseActivationPolicy(ETabCloseActivationPolicy policy)
{
    if (CloseActivationPolicy_ == policy) {
        return;
    }

    CloseActivationPolicy_ = policy;
}

void ImTabView::SetTabStripPlacement(ETabStripPlacement placement)
{
    if (TabStripPlacement_ == placement) {
        return;
    }

    TabStripPlacement_ = placement;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTabView::SetStyle(const FTabViewStyle& style)
{
    Style_ = style;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTabView::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        Style_.BackgroundColor,
        Style_.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        HasFocusWithinTabView() ? Style_.FocusedOutlineColor : Style_.BorderColor,
        Style_.CornerRadius,
        Style_.BorderThickness);

    if (TabStripGeometry_.IsValid()) {
        paintContext.DrawContext_.DrawRectFilled(
            TabStripGeometry_.GetMin(),
            TabStripGeometry_.GetMax(),
            Style_.TabStripBackgroundColor,
            Style_.CornerRadius);
        paintContext.DrawContext_.PushClipRect(TabStripGeometry_.GetMin(), TabStripGeometry_.GetMax(), true);

        for (const FTabGeometry& tabGeometry : VisibleTabGeometries_) {
            const FTabViewItem& item = Tabs_[static_cast<std::size_t>(tabGeometry.Index)];
            const std::string title = ResolveTabTitle(item);
            paintContext.DrawContext_.DrawRectFilled(
                tabGeometry.Geometry.GetMin(),
                tabGeometry.Geometry.GetMax(),
                ResolveTabBackgroundColor(tabGeometry.Index),
                Style_.CornerRadius);
            paintContext.DrawContext_.DrawRect(
                tabGeometry.Geometry.GetMin(),
                tabGeometry.Geometry.GetMax(),
                Style_.TabBorderColor,
                Style_.CornerRadius,
                1.0f);

            paintContext.DrawContext_.PushClipRect(
                tabGeometry.Geometry.GetMin(),
                tabGeometry.Geometry.GetMax(),
                true);

            const FColor contentColor = ResolveTabTextColor(tabGeometry.Index);
            float contentX = tabGeometry.ContentStartX;
            if (item.Icon.IsValid()) {
                const float iconSize = std::min(
                    Style_.IconSize,
                    std::max(0.0f, tabGeometry.Geometry.Size.Y - Style_.TabPadding.Top - Style_.TabPadding.Bottom));
                const float iconY = tabGeometry.Geometry.Position.Y +
                    std::max(0.0f, (tabGeometry.Geometry.Size.Y - iconSize) * 0.5f);
                ImTextureID textureId = item.Icon.TextureId;
                if (GetApplication() != nullptr) {
                    textureId = GetApplication()->ResolveTextureForPaint(textureId);
                }
                if (textureId != nullptr) {
                    paintContext.DrawContext_.DrawImage(
                        textureId,
                        FVector2(contentX, iconY),
                        FVector2(contentX + iconSize, iconY + iconSize),
                        item.Icon.Uv0,
                        item.Icon.Uv1,
                        contentColor);
                    contentX += iconSize + 6.0f;
                }
            }

            if (item.bDirty) {
                const float markerCenterY = tabGeometry.Geometry.Position.Y + tabGeometry.Geometry.Size.Y * 0.5f;
                paintContext.DrawContext_.DrawCircleFilled(
                    FVector2(contentX + Style_.DirtyMarkerRadius, markerCenterY),
                    Style_.DirtyMarkerRadius,
                    Style_.DirtyMarkerColor);
                contentX += Style_.DirtyMarkerRadius * 2.0f + 6.0f;
            }

            const float textWidth = MeasureTextWidth(title);
            const float textLaneWidth = std::max(0.0f, tabGeometry.TextClipMaxX - contentX);
            const float textDrawX = SnapToPixel(contentX + (textLaneWidth - textWidth) * 0.5f);
            const float textY = SnapToPixel(
                tabGeometry.Geometry.Position.Y +
                std::max(0.0f, (tabGeometry.Geometry.Size.Y - MeasureTextHeight()) * 0.5f));
            paintContext.DrawContext_.PushClipRect(
                FVector2(contentX, tabGeometry.Geometry.Position.Y),
                FVector2(tabGeometry.TextClipMaxX, tabGeometry.Geometry.GetMax().Y),
                true);
            paintContext.DrawContext_.DrawText(
                FVector2(textDrawX, textY),
                contentColor,
                title,
                Style_.FontSize);
            paintContext.DrawContext_.PopClipRect();

            if (tabGeometry.bShowsCloseButton) {
                const FColor closeColor = ResolveCloseButtonColor(tabGeometry);
                const FVector2 closeMin = tabGeometry.CloseButtonGeometry.GetMin();
                const FVector2 closeMax = tabGeometry.CloseButtonGeometry.GetMax();
                const float inset = 2.0f;
                paintContext.DrawContext_.DrawLine(
                    closeMin + FVector2(inset, inset),
                    closeMax - FVector2(inset, inset),
                    closeColor,
                    1.5f);
                paintContext.DrawContext_.DrawLine(
                    FVector2(closeMin.X + inset, closeMax.Y - inset),
                    FVector2(closeMax.X - inset, closeMin.Y + inset),
                    closeColor,
                    1.5f);
            }

            paintContext.DrawContext_.PopClipRect();
        }

        if (LeftOverflowButtonGeometry_.IsValid()) {
            const FColor buttonColor = ResolveOverflowButtonColor(-1);
            paintContext.DrawContext_.DrawRectFilled(
                LeftOverflowButtonGeometry_.GetMin(),
                LeftOverflowButtonGeometry_.GetMax(),
                Style_.TabColor,
                Style_.CornerRadius);
            const FVector2 center = LeftOverflowButtonGeometry_.GetCenter();
            paintContext.DrawContext_.DrawLine(center + FVector2(3.0f, -5.0f), center + FVector2(-3.0f, 0.0f), buttonColor, 1.5f);
            paintContext.DrawContext_.DrawLine(center + FVector2(-3.0f, 0.0f), center + FVector2(3.0f, 5.0f), buttonColor, 1.5f);
        }

        if (RightOverflowButtonGeometry_.IsValid()) {
            const FColor buttonColor = ResolveOverflowButtonColor(1);
            paintContext.DrawContext_.DrawRectFilled(
                RightOverflowButtonGeometry_.GetMin(),
                RightOverflowButtonGeometry_.GetMax(),
                Style_.TabColor,
                Style_.CornerRadius);
            const FVector2 center = RightOverflowButtonGeometry_.GetCenter();
            paintContext.DrawContext_.DrawLine(center + FVector2(-3.0f, -5.0f), center + FVector2(3.0f, 0.0f), buttonColor, 1.5f);
            paintContext.DrawContext_.DrawLine(center + FVector2(3.0f, 0.0f), center + FVector2(-3.0f, 5.0f), buttonColor, 1.5f);
        }

        paintContext.DrawContext_.PopClipRect();
    }

    if (const std::shared_ptr<ImWidget> activeContent = GetActiveContent()) {
        paintContext.DrawContext_.PushClipRect(ContentGeometry_.GetMin(), ContentGeometry_.GetMax(), true);
        FPaintContext childPaintContext(
            paintContext.DrawContext_,
            activeContent->GetGeometry(),
            paintContext.StyleSet,
            paintContext.CursorPosition,
            paintContext.bHasCursorPosition,
            paintContext.DeltaTime);
        activeContent->Paint(childPaintContext);
        paintContext.DrawContext_.PopClipRect();
    }
}

FVector2 ImTabView::GetMinSize() const
{
    FVector2 minSize = Style_.MinDesiredSize;
    if (const std::shared_ptr<ImWidget> activeContent = GetActiveContent()) {
        const FVector2 contentMinSize = activeContent->GetMinSize();
        minSize.X = std::max(
            minSize.X,
            contentMinSize.X + Style_.Padding.Left + Style_.Padding.Right + Style_.BorderThickness * 2.0f);
        minSize.Y = std::max(
            minSize.Y,
            contentMinSize.Y + Style_.Padding.Top + Style_.Padding.Bottom + Style_.BorderThickness * 2.0f + Style_.TabHeight);
    }

    return minSize;
}

FReply ImTabView::OnPreviewInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::KeyDown && HandleKeyboardNavigation(event)) {
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply ImTabView::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseMove:
    case EInputEventType::MouseEnter: {
        const int hoveredCloseTab = ResolveCloseButtonTabIndexAt(event.MousePosition);
        const int hoveredOverflowDirection = LeftOverflowButtonGeometry_.Contains(event.MousePosition)
            ? -1
            : (RightOverflowButtonGeometry_.Contains(event.MousePosition) ? 1 : 0);
        const int hoveredTab = hoveredCloseTab >= 0 ? -1 : ResolveTabIndexAt(event.MousePosition);

        if (HoveredTabIndex_ != hoveredTab ||
            HoveredCloseTabIndex_ != hoveredCloseTab ||
            HoveredOverflowDirection_ != hoveredOverflowDirection) {
            HoveredTabIndex_ = hoveredTab;
            HoveredCloseTabIndex_ = hoveredCloseTab;
            HoveredOverflowDirection_ = hoveredOverflowDirection;
            UpdateHoveredTitleToolTip();
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    }

    case EInputEventType::MouseLeave:
        if (HoveredTabIndex_ != -1 || HoveredCloseTabIndex_ != -1 || HoveredOverflowDirection_ != 0) {
            HoveredTabIndex_ = -1;
            HoveredCloseTabIndex_ = -1;
            HoveredOverflowDirection_ = 0;
            UpdateHoveredTitleToolTip();
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonDown:
        if (event.MouseButton == EMouseButton::Left) {
            const int closeTabIndex = ResolveCloseButtonTabIndexAt(event.MousePosition);
            if (closeTabIndex >= 0) {
                PressedCloseTabIndex_ = closeTabIndex;
                Invalidate(EInvalidateReason::Paint);
                return FReply::Handled()
                    .SetKeyboardFocus(shared_from_this())
                    .CaptureMouse(shared_from_this(), EMouseButton::Left);
            }

            const int overflowDirection = LeftOverflowButtonGeometry_.Contains(event.MousePosition)
                ? -1
                : (RightOverflowButtonGeometry_.Contains(event.MousePosition) ? 1 : 0);
            if (overflowDirection != 0) {
                PressedOverflowDirection_ = overflowDirection;
                Invalidate(EInvalidateReason::Paint);
                return FReply::Handled()
                    .SetKeyboardFocus(shared_from_this())
                    .CaptureMouse(shared_from_this(), EMouseButton::Left);
            }

            const int tabIndex = ResolveTabIndexAt(event.MousePosition);
            if (tabIndex >= 0 && IsTabEnabled(tabIndex)) {
                PressedTabIndex_ = tabIndex;
                HoveredTabIndex_ = tabIndex;
                Invalidate(EInvalidateReason::Paint);
                return FReply::Handled()
                    .SetKeyboardFocus(shared_from_this())
                    .CaptureMouse(shared_from_this(), EMouseButton::Left);
            }
        } else if (event.MouseButton == EMouseButton::Middle) {
            const int tabIndex = ResolveTabIndexAt(event.MousePosition);
            if (tabIndex >= 0 && IsTabClosable(tabIndex)) {
                RequestCloseTab(tabIndex);
                return FReply::Handled();
            }
        } else if (event.MouseButton == EMouseButton::Right) {
            const int tabIndex = ResolveTabIndexAt(event.MousePosition);
            if (tabIndex >= 0 && IsTabEnabled(tabIndex)) {
                SetActiveTab(tabIndex);
                OnTabContextMenuRequested.Broadcast(*this, tabIndex, event.MousePosition);
                return FReply::Handled().SetKeyboardFocus(shared_from_this());
            }
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonUp:
        if (event.MouseButton == EMouseButton::Left) {
            if (PressedCloseTabIndex_ >= 0) {
                const int pressedCloseTab = PressedCloseTabIndex_;
                PressedCloseTabIndex_ = -1;
                const int releasedCloseTab = ResolveCloseButtonTabIndexAt(event.MousePosition);
                Invalidate(EInvalidateReason::Paint);
                if (pressedCloseTab == releasedCloseTab && IsTabClosable(pressedCloseTab)) {
                    RequestCloseTab(pressedCloseTab);
                }
                return FReply::Handled().ReleaseMouseCapture();
            }

            if (PressedOverflowDirection_ != 0) {
                const int pressedDirection = PressedOverflowDirection_;
                PressedOverflowDirection_ = 0;
                const int releasedDirection = LeftOverflowButtonGeometry_.Contains(event.MousePosition)
                    ? -1
                    : (RightOverflowButtonGeometry_.Contains(event.MousePosition) ? 1 : 0);
                Invalidate(EInvalidateReason::Paint);
                if (pressedDirection == releasedDirection && CanScrollTabs(pressedDirection)) {
                    ScrollTabs(pressedDirection);
                }
                return FReply::Handled().ReleaseMouseCapture();
            }

            if (PressedTabIndex_ >= 0) {
                const int pressedTab = PressedTabIndex_;
                PressedTabIndex_ = -1;
                const int releasedTab = ResolveTabIndexAt(event.MousePosition);
                Invalidate(EInvalidateReason::Paint);

                if (pressedTab == releasedTab && IsTabEnabled(pressedTab)) {
                    OnTabInvoked.Broadcast(*this, pressedTab);
                    SetActiveTab(pressedTab);
                    const bool bIsDoubleClick =
                        LastClickedTabIndex_ == pressedTab &&
                        LastClickTimestamp_ >= 0.0 &&
                        event.Timestamp > 0.0 &&
                        (event.Timestamp - LastClickTimestamp_) <= TabDoubleClickThresholdSeconds;
                    LastClickedTabIndex_ = pressedTab;
                    LastClickTimestamp_ = event.Timestamp;
                    if (bIsDoubleClick) {
                        OnTabDoubleClicked.Broadcast(*this, pressedTab);
                    }
                }

                return FReply::Handled().ReleaseMouseCapture();
            }
        }
        return FReply::Unhandled();

    default:
        return FReply::Unhandled();
    }
}

bool ImTabView::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());

    if (TabStripGeometry_.Contains(position)) {
        return true;
    }

    if (ContentGeometry_.Contains(position)) {
        if (const std::shared_ptr<ImWidget> activeContent = GetActiveContent()) {
            if (activeContent->BuildHitTestPath(position, outPath)) {
                return true;
            }
        }
    }

    return true;
}

void ImTabView::OnFocusChanged(bool)
{
    Invalidate(EInvalidateReason::Paint);
}

void ImTabView::SetActiveTabProperty(int& index)
{
    SetActiveTab(index);
}

int& ImTabView::GetActiveTabProperty()
{
    ReflectedActiveTabIndex_ = ActiveTabIndex_;
    return ReflectedActiveTabIndex_;
}

void ImTabView::SetCloseActivationPolicyProperty(int& value)
{
    value = std::clamp(value, 0, 1);
    SetCloseActivationPolicy(static_cast<ETabCloseActivationPolicy>(value));
}

int& ImTabView::GetCloseActivationPolicyProperty()
{
    ReflectedCloseActivationPolicy_ = static_cast<int>(CloseActivationPolicy_);
    return ReflectedCloseActivationPolicy_;
}

void ImTabView::SetTabStripPlacementProperty(int& value)
{
    value = std::clamp(value, 0, 1);
    SetTabStripPlacement(static_cast<ETabStripPlacement>(value));
}

int& ImTabView::GetTabStripPlacementProperty()
{
    ReflectedTabStripPlacement_ = static_cast<int>(TabStripPlacement_);
    return ReflectedTabStripPlacement_;
}

void ImTabView::Relayout()
{
    if (!bLayoutDirty_ &&
        bHasLastLayoutGeometry_ &&
        AreGeometriesEquivalent(LastLayoutGeometry_, m_Geometry)) {
        return;
    }

    LastLayoutGeometry_ = m_Geometry;
    bHasLastLayoutGeometry_ = true;
    UpdateRegisteredActiveContent();

    const FGeometry innerGeometry = GetInnerGeometry();
    const float stripHeight = std::min(Style_.TabHeight, innerGeometry.Size.Y);
    const float contentHeight = std::max(0.0f, innerGeometry.Size.Y - stripHeight);
    if (TabStripPlacement_ == ETabStripPlacement::Bottom) {
        ContentGeometry_ = FGeometry(
            innerGeometry.Position,
            FVector2(innerGeometry.Size.X, contentHeight));
        TabStripGeometry_ = FGeometry(
            FVector2(innerGeometry.Position.X, innerGeometry.Position.Y + contentHeight),
            FVector2(innerGeometry.Size.X, stripHeight));
    } else {
        TabStripGeometry_ = FGeometry(
            innerGeometry.Position,
            FVector2(innerGeometry.Size.X, stripHeight));
        ContentGeometry_ = FGeometry(
            FVector2(innerGeometry.Position.X, innerGeometry.Position.Y + stripHeight),
            FVector2(innerGeometry.Size.X, contentHeight));
    }

    float totalTabsWidth = 0.0f;
    std::vector<float> naturalTabWidths;
    naturalTabWidths.reserve(Tabs_.size());
    for (std::size_t tabIndex = 0; tabIndex < Tabs_.size(); ++tabIndex) {
        const float naturalWidth = ComputeTabWidth(Tabs_[tabIndex]);
        naturalTabWidths.push_back(naturalWidth);
        totalTabsWidth += naturalWidth;
        if (tabIndex + 1 < Tabs_.size()) {
            totalTabsWidth += Style_.TabSpacing;
        }
    }

    LeftOverflowButtonGeometry_ = FGeometry();
    RightOverflowButtonGeometry_ = FGeometry();
    const float tabsViewportWidth = GetTabsViewportWidth();
    const float totalSpacing = Tabs_.size() > 1
        ? Style_.TabSpacing * static_cast<float>(Tabs_.size() - 1)
        : 0.0f;
    const float widthAvailableForTabs = std::max(0.0f, tabsViewportWidth - totalSpacing);
    float naturalTabsWidth = std::max(0.0f, totalTabsWidth - totalSpacing);
    const float widthScale = naturalTabsWidth > 0.0f && naturalTabsWidth > widthAvailableForTabs
        ? (widthAvailableForTabs / naturalTabsWidth)
        : 1.0f;
    TabScrollOffset_ = 0.0f;
    bEnsureActiveTabVisible_ = false;

    VisibleTabGeometries_.clear();
    float cursorX = TabStripGeometry_.Position.X;
    const float stripMinX = TabStripGeometry_.Position.X;
    const float stripMaxX = stripMinX + tabsViewportWidth;

    for (int index = 0; index < static_cast<int>(Tabs_.size()); ++index) {
        const FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
        const std::string title = ResolveTabTitle(item);
        const float exactTabStartX = cursorX;
        const float exactTabMaxX = exactTabStartX + naturalTabWidths[static_cast<std::size_t>(index)] * widthScale;
        const float tabMinX = SnapToPixel(exactTabStartX);
        const float tabMaxX = SnapToPixel(exactTabMaxX);
        const float tabWidth = std::max(0.0f, tabMaxX - tabMinX);

        if (tabMaxX >= stripMinX && cursorX <= stripMaxX) {
            FTabGeometry geometry;
            geometry.Index = index;
            geometry.Geometry = FGeometry(
                FVector2(tabMinX, TabStripGeometry_.Position.Y),
                FVector2(tabWidth, TabStripGeometry_.Size.Y));
            geometry.bShowsCloseButton = item.bClosable;

            const float innerMinX = geometry.Geometry.Position.X + Style_.TabPadding.Left;
            const float innerMaxX = std::max(innerMinX, geometry.Geometry.GetMax().X - Style_.TabPadding.Right);
            const float availableInnerWidth = std::max(0.0f, innerMaxX - innerMinX);
            const float iconSize = item.Icon.IsValid()
                ? std::min(
                    Style_.IconSize,
                    std::max(0.0f, geometry.Geometry.Size.Y - Style_.TabPadding.Top - Style_.TabPadding.Bottom))
                : 0.0f;
            const float iconWidth = item.Icon.IsValid() ? (iconSize + 6.0f) : 0.0f;
            const float dirtyWidth = item.bDirty ? (Style_.DirtyMarkerRadius * 2.0f + 6.0f) : 0.0f;
            const float textWidth = MeasureTextWidth(title);
            const float closeSize = geometry.bShowsCloseButton
                ? std::min(
                    Style_.CloseButtonSize,
                    std::max(0.0f, geometry.Geometry.Size.Y - Style_.TabPadding.Top - Style_.TabPadding.Bottom))
                : 0.0f;
            const float closeWidth = geometry.bShowsCloseButton ? (closeSize + 6.0f) : 0.0f;
            const float contentWidth = iconWidth + dirtyWidth + textWidth + closeWidth;

            geometry.ContentStartX = innerMinX;
            if (contentWidth < availableInnerWidth - 0.5f) {
                geometry.ContentStartX += (availableInnerWidth - contentWidth) * 0.5f;
            }
            geometry.ContentStartX = SnapToPixel(geometry.ContentStartX);

            float contentX = geometry.ContentStartX;
            if (item.Icon.IsValid()) {
                contentX += iconSize + 6.0f;
            }
            if (item.bDirty) {
                contentX += Style_.DirtyMarkerRadius * 2.0f + 6.0f;
            }
            float textClipMaxX = geometry.Geometry.GetMax().X - Style_.TabPadding.Right;
            if (geometry.bShowsCloseButton) {
                const float closeX = SnapToPixel(contentX + textWidth + 6.0f);
                const float closeY = SnapToPixel(
                    geometry.Geometry.Position.Y +
                    std::max(0.0f, (geometry.Geometry.Size.Y - closeSize) * 0.5f));
                geometry.CloseButtonGeometry = FGeometry(
                    FVector2(closeX, closeY),
                    FVector2(closeSize, closeSize));
                textClipMaxX = std::min(textClipMaxX, closeX - 6.0f);
            }
            const float visibleTextClipMinX = std::max(contentX, geometry.Geometry.Position.X);
            geometry.TextClipMaxX = std::max(visibleTextClipMinX, std::min(textClipMaxX, geometry.Geometry.GetMax().X));
            geometry.bTitleClipped =
                textWidth > std::max(0.0f, geometry.TextClipMaxX - visibleTextClipMinX) + 0.5f;
            VisibleTabGeometries_.push_back(geometry);
        }

        cursorX = exactTabMaxX + Style_.TabSpacing;
    }

    if (const std::shared_ptr<ImWidget> activeContent = GetActiveContent()) {
        activeContent->SetGeometry(ContentGeometry_);
    }

    UpdateHoveredTitleToolTip();
    bLayoutDirty_ = false;
}

void ImTabView::UpdateRegisteredActiveContent()
{
    const std::shared_ptr<ImWidget> desiredContent = GetActiveContent();
    if (RegisteredActiveTabIndex_ == ActiveTabIndex_ &&
        ((desiredContent == nullptr && m_Children.empty()) ||
         (!m_Children.empty() && desiredContent == m_Children.front()))) {
        return;
    }

    ImWidget::ClearChildren();
    if (desiredContent) {
        AddChild(desiredContent);
    }
    RegisteredActiveTabIndex_ = ActiveTabIndex_;
}

void ImTabView::NoteTabActivated(int index)
{
    if (!IsValidIndex(index)) {
        return;
    }

    ActivationHistory_.erase(
        std::remove(ActivationHistory_.begin(), ActivationHistory_.end(), index),
        ActivationHistory_.end());
    ActivationHistory_.push_back(index);
}

void ImTabView::UpdateHoveredTitleToolTip()
{
    if (HoveredTabIndex_ < 0) {
        ClearToolTip();
        return;
    }

    for (const FTabGeometry& geometry : VisibleTabGeometries_) {
        if (geometry.Index == HoveredTabIndex_) {
            if (geometry.bTitleClipped && IsValidIndex(geometry.Index)) {
                SetToolTipText(Tabs_[static_cast<std::size_t>(geometry.Index)].TitleText);
            } else {
                ClearToolTip();
            }
            return;
        }
    }

    ClearToolTip();
}

void ImTabView::CleanupInteractionStateForContent(const std::shared_ptr<ImWidget>& content)
{
    ImApplication* application = GetApplication();
    if (application == nullptr || !content) {
        return;
    }

    const std::shared_ptr<ImWidget>& focusedWidget = application->GetKeyboardFocus();
    if (focusedWidget && ContainsWidgetInSubtree(focusedWidget, content)) {
        application->ClearKeyboardFocus();
    }

    const std::shared_ptr<ImWidget>& capturedWidget = application->GetMouseCapture();
    if (capturedWidget && ContainsWidgetInSubtree(capturedWidget, content)) {
        application->ReleaseMouseCapture();
    }
}

bool ImTabView::ContainsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget, const std::shared_ptr<ImWidget>& subtreeRoot) const
{
    if (!widget || !subtreeRoot) {
        return false;
    }

    std::shared_ptr<ImWidget> current = widget;
    while (current) {
        if (current == subtreeRoot) {
            return true;
        }
        current = current->GetParent();
    }

    return false;
}

int ImTabView::ResolveTabIndexAt(const FVector2& position) const
{
    if (!TabStripGeometry_.Contains(position) ||
        LeftOverflowButtonGeometry_.Contains(position) ||
        RightOverflowButtonGeometry_.Contains(position)) {
        return -1;
    }

    for (const FTabGeometry& tabGeometry : VisibleTabGeometries_) {
        if (tabGeometry.Geometry.Contains(position) &&
            (!tabGeometry.bShowsCloseButton || !tabGeometry.CloseButtonGeometry.Contains(position))) {
            return tabGeometry.Index;
        }
    }

    return -1;
}

int ImTabView::ResolveCloseButtonTabIndexAt(const FVector2& position) const
{
    if (!TabStripGeometry_.Contains(position)) {
        return -1;
    }

    for (const FTabGeometry& tabGeometry : VisibleTabGeometries_) {
        if (tabGeometry.bShowsCloseButton && tabGeometry.CloseButtonGeometry.Contains(position)) {
            return tabGeometry.Index;
        }
    }

    return -1;
}

bool ImTabView::HandleKeyboardNavigation(const FInputEvent& event)
{
    if (!HasFocusWithinTabView() || Tabs_.empty()) {
        return false;
    }

    int targetIndex = -1;
    if (event.Key == EKey::Tab && event.Modifiers.bCtrl) {
        targetIndex = event.Modifiers.bShift
            ? FindNextEnabledTab(ActiveTabIndex_, -1, true)
            : FindNextEnabledTab(ActiveTabIndex_, 1, true);
    } else if (event.Key == EKey::Home) {
        targetIndex = FindFirstEnabledTab();
    } else if (event.Key == EKey::End) {
        targetIndex = FindLastEnabledTab();
    } else {
        return false;
    }

    return targetIndex >= 0 && SetActiveTab(targetIndex);
}

bool ImTabView::IsValidIndex(int index) const
{
    return index >= 0 && index < static_cast<int>(Tabs_.size());
}

bool ImTabView::IsTabEnabled(int index) const
{
    return IsValidIndex(index) && Tabs_[static_cast<std::size_t>(index)].bEnabled;
}

int ImTabView::FindNextEnabledTab(int startIndex, int direction, bool bWrap) const
{
    if (Tabs_.empty()) {
        return -1;
    }

    const int count = static_cast<int>(Tabs_.size());
    int index = startIndex;
    for (int step = 0; step < count; ++step) {
        index += direction;
        if (index < 0 || index >= count) {
            if (!bWrap) {
                return -1;
            }
            index = direction > 0 ? 0 : (count - 1);
        }

        if (IsTabEnabled(index)) {
            return index;
        }
    }

    return -1;
}

int ImTabView::FindFirstEnabledTab() const
{
    for (int index = 0; index < static_cast<int>(Tabs_.size()); ++index) {
        if (IsTabEnabled(index)) {
            return index;
        }
    }

    return -1;
}

int ImTabView::FindLastEnabledTab() const
{
    for (int index = static_cast<int>(Tabs_.size()) - 1; index >= 0; --index) {
        if (IsTabEnabled(index)) {
            return index;
        }
    }

    return -1;
}

int ImTabView::FindMostRecentlyActiveTab() const
{
    for (auto it = ActivationHistory_.rbegin(); it != ActivationHistory_.rend(); ++it) {
        if (IsTabEnabled(*it)) {
            return *it;
        }
    }

    return -1;
}

int ImTabView::ResolveReplacementActiveIndex(int removedIndex) const
{
    if (Tabs_.empty()) {
        return -1;
    }

    if (CloseActivationPolicy_ == ETabCloseActivationPolicy::MostRecentlyActive) {
        const int recentIndex = FindMostRecentlyActiveTab();
        if (recentIndex >= 0) {
            return recentIndex;
        }
    }

    const int leftStart = std::min(removedIndex - 1, static_cast<int>(Tabs_.size()) - 1);
    for (int index = leftStart; index >= 0; --index) {
        if (IsTabEnabled(index)) {
            return index;
        }
    }

    const int rightStart = std::clamp(removedIndex, 0, static_cast<int>(Tabs_.size()) - 1);
    for (int index = rightStart; index < static_cast<int>(Tabs_.size()); ++index) {
        if (IsTabEnabled(index)) {
            return index;
        }
    }

    return -1;
}

float ImTabView::MeasureTextWidth(const std::string& text) const
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        const ImVec2 size = ImGui::GetFont()->CalcTextSizeA(
            Style_.FontSize,
            FLT_MAX,
            0.0f,
            text.c_str(),
            text.c_str() + text.size());
        return size.x;
    }

    return static_cast<float>(text.size()) * Style_.FontSize * 0.55f;
}

std::string ImTabView::ResolveTabTitle(const FTabViewItem& item) const
{
    return item.TitleText.Resolve();
}

float ImTabView::MeasureTextHeight() const
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(Style_.FontSize, FLT_MAX, 0.0f, "Ag").y;
    }

    return Style_.FontSize;
}

FGeometry ImTabView::GetInnerGeometry() const
{
    return InsetGeometry(m_Geometry, Style_.Padding, Style_.BorderThickness);
}

FColor ImTabView::ResolveTabBackgroundColor(int index) const
{
    if (!IsTabEnabled(index)) {
        return Style_.DisabledTabColor;
    }
    if (PressedTabIndex_ == index) {
        return Style_.TabPressedColor;
    }
    if (ActiveTabIndex_ == index) {
        return Style_.ActiveTabColor;
    }
    if (HoveredTabIndex_ == index) {
        return Style_.TabHoveredColor;
    }
    return Style_.TabColor;
}

FColor ImTabView::ResolveTabTextColor(int index) const
{
    if (!IsTabEnabled(index)) {
        return Style_.DisabledTextColor;
    }
    if (ActiveTabIndex_ == index) {
        return Style_.ActiveTextColor;
    }
    return Style_.TextColor;
}

FColor ImTabView::ResolveCloseButtonColor(const FTabGeometry& tabGeometry) const
{
    if (PressedCloseTabIndex_ == tabGeometry.Index) {
        return Style_.CloseButtonPressedColor;
    }
    if (HoveredCloseTabIndex_ == tabGeometry.Index) {
        return Style_.CloseButtonHoveredColor;
    }
    return Style_.CloseButtonColor;
}

FColor ImTabView::ResolveOverflowButtonColor(int direction) const
{
    if (!CanScrollTabs(direction)) {
        return Style_.OverflowButtonDisabledColor;
    }
    if (PressedOverflowDirection_ == direction) {
        return Style_.OverflowButtonPressedColor;
    }
    if (HoveredOverflowDirection_ == direction) {
        return Style_.OverflowButtonHoveredColor;
    }
    return Style_.OverflowButtonColor;
}

bool ImTabView::HasFocusWithinTabView() const
{
    ImApplication* application = GetApplication();
    if (application == nullptr) {
        return HasKeyboardFocus();
    }

    const std::shared_ptr<ImWidget>& focusedWidget = application->GetKeyboardFocus();
    if (!focusedWidget) {
        return HasKeyboardFocus();
    }

    if (focusedWidget.get() == this) {
        return true;
    }

    return ContainsWidgetInSubtree(focusedWidget, GetActiveContent());
}

bool ImTabView::CanScrollTabs(int direction) const
{
    if (direction < 0) {
        return TabScrollOffset_ > 0.5f;
    }

    const float totalTabsWidth = [&]() {
        float width = 0.0f;
        for (std::size_t index = 0; index < Tabs_.size(); ++index) {
            width += ComputeTabWidth(Tabs_[index]);
            if (index + 1 < Tabs_.size()) {
                width += Style_.TabSpacing;
            }
        }
        return width;
    }();

    return TabScrollOffset_ < std::max(0.0f, totalTabsWidth - GetTabsViewportWidth()) - 0.5f;
}

void ImTabView::ScrollTabs(int direction)
{
    (void)direction;
}

void ImTabView::EnsureTabVisible(int index, float viewportWidth)
{
    if (!IsValidIndex(index) || viewportWidth <= 0.0f) {
        return;
    }

    float tabStart = 0.0f;
    for (int currentIndex = 0; currentIndex < index; ++currentIndex) {
        tabStart += ComputeTabWidth(Tabs_[static_cast<std::size_t>(currentIndex)]) + Style_.TabSpacing;
    }
    const float tabEnd = tabStart + ComputeTabWidth(Tabs_[static_cast<std::size_t>(index)]);

    if (tabStart < TabScrollOffset_) {
        TabScrollOffset_ = tabStart;
    } else if (tabEnd > TabScrollOffset_ + viewportWidth) {
        TabScrollOffset_ = tabEnd - viewportWidth;
    }
}

float ImTabView::ComputeTabWidth(const FTabViewItem& item) const
{
    float tabWidth = Style_.TabPadding.Left + Style_.TabPadding.Right + MeasureTextWidth(ResolveTabTitle(item));
        if (item.Icon.IsValid()) {
            tabWidth += Style_.IconSize + 6.0f;
        }
    if (item.bDirty) {
        tabWidth += Style_.DirtyMarkerRadius * 2.0f + 6.0f;
    }
    if (item.bClosable) {
        tabWidth += Style_.CloseButtonSize + 6.0f;
    }

    return std::max(0.0f, tabWidth);
}

float ImTabView::GetTabsViewportWidth() const
{
    float width = TabStripGeometry_.Size.X;
    if (LeftOverflowButtonGeometry_.IsValid()) {
        width -= LeftOverflowButtonGeometry_.Size.X;
    }
    if (RightOverflowButtonGeometry_.IsValid()) {
        width -= RightOverflowButtonGeometry_.Size.X;
    }
    if (LeftOverflowButtonGeometry_.IsValid() && RightOverflowButtonGeometry_.IsValid()) {
        width -= Style_.TabSpacing;
    }
    return std::max(0.0f, width);
}

} // namespace ImWidgetV4
