#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/Image.h>
#include <functional>
#include <string>
#include <vector>

namespace ImWidgetV4 {

struct FPopupMenuItem {
    std::string Text;
    FImageBrush Icon;
    std::vector<FPopupMenuItem> SubItems;
    bool bEnabled = true;
    bool bIsSeparator = false;
    std::function<void()> OnInvoked;
    FText TextValue;

    bool HasSubMenu() const { return !SubItems.empty(); }
};

struct FPopupMenuStyle {
    FColor BackgroundColor = FColor::FromBytes(26, 31, 38);
    FColor BorderColor = FColor::FromBytes(63, 73, 89);
    FColor RowHoveredColor = FColor::FromBytes(48, 60, 77);
    FColor RowPressedColor = FColor::FromBytes(69, 101, 154);
    FColor TextColor = FColor::FromBytes(238, 242, 247);
    FColor DisabledTextColor = FColor::FromBytes(128, 134, 143);
    FColor SeparatorColor = FColor::FromBytes(57, 66, 80);
    FColor SubmenuArrowColor = FColor::FromBytes(238, 242, 247);
    float FontSize = 14.0f;
    float RowHeight = 28.0f;
    float IconSize = 18.0f;
    float HorizontalPadding = 12.0f;
    float IconTextSpacing = 8.0f;
    float SubmenuIndicatorSpacing = 12.0f;
    float OuterPaddingX = 4.0f;
    float OuterPaddingY = 6.0f;
    float CornerRadius = 8.0f;
    float BorderThickness = 1.0f;
    FVector2 MinDesiredSize {180.0f, 36.0f};
};

class ImPopupMenu : public ImWidget {
public:
    using FItemInvokedEvent = TMulticastDelegate<ImPopupMenu&, int>;

    ImPopupMenu();
    virtual ~ImPopupMenu() override;

    void SetItems(const std::vector<FPopupMenuItem>& items);
    void SetItems(std::vector<FPopupMenuItem>&& items);
    const std::vector<FPopupMenuItem>& GetItems() const { return Items_; }

    void SetStyle(const FPopupMenuStyle& style);
    const FPopupMenuStyle& GetStyle() const { return GetEffectiveStyle(); }

    FItemInvokedEvent OnItemInvoked;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

private:
    struct FMenuMetrics {
        float MaxTextWidth = 0.0f;
        bool bHasAnyIcon = false;
        bool bHasAnySubMenu = false;
    };

    FGeometry GetContentGeometry() const;
    FGeometry GetRowGeometry(int index) const;
    int ResolveIndexAt(const FVector2& position) const;
    bool HasSubMenuAt(int index) const;
    bool IsInteractiveIndex(int index) const;
    const FPopupMenuStyle& GetEffectiveStyle() const;
    FWindowStyle BuildChildPopupWindowStyle() const;
    float MeasureTextWidth(const std::string& text) const;
    std::string ResolveItemText(const FPopupMenuItem& item) const;
    float ResolveSubmenuIndicatorWidth() const;
    FMenuMetrics ComputeMetrics() const;
    void SyncChildSubmenuState();
    void OpenChildSubmenu(int index);
    void CloseChildSubmenuChain();
    void RelayDescendantInvocation(ImPopupMenu& sender, int index);
    void ClearInteractionState();

    std::vector<FPopupMenuItem> Items_;
    FPopupMenuStyle Style_;
    mutable FPopupMenuStyle ResolvedThemeStyle_;
    int HoveredItemIndex_ = -1;
    int PressedItemIndex_ = -1;
    int ActiveSubMenuIndex_ = -1;
    std::shared_ptr<class ImPopupMenu> ActiveChildMenu_;
    std::shared_ptr<class ImWindow> ActiveChildWindow_;
    std::weak_ptr<class ImPopupMenu> ParentMenu_;
    bool bHasExplicitStyle_ = false;
};

} // namespace ImWidgetV4
