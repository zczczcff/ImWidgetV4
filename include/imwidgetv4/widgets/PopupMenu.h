#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/Image.h>
#include <functional>
#include <string>
#include <vector>

namespace ImWidgetV4 {

struct FPopupMenuItem {
    std::string Text;
    FImageBrush Icon;
    bool bEnabled = true;
    bool bIsSeparator = false;
    std::function<void()> OnInvoked;
};

struct FPopupMenuStyle {
    FColor BackgroundColor = FColor::FromBytes(26, 31, 38);
    FColor BorderColor = FColor::FromBytes(63, 73, 89);
    FColor RowHoveredColor = FColor::FromBytes(48, 60, 77);
    FColor RowPressedColor = FColor::FromBytes(69, 101, 154);
    FColor TextColor = FColor::FromBytes(238, 242, 247);
    FColor DisabledTextColor = FColor::FromBytes(128, 134, 143);
    FColor SeparatorColor = FColor::FromBytes(57, 66, 80);
    float FontSize = 14.0f;
    float RowHeight = 28.0f;
    float IconSize = 18.0f;
    float HorizontalPadding = 12.0f;
    float IconTextSpacing = 8.0f;
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
    virtual ~ImPopupMenu() = default;

    void SetItems(const std::vector<FPopupMenuItem>& items);
    void SetItems(std::vector<FPopupMenuItem>&& items);
    const std::vector<FPopupMenuItem>& GetItems() const { return Items_; }

    void SetStyle(const FPopupMenuStyle& style);
    const FPopupMenuStyle& GetStyle() const { return Style_; }

    FItemInvokedEvent OnItemInvoked;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

private:
    int ResolveIndexAt(const FVector2& position) const;
    bool IsInteractiveIndex(int index) const;
    float MeasureTextWidth(const std::string& text) const;
    void ClearInteractionState();

    std::vector<FPopupMenuItem> Items_;
    FPopupMenuStyle Style_;
    int HoveredItemIndex_ = -1;
    int PressedItemIndex_ = -1;
};

} // namespace ImWidgetV4
