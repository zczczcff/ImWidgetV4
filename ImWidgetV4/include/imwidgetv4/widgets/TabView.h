#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/Image.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

enum class ETabCloseActivationPolicy {
    LeftNeighbor = 0,
    MostRecentlyActive = 1
};

enum class ETabStripPlacement {
    Top = 0,
    Bottom = 1
};

struct FTabViewStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FTabViewStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FTabViewStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FTabViewStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FTabViewStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "TabStripBackgroundColor", &FTabViewStyle::TabStripBackgroundColor, "Tab strip background color")
        .RegisterProperty(PropertyType::Color, "TabColor", &FTabViewStyle::TabColor, "Inactive tab color")
        .RegisterProperty(PropertyType::Color, "TabHoveredColor", &FTabViewStyle::TabHoveredColor, "Hovered tab color")
        .RegisterProperty(PropertyType::Color, "TabPressedColor", &FTabViewStyle::TabPressedColor, "Pressed tab color")
        .RegisterProperty(PropertyType::Color, "ActiveTabColor", &FTabViewStyle::ActiveTabColor, "Active tab color")
        .RegisterProperty(PropertyType::Color, "DisabledTabColor", &FTabViewStyle::DisabledTabColor, "Disabled tab color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FTabViewStyle::TextColor, "Inactive text color")
        .RegisterProperty(PropertyType::Color, "ActiveTextColor", &FTabViewStyle::ActiveTextColor, "Active text color")
        .RegisterProperty(PropertyType::Color, "DisabledTextColor", &FTabViewStyle::DisabledTextColor, "Disabled text color")
        .RegisterProperty(PropertyType::Color, "TabBorderColor", &FTabViewStyle::TabBorderColor, "Tab border color")
        .RegisterProperty(PropertyType::Color, "DirtyMarkerColor", &FTabViewStyle::DirtyMarkerColor, "Dirty marker color")
        .RegisterProperty(PropertyType::Color, "CloseButtonColor", &FTabViewStyle::CloseButtonColor, "Close button color")
        .RegisterProperty(PropertyType::Color, "CloseButtonHoveredColor", &FTabViewStyle::CloseButtonHoveredColor, "Hovered close button color")
        .RegisterProperty(PropertyType::Color, "CloseButtonPressedColor", &FTabViewStyle::CloseButtonPressedColor, "Pressed close button color")
        .RegisterProperty(PropertyType::Color, "OverflowButtonColor", &FTabViewStyle::OverflowButtonColor, "Overflow button color")
        .RegisterProperty(PropertyType::Color, "OverflowButtonHoveredColor", &FTabViewStyle::OverflowButtonHoveredColor, "Hovered overflow button color")
        .RegisterProperty(PropertyType::Color, "OverflowButtonPressedColor", &FTabViewStyle::OverflowButtonPressedColor, "Pressed overflow button color")
        .RegisterProperty(PropertyType::Color, "OverflowButtonDisabledColor", &FTabViewStyle::OverflowButtonDisabledColor, "Disabled overflow button color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FTabViewStyle::Padding, "Outer padding")
        .RegisterProperty(PropertyType::Struct, "TabPadding", &FTabViewStyle::TabPadding, "Tab padding")
        .RegisterProperty(PropertyType::Float, "TabSpacing", &FTabViewStyle::TabSpacing, "Spacing between tabs")
        .RegisterProperty(PropertyType::Float, "TabMinWidth", &FTabViewStyle::TabMinWidth, "Minimum tab width")
        .RegisterProperty(PropertyType::Float, "TabHeight", &FTabViewStyle::TabHeight, "Tab strip item height")
        .RegisterProperty(PropertyType::Float, "IconSize", &FTabViewStyle::IconSize, "Tab icon size")
        .RegisterProperty(PropertyType::Float, "DirtyMarkerRadius", &FTabViewStyle::DirtyMarkerRadius, "Dirty marker radius")
        .RegisterProperty(PropertyType::Float, "CloseButtonSize", &FTabViewStyle::CloseButtonSize, "Close button size")
        .RegisterProperty(PropertyType::Float, "OverflowButtonWidth", &FTabViewStyle::OverflowButtonWidth, "Overflow button width")
        .RegisterProperty(PropertyType::Float, "FontSize", &FTabViewStyle::FontSize, "Tab font size")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FTabViewStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FTabViewStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FTabViewStyle::MinDesiredSize, "Minimum desired size");
    END_DECLARE_OBJECT()

public:
    FColor BackgroundColor = FColor::FromBytes(20, 24, 30);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor TabStripBackgroundColor = FColor::FromBytes(26, 31, 39);
    FColor TabColor = FColor::FromBytes(44, 51, 61);
    FColor TabHoveredColor = FColor::FromBytes(56, 66, 80);
    FColor TabPressedColor = FColor::FromBytes(35, 43, 52);
    FColor ActiveTabColor = FColor::FromBytes(64, 88, 123);
    FColor DisabledTabColor = FColor::FromBytes(39, 44, 51);
    FColor TextColor = FColor::FromBytes(214, 222, 234);
    FColor ActiveTextColor = FColor::White;
    FColor DisabledTextColor = FColor::FromBytes(126, 132, 141);
    FColor TabBorderColor = FColor::FromBytes(18, 22, 28);
    FColor DirtyMarkerColor = FColor::FromBytes(255, 196, 84);
    FColor CloseButtonColor = FColor::FromBytes(182, 190, 202);
    FColor CloseButtonHoveredColor = FColor::White;
    FColor CloseButtonPressedColor = FColor::FromBytes(255, 214, 102);
    FColor OverflowButtonColor = FColor::FromBytes(88, 102, 119);
    FColor OverflowButtonHoveredColor = FColor::FromBytes(122, 143, 168);
    FColor OverflowButtonPressedColor = FColor::FromBytes(156, 182, 212);
    FColor OverflowButtonDisabledColor = FColor::FromBytes(72, 78, 86);
    FMargin Padding = FMargin(6.0f);
    FMargin TabPadding = FMargin(12.0f, 12.0f, 6.0f, 6.0f);
    float TabSpacing = 4.0f;
    float TabMinWidth = 96.0f;
    float TabHeight = 32.0f;
    float IconSize = 16.0f;
    float DirtyMarkerRadius = 4.0f;
    float CloseButtonSize = 12.0f;
    float OverflowButtonWidth = 24.0f;
    float FontSize = 14.0f;
    float BorderThickness = 1.0f;
    float CornerRadius = 6.0f;
    FVector2 MinDesiredSize {280.0f, 220.0f};
};

struct FTabViewItem {
    std::string Title;
    FImageBrush Icon;
    std::shared_ptr<ImWidget> Content;
    bool bEnabled = true;
    bool bClosable = false;
    bool bDirty = false;
};

class ImTabView : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImTabView, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::Int,
            "ActiveTabIndex",
            static_cast<void (ImTabView::*)(int&)>(&ImTabView::SetActiveTabProperty),
            static_cast<int& (ImTabView::*)()>(&ImTabView::GetActiveTabProperty),
            "Active tab index")
        .RegisterOptionalProperty(
            PropertyType::Enum,
            "CloseActivationPolicy",
            static_cast<void (ImTabView::*)(int&)>(&ImTabView::SetCloseActivationPolicyProperty),
            static_cast<int& (ImTabView::*)()>(&ImTabView::GetCloseActivationPolicyProperty),
            {"LeftNeighbor", "MostRecentlyActive"},
            "Policy used to choose the next active tab after closing the active tab")
        .RegisterOptionalProperty(
            PropertyType::Enum,
            "TabStripPlacement",
            static_cast<void (ImTabView::*)(int&)>(&ImTabView::SetTabStripPlacementProperty),
            static_cast<int& (ImTabView::*)()>(&ImTabView::GetTabStripPlacementProperty),
            {"Top", "Bottom"},
            "Placement of the tab strip relative to the content area")
        .RegisterProperty(PropertyType::Struct, "Style", &ImTabView::Style_, "Tab view style");
    END_DECLARE_OBJECT()

public:
    using FTabEvent = TMulticastDelegate<ImTabView&, int>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImTabView&, int, FVector2>;

    ImTabView();
    virtual ~ImTabView() = default;

    int AddTab(const std::string& title, const std::shared_ptr<ImWidget>& content);
    int AddTab(const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const std::string& title, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    bool RemoveTab(int index);
    void ClearTabs();
    int GetTabCount() const { return static_cast<int>(Tabs_.size()); }

    bool SetActiveTab(int index);
    int GetActiveTabIndex() const { return ActiveTabIndex_; }
    std::shared_ptr<ImWidget> GetActiveContent() const;
    const FTabViewItem* GetTab(int index) const;
    bool SetTabEnabled(int index, bool bEnabled);
    bool SetTabClosable(int index, bool bClosable);
    bool IsTabClosable(int index) const;
    bool SetTabDirty(int index, bool bDirty);
    bool IsTabDirty(int index) const;
    bool SetTabTitle(int index, const std::string& title);
    bool SetTabIcon(int index, const FImageBrush& icon);
    void SetCloseActivationPolicy(ETabCloseActivationPolicy policy);
    ETabCloseActivationPolicy GetCloseActivationPolicy() const { return CloseActivationPolicy_; }
    void SetTabStripPlacement(ETabStripPlacement placement);
    ETabStripPlacement GetTabStripPlacement() const { return TabStripPlacement_; }

    void SetStyle(const FTabViewStyle& style);
    const FTabViewStyle& GetStyle() const { return Style_; }

    FTabEvent OnActiveTabChanged;
    FTabEvent OnTabInvoked;
    FTabEvent OnTabDoubleClicked;
    FTabEvent OnTabClosed;
    FContextMenuRequestedEvent OnTabContextMenuRequested;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnPreviewInputEvent(const FInputEvent& event) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    struct FTabGeometry {
        int Index = -1;
        FGeometry Geometry;
        FGeometry CloseButtonGeometry;
        bool bShowsCloseButton = false;
        float ContentStartX = 0.0f;
        float TextClipMaxX = 0.0f;
        bool bTitleClipped = false;
    };

    void SetActiveTabProperty(int& index);
    int& GetActiveTabProperty();
    void SetCloseActivationPolicyProperty(int& value);
    int& GetCloseActivationPolicyProperty();
    void SetTabStripPlacementProperty(int& value);
    int& GetTabStripPlacementProperty();

    void Relayout();
    void UpdateRegisteredActiveContent();
    void NoteTabActivated(int index);
    void UpdateHoveredTitleToolTip();
    void CleanupInteractionStateForContent(const std::shared_ptr<ImWidget>& content);
    bool ContainsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget, const std::shared_ptr<ImWidget>& subtreeRoot) const;
    int ResolveTabIndexAt(const FVector2& position) const;
    int ResolveCloseButtonTabIndexAt(const FVector2& position) const;
    bool HandleKeyboardNavigation(const FInputEvent& event);
    bool IsValidIndex(int index) const;
    bool IsTabEnabled(int index) const;
    int FindNextEnabledTab(int startIndex, int direction, bool bWrap) const;
    int FindFirstEnabledTab() const;
    int FindLastEnabledTab() const;
    int FindMostRecentlyActiveTab() const;
    int ResolveReplacementActiveIndex(int removedIndex) const;
    float MeasureTextWidth(const std::string& text) const;
    float MeasureTextHeight() const;
    FGeometry GetInnerGeometry() const;
    FColor ResolveTabBackgroundColor(int index) const;
    FColor ResolveTabTextColor(int index) const;
    FColor ResolveCloseButtonColor(const FTabGeometry& tabGeometry) const;
    FColor ResolveOverflowButtonColor(int direction) const;
    bool HasFocusWithinTabView() const;
    bool CanScrollTabs(int direction) const;
    void ScrollTabs(int direction);
    void EnsureTabVisible(int index, float viewportWidth);
    float ComputeTabWidth(const FTabViewItem& item) const;
    float GetTabsViewportWidth() const;

    FTabViewStyle Style_;
    std::vector<FTabViewItem> Tabs_;
    std::vector<FTabGeometry> VisibleTabGeometries_;
    FGeometry TabStripGeometry_;
    FGeometry ContentGeometry_;
    FGeometry LeftOverflowButtonGeometry_;
    FGeometry RightOverflowButtonGeometry_;
    int ActiveTabIndex_ = -1;
    int HoveredTabIndex_ = -1;
    int PressedTabIndex_ = -1;
    int HoveredCloseTabIndex_ = -1;
    int PressedCloseTabIndex_ = -1;
    int HoveredOverflowDirection_ = 0;
    int PressedOverflowDirection_ = 0;
    int LastClickedTabIndex_ = -1;
    double LastClickTimestamp_ = -1.0;
    int ReflectedActiveTabIndex_ = -1;
    int RegisteredActiveTabIndex_ = -2;
    int ReflectedCloseActivationPolicy_ = 0;
    int ReflectedTabStripPlacement_ = 0;
    float TabScrollOffset_ = 0.0f;
    bool bEnsureActiveTabVisible_ = false;
    bool bLayoutDirty_ = true;
    bool bHasLastLayoutGeometry_ = false;
    FGeometry LastLayoutGeometry_;
    ETabCloseActivationPolicy CloseActivationPolicy_ = ETabCloseActivationPolicy::LeftNeighbor;
    ETabStripPlacement TabStripPlacement_ = ETabStripPlacement::Top;
    std::vector<int> ActivationHistory_;
};

} // namespace ImWidgetV4
