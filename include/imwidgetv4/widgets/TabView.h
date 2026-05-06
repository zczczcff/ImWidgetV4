#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/Image.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

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
        .RegisterProperty(PropertyType::Struct, "Padding", &FTabViewStyle::Padding, "Outer padding")
        .RegisterProperty(PropertyType::Struct, "TabPadding", &FTabViewStyle::TabPadding, "Tab padding")
        .RegisterProperty(PropertyType::Float, "TabSpacing", &FTabViewStyle::TabSpacing, "Spacing between tabs")
        .RegisterProperty(PropertyType::Float, "TabMinWidth", &FTabViewStyle::TabMinWidth, "Minimum tab width")
        .RegisterProperty(PropertyType::Float, "TabHeight", &FTabViewStyle::TabHeight, "Tab strip item height")
        .RegisterProperty(PropertyType::Float, "IconSize", &FTabViewStyle::IconSize, "Tab icon size")
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
    FMargin Padding = FMargin(6.0f);
    FMargin TabPadding = FMargin(12.0f, 12.0f, 6.0f, 6.0f);
    float TabSpacing = 4.0f;
    float TabMinWidth = 96.0f;
    float TabHeight = 32.0f;
    float IconSize = 16.0f;
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
        .RegisterProperty(PropertyType::Struct, "Style", &ImTabView::Style_, "Tab view style");
    END_DECLARE_OBJECT()

public:
    using FTabEvent = TMulticastDelegate<ImTabView&, int>;

    ImTabView();
    virtual ~ImTabView() = default;

    int AddTab(const std::string& title, const std::shared_ptr<ImWidget>& content);
    int AddTab(const std::string& title, const FImageBrush& icon, const std::shared_ptr<ImWidget>& content);
    bool RemoveTab(int index);
    void ClearTabs();
    int GetTabCount() const { return static_cast<int>(Tabs_.size()); }

    bool SetActiveTab(int index);
    int GetActiveTabIndex() const { return ActiveTabIndex_; }
    std::shared_ptr<ImWidget> GetActiveContent() const;
    const FTabViewItem* GetTab(int index) const;
    bool SetTabEnabled(int index, bool bEnabled);

    void SetStyle(const FTabViewStyle& style);
    const FTabViewStyle& GetStyle() const { return Style_; }

    FTabEvent OnActiveTabChanged;
    FTabEvent OnTabInvoked;

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
    };

    void SetActiveTabProperty(int& index);
    int& GetActiveTabProperty();

    void Relayout();
    void UpdateRegisteredActiveContent();
    void CleanupInteractionStateForContent(const std::shared_ptr<ImWidget>& content);
    bool ContainsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget, const std::shared_ptr<ImWidget>& subtreeRoot) const;
    int ResolveTabIndexAt(const FVector2& position) const;
    bool HandleKeyboardNavigation(const FInputEvent& event);
    bool IsValidIndex(int index) const;
    bool IsTabEnabled(int index) const;
    int FindNextEnabledTab(int startIndex, int direction, bool bWrap) const;
    int FindFirstEnabledTab() const;
    int FindLastEnabledTab() const;
    int ResolveReplacementActiveIndex(int removedIndex) const;
    float MeasureTextWidth(const std::string& text) const;
    float MeasureTextHeight() const;
    FGeometry GetInnerGeometry() const;
    FColor ResolveTabBackgroundColor(int index) const;
    FColor ResolveTabTextColor(int index) const;
    bool HasFocusWithinTabView() const;

    FTabViewStyle Style_;
    std::vector<FTabViewItem> Tabs_;
    std::vector<FTabGeometry> VisibleTabGeometries_;
    FGeometry TabStripGeometry_;
    FGeometry ContentGeometry_;
    int ActiveTabIndex_ = -1;
    int HoveredTabIndex_ = -1;
    int PressedTabIndex_ = -1;
    int ReflectedActiveTabIndex_ = -1;
    int RegisteredActiveTabIndex_ = -2;
    bool bLayoutDirty_ = true;
};

} // namespace ImWidgetV4
