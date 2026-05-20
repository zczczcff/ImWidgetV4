#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FTabViewStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
    FText TitleText;
    FImageBrush Icon;
    std::shared_ptr<ImWidget> Content;
    bool bEnabled = true;
    bool bClosable = false;
    bool bDirty = false;
};

class ImTabView : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTabView"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FTabEvent = TMulticastDelegate<ImTabView&, int>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImTabView&, int, FVector2>;
    using FTabCloseRequestedEvent = TMulticastDelegate<ImTabView&, int, bool&>;

    ImTabView();
    virtual ~ImTabView() = default;

    int AddTab(const std::string& title, const std::shared_ptr<ImWidget>& content);
    int AddTab(const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    int AddTab(const FText& title, const std::shared_ptr<ImWidget>& content);
    int AddTab(const FText& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const std::string& title, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const FText& title, const std::shared_ptr<ImWidget>& content);
    int InsertTab(int index, const FText& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
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
    bool SetTabTitle(int index, const FText& title);
    bool SetTabIcon(int index, const FImageBrush& icon);
    void SetCloseActivationPolicy(ETabCloseActivationPolicy policy);
    ETabCloseActivationPolicy GetCloseActivationPolicy() const { return CloseActivationPolicy_; }
    void SetTabStripPlacement(ETabStripPlacement placement);
    ETabStripPlacement GetTabStripPlacement() const { return TabStripPlacement_; }

    void SetStyle(const FTabViewStyle& style);
    const FTabViewStyle& GetStyle() const { return GetEffectiveStyle(); }
    const FTabViewStyle& GetEffectiveStyle() const;

    FTabEvent OnActiveTabChanged;
    FTabEvent OnTabInvoked;
    FTabEvent OnTabDoubleClicked;
    FTabEvent OnTabClosed;
    FContextMenuRequestedEvent OnTabContextMenuRequested;
    FTabCloseRequestedEvent OnTabCloseRequested;

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
    bool RequestCloseTab(int index);
    bool HandleKeyboardNavigation(const FInputEvent& event);
    bool IsValidIndex(int index) const;
    bool IsTabEnabled(int index) const;
    int FindNextEnabledTab(int startIndex, int direction, bool bWrap) const;
    int FindFirstEnabledTab() const;
    int FindLastEnabledTab() const;
    int FindMostRecentlyActiveTab() const;
    int ResolveReplacementActiveIndex(int removedIndex) const;
    float MeasureTextWidth(const std::string& text) const;
    std::string ResolveTabTitle(const FTabViewItem& item) const;
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
    mutable FTabViewStyle ResolvedThemeStyle_;
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
    bool bHasExplicitStyle_ = false;
    bool bHasLastLayoutGeometry_ = false;
    FGeometry LastLayoutGeometry_;
    ETabCloseActivationPolicy CloseActivationPolicy_ = ETabCloseActivationPolicy::LeftNeighbor;
    ETabStripPlacement TabStripPlacement_ = ETabStripPlacement::Top;
    std::vector<int> ActivationHistory_;
};

} // namespace ImWidgetV4
