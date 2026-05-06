#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

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

} // namespace

ImTabView::ImTabView()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

int ImTabView::AddTab(const std::string& title, const std::shared_ptr<ImWidget>& content)
{
    return AddTab(title, FImageBrush(), content);
}

int ImTabView::AddTab(const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content)
{
    FTabViewItem item;
    item.Title = title;
    item.Icon = icon;
    item.Content = content;
    Tabs_.push_back(std::move(item));

    const int addedIndex = static_cast<int>(Tabs_.size() - 1);
    bLayoutDirty_ = true;

    if (ActiveTabIndex_ < 0 && Tabs_[static_cast<std::size_t>(addedIndex)].bEnabled) {
        SetActiveTab(addedIndex);
    } else {
        Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    }

    return addedIndex;
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

    if (Tabs_.empty()) {
        ActiveTabIndex_ = -1;
    } else if (bRemovingActive) {
        ActiveTabIndex_ = ResolveReplacementActiveIndex(index);
    } else if (ActiveTabIndex_ > index) {
        --ActiveTabIndex_;
    }

    HoveredTabIndex_ = HoveredTabIndex_ == index ? -1 : (HoveredTabIndex_ > index ? HoveredTabIndex_ - 1 : HoveredTabIndex_);
    PressedTabIndex_ = PressedTabIndex_ == index ? -1 : (PressedTabIndex_ > index ? PressedTabIndex_ - 1 : PressedTabIndex_);
    bLayoutDirty_ = true;
    UpdateRegisteredActiveContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (ActiveTabIndex_ != previousActiveIndex) {
        OnActiveTabChanged.Broadcast(*this, ActiveTabIndex_);
    }
    return true;
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
        UpdateRegisteredActiveContent();
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (ActiveTabIndex_ != previousActiveIndex) {
        OnActiveTabChanged.Broadcast(*this, ActiveTabIndex_);
    }
    return true;
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
        Style_.BorderColor,
        Style_.CornerRadius,
        Style_.BorderThickness);

    if (HasKeyboardFocus()) {
        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin() + FVector2(2.0f, 2.0f),
            m_Geometry.GetMax() - FVector2(2.0f, 2.0f),
            Style_.FocusedOutlineColor,
            std::max(0.0f, Style_.CornerRadius - 2.0f),
            1.5f);
    }

    if (TabStripGeometry_.IsValid()) {
        paintContext.DrawContext_.DrawRectFilled(
            TabStripGeometry_.GetMin(),
            TabStripGeometry_.GetMax(),
            Style_.TabStripBackgroundColor,
            Style_.CornerRadius);
        paintContext.DrawContext_.PushClipRect(TabStripGeometry_.GetMin(), TabStripGeometry_.GetMax(), true);

        for (const FTabGeometry& tabGeometry : VisibleTabGeometries_) {
            const FTabViewItem& item = Tabs_[static_cast<std::size_t>(tabGeometry.Index)];
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

            const FColor contentColor = ResolveTabTextColor(tabGeometry.Index);
            float contentX = tabGeometry.Geometry.Position.X + Style_.TabPadding.Left;
            if (item.Icon.IsValid()) {
                const float iconSize = std::min(Style_.IconSize, std::max(0.0f, tabGeometry.Geometry.Size.Y - Style_.TabPadding.Top - Style_.TabPadding.Bottom));
                const float iconY = tabGeometry.Geometry.Position.Y +
                    std::max(0.0f, (tabGeometry.Geometry.Size.Y - iconSize) * 0.5f);
                paintContext.DrawContext_.DrawImage(
                    item.Icon.TextureId,
                    FVector2(contentX, iconY),
                    FVector2(contentX + iconSize, iconY + iconSize),
                    item.Icon.Uv0,
                    item.Icon.Uv1,
                    contentColor);
                contentX += iconSize + 6.0f;
            }

            const float textY = tabGeometry.Geometry.Position.Y +
                std::max(0.0f, (tabGeometry.Geometry.Size.Y - MeasureTextHeight()) * 0.5f);
            paintContext.DrawContext_.DrawText(
                FVector2(contentX, textY),
                contentColor,
                item.Title,
                Style_.FontSize);
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
        const int hoveredTab = ResolveTabIndexAt(event.MousePosition);
        if (HoveredTabIndex_ != hoveredTab) {
            HoveredTabIndex_ = hoveredTab;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    }

    case EInputEventType::MouseLeave:
        if (HoveredTabIndex_ != -1) {
            HoveredTabIndex_ = -1;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonDown:
        if (event.MouseButton == EMouseButton::Left) {
            const int tabIndex = ResolveTabIndexAt(event.MousePosition);
            if (tabIndex >= 0 && IsTabEnabled(tabIndex)) {
                PressedTabIndex_ = tabIndex;
                HoveredTabIndex_ = tabIndex;
                Invalidate(EInvalidateReason::Paint);
                return FReply::Handled()
                    .SetKeyboardFocus(shared_from_this())
                    .CaptureMouse(shared_from_this(), EMouseButton::Left);
            }
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonUp:
        if (event.MouseButton == EMouseButton::Left && PressedTabIndex_ >= 0) {
            const int pressedTab = PressedTabIndex_;
            PressedTabIndex_ = -1;
            const int releasedTab = ResolveTabIndexAt(event.MousePosition);
            Invalidate(EInvalidateReason::Paint);

            if (pressedTab == releasedTab && IsTabEnabled(pressedTab)) {
                OnTabInvoked.Broadcast(*this, pressedTab);
                SetActiveTab(pressedTab);
            }

            return FReply::Handled().ReleaseMouseCapture();
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

void ImTabView::Relayout()
{
    if (!bLayoutDirty_) {
        return;
    }

    UpdateRegisteredActiveContent();

    const FGeometry innerGeometry = GetInnerGeometry();
    const float stripHeight = std::min(Style_.TabHeight, innerGeometry.Size.Y);
    TabStripGeometry_ = FGeometry(
        innerGeometry.Position,
        FVector2(innerGeometry.Size.X, stripHeight));
    ContentGeometry_ = FGeometry(
        FVector2(innerGeometry.Position.X, innerGeometry.Position.Y + stripHeight),
        FVector2(innerGeometry.Size.X, std::max(0.0f, innerGeometry.Size.Y - stripHeight)));

    VisibleTabGeometries_.clear();
    float cursorX = TabStripGeometry_.Position.X;
    const float stripMaxX = TabStripGeometry_.Position.X + TabStripGeometry_.Size.X;

    for (int index = 0; index < static_cast<int>(Tabs_.size()); ++index) {
        const FTabViewItem& item = Tabs_[static_cast<std::size_t>(index)];
        float tabWidth = Style_.TabPadding.Left + Style_.TabPadding.Right + MeasureTextWidth(item.Title);
        if (item.Icon.IsValid()) {
            tabWidth += Style_.IconSize + 6.0f;
        }
        tabWidth = std::max(tabWidth, Style_.TabMinWidth);

        if (cursorX + tabWidth > stripMaxX) {
            break;
        }

        VisibleTabGeometries_.push_back(FTabGeometry {
            index,
            FGeometry(
                FVector2(cursorX, TabStripGeometry_.Position.Y),
                FVector2(tabWidth, TabStripGeometry_.Size.Y))
        });
        cursorX += tabWidth + Style_.TabSpacing;
    }

    if (const std::shared_ptr<ImWidget> activeContent = GetActiveContent()) {
        activeContent->SetGeometry(ContentGeometry_);
    }

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
    if (!TabStripGeometry_.Contains(position)) {
        return -1;
    }

    for (const FTabGeometry& tabGeometry : VisibleTabGeometries_) {
        if (tabGeometry.Geometry.Contains(position)) {
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

int ImTabView::ResolveReplacementActiveIndex(int removedIndex) const
{
    if (Tabs_.empty()) {
        return -1;
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

} // namespace ImWidgetV4
