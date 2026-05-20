#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

class ImComboPopupList;

struct FComboBoxStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FComboBoxStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(31, 37, 46);
    FColor HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    FColor PressedBackgroundColor = FColor::FromBytes(24, 31, 40);
    FColor DisabledBackgroundColor = FColor::FromBytes(56, 60, 66);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor TextColor = FColor::FromBytes(238, 241, 245);
    FColor PlaceholderTextColor = FColor::FromBytes(135, 145, 157);
    FColor DisabledTextColor = FColor::FromBytes(140, 147, 156);
    FColor ArrowColor = FColor::FromBytes(220, 227, 235);
    FColor PopupRowHoveredColor = FColor::FromBytes(46, 58, 76);
    FColor PopupRowSelectedColor = FColor::FromBytes(78, 126, 196);
    FColor PopupRowSelectedHoveredColor = FColor::FromBytes(96, 149, 221);
    FColor PopupOutlineColor = FColor::FromBytes(16, 19, 23);
    FMargin Padding = FMargin(12.0f);
    float FontSize = 16.0f;
    float BorderThickness = 1.0f;
    float CornerRadius = 7.0f;
    float ArrowSize = 10.0f;
    float PopupItemHeight = 30.0f;
    float PopupMaxVisibleItems = 6.0f;
    FVector2 MinDesiredSize {180.0f, 38.0f};
};

class ImComboBox : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImComboBox"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FSelectionChangedEvent = TMulticastDelegate<ImComboBox&, int>;
    using FPopupEvent = TMulticastDelegate<ImComboBox&>;

    ImComboBox();
    virtual ~ImComboBox();

    void SetItems(const std::vector<std::string>& items);
    void SetItems(const std::vector<FText>& items);
    const std::vector<std::string>& GetItems() const { return m_Items; }
    void AddItem(const std::string& item);
    void AddItem(const FText& item);
    void ClearItems();

    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_SelectedIndex; }
    bool HasSelection() const { return m_SelectedIndex >= 0 && m_SelectedIndex < static_cast<int>(m_Items.size()); }
    std::string GetSelectedText() const;
    void ClearSelection();

    void SetPlaceholderText(const std::string& text);
    void SetPlaceholderText(const FText& text);
    const std::string& GetPlaceholderText() const { return m_PlaceholderText; }
    const FText& GetPlaceholderTextValue() const { return m_PlaceholderTextValue; }

    void SetMaxVisibleItems(int count);
    int GetMaxVisibleItems() const { return m_MaxVisibleItems; }

    void SetStyle(const FComboBoxStyle& style);
    const FComboBoxStyle& GetStyle() const { return GetEffectiveStyle(); }

    void SetDisabled(bool bDisabled);
    bool IsDisabled() const { return m_bDisabled; }
    bool IsHovered() const { return m_bHovered; }
    bool IsPressed() const { return m_bPressed; }
    bool IsPopupOpen() const;

    void OpenPopup();
    void ClosePopup();
    void TogglePopup();

    FSelectionChangedEvent OnSelectionChanged;
    FPopupEvent OnPopupOpened;
    FPopupEvent OnPopupClosed;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    friend class ImComboPopupList;

    void SetItemsProperty(std::vector<std::string>& items) { SetItems(items); }
    std::vector<std::string>& GetItemsProperty() { return m_Items; }
    void SetSelectedIndexProperty(int& index) { SetSelectedIndexInternal(index, false); }
    int& GetSelectedIndexProperty() { return m_SelectedIndex; }
    void SetPlaceholderTextProperty(std::string& text) { SetPlaceholderText(text); }
    std::string& GetPlaceholderTextProperty() { return m_PlaceholderText; }
    void SetMaxVisibleItemsProperty(int& count) { SetMaxVisibleItems(count); }
    int& GetMaxVisibleItemsProperty() { return m_MaxVisibleItems; }

    void SyncPopupStateFromWindow();
    void EnsurePopupSelectionVisible();
    void UpdatePopupWindowLayout();
    void RefreshPopupWindowContent();
    void SetHovered(bool bHovered);
    void SetPressed(bool bPressed);
    void SetSelectedIndexInternal(int index, bool bBroadcast);
    void MoveSelection(int direction);
    void MoveHighlight(int direction);
    void CommitHighlightedItem();
    void SetPopupHighlightedIndex(int index);
    const FComboBoxStyle& GetEffectiveStyle() const;
    FWindowStyle BuildPopupWindowStyle() const;
    void ClampPopupScrollOffset();
    std::string ResolveItemText(int index) const;
    std::string ResolvePlaceholderText() const;
    void SyncLocalizedItemsFromSerializableItems();
    float MeasureTextWidth(const std::string& text) const;
    float ResolvePopupHeight() const;
    float ResolvePopupContentHeight() const;

    FComboBoxStyle m_Style;
    mutable FComboBoxStyle m_ResolvedThemeStyle;
    std::vector<std::string> m_Items;
    std::vector<FText> m_ItemTexts;
    std::string m_PlaceholderText = "Select an option";
    FText m_PlaceholderTextValue = FText::FromString("Select an option");
    std::shared_ptr<ImWindow> m_PopupWindow;
    std::shared_ptr<ImComboPopupList> m_PopupList;
    int m_SelectedIndex = -1;
    int m_HighlightedIndex = -1;
    int m_HoveredPopupIndex = -1;
    int m_PressedPopupIndex = -1;
    int m_MaxVisibleItems = 6;
    float m_PopupScrollOffset = 0.0f;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
    bool m_bPopupOpen = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
