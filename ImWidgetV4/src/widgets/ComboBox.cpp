#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

constexpr int InvalidComboIndex = -1;

float Clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float MeasureTextWidthWithFont(const std::string& text, float fontSize)
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str()).x;
    }

    return fontSize * 0.55f * static_cast<float>(text.size());
}

float ResolveContentInset(float borderThickness)
{
    return std::max(0.0f, borderThickness);
}

FGeometry InsetGeometryByBorder(const FGeometry& geometry, float borderThickness)
{
    const float borderInset = ResolveContentInset(borderThickness);
    return FGeometry(
        FVector2(geometry.Position.X + borderInset, geometry.Position.Y + borderInset),
        FVector2(
            std::max(0.0f, geometry.Size.X - borderInset * 2.0f),
            std::max(0.0f, geometry.Size.Y - borderInset * 2.0f)));
}

} // namespace

const Reflection::FTypeDesc& FComboBoxStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::BackgroundColor>(
            "FComboBoxStyle",
            "BackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Background color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::HoveredBackgroundColor>(
            "FComboBoxStyle",
            "HoveredBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Hovered background color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PressedBackgroundColor>(
            "FComboBoxStyle",
            "PressedBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Pressed background color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::DisabledBackgroundColor>(
            "FComboBoxStyle",
            "DisabledBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled background color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::BorderColor>(
            "FComboBoxStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::FocusedOutlineColor>(
            "FComboBoxStyle",
            "FocusedOutlineColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Focused outline color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::TextColor>(
            "FComboBoxStyle",
            "TextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Text color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PlaceholderTextColor>(
            "FComboBoxStyle",
            "PlaceholderTextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Placeholder text color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::DisabledTextColor>(
            "FComboBoxStyle",
            "DisabledTextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled text color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::ArrowColor>(
            "FComboBoxStyle",
            "ArrowColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Arrow color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PopupRowHoveredColor>(
            "FComboBoxStyle",
            "PopupRowHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Popup row hovered color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PopupRowSelectedColor>(
            "FComboBoxStyle",
            "PopupRowSelectedColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Popup row selected color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PopupRowSelectedHoveredColor>(
            "FComboBoxStyle",
            "PopupRowSelectedHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Popup row selected hovered color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FColor, &FComboBoxStyle::PopupOutlineColor>(
            "FComboBoxStyle",
            "PopupOutlineColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Popup outline color"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FMargin, &FComboBoxStyle::Padding>(
            "FComboBoxStyle",
            "Padding",
            Reflection::EPropertyKind::Struct,
            "FMargin",
            "Content padding",
            &FMargin::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::FontSize>(
            "FComboBoxStyle",
            "FontSize",
            Reflection::EPropertyKind::Float,
            "float",
            "Font size"),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::BorderThickness>(
            "FComboBoxStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::CornerRadius>(
            "FComboBoxStyle",
            "CornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Corner radius"),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::ArrowSize>(
            "FComboBoxStyle",
            "ArrowSize",
            Reflection::EPropertyKind::Float,
            "float",
            "Arrow size"),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::PopupItemHeight>(
            "FComboBoxStyle",
            "PopupItemHeight",
            Reflection::EPropertyKind::Float,
            "float",
            "Popup item height"),
        Reflection::MakeMemberProperty<FComboBoxStyle, float, &FComboBoxStyle::PopupMaxVisibleItems>(
            "FComboBoxStyle",
            "PopupMaxVisibleItems",
            Reflection::EPropertyKind::Float,
            "float",
            "Popup max visible items"),
        Reflection::MakeMemberProperty<FComboBoxStyle, FVector2, &FComboBoxStyle::MinDesiredSize>(
            "FComboBoxStyle",
            "MinDesiredSize",
            Reflection::EPropertyKind::Vec2,
            "FVector2",
            "Minimum desired size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FComboBoxStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImComboBox::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeObjectAccessorProperty<ImComboBox, std::vector<std::string>, &ImComboBox::SetItemsProperty, &ImComboBox::GetItemsProperty>(
            "ImComboBox",
            "Items",
            Reflection::EPropertyKind::StringArray,
            "std::vector<std::string>",
            "Available combo box items"),
        Reflection::MakeObjectAccessorProperty<ImComboBox, int, &ImComboBox::SetSelectedIndexProperty, &ImComboBox::GetSelectedIndexProperty>(
            "ImComboBox",
            "SelectedIndex",
            Reflection::EPropertyKind::Int,
            "int",
            "Selected item index"),
        Reflection::MakeObjectAccessorProperty<ImComboBox, std::string, &ImComboBox::SetPlaceholderTextProperty, &ImComboBox::GetPlaceholderTextProperty>(
            "ImComboBox",
            "PlaceholderText",
            Reflection::EPropertyKind::String,
            "std::string",
            "Placeholder text"),
        Reflection::MakeObjectAccessorProperty<ImComboBox, int, &ImComboBox::SetMaxVisibleItemsProperty, &ImComboBox::GetMaxVisibleItemsProperty>(
            "ImComboBox",
            "MaxVisibleItems",
            Reflection::EPropertyKind::Int,
            "int",
            "Maximum visible popup items"),
        Reflection::MakeMemberProperty<ImComboBox, bool, &ImComboBox::m_bDisabled>(
            "ImComboBox",
            "Disabled",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the combo box is disabled"),
        Reflection::MakeMemberProperty<ImComboBox, FComboBoxStyle, &ImComboBox::m_Style>(
            "ImComboBox",
            "Style",
            Reflection::EPropertyKind::Struct,
            "FComboBoxStyle",
            "Combo box style",
            &FComboBoxStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImComboBox",
        &ImWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FComboBoxStyleReflectionTypeDesc = FComboBoxStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImComboBoxReflectionTypeDesc = ImComboBox::StaticTypeDesc();

} // namespace

class ImComboPopupList : public ImWidget {
public:
    explicit ImComboPopupList(const std::shared_ptr<ImComboBox>& owner)
        : Owner_(owner)
    {
        SetHitTestVisible(true);
    }

    virtual void Paint(const FPaintContext& paintContext) override
    {
        const std::shared_ptr<ImComboBox> owner = Owner_.lock();
        if (!owner) {
            return;
        }

        const FComboBoxStyle& style = owner->GetEffectiveStyle();
        const float itemHeight = style.PopupItemHeight;
        const float scrollOffset = owner->m_PopupScrollOffset;
        const int firstVisibleIndex = std::max(0, static_cast<int>(scrollOffset / itemHeight));
        const int lastVisibleIndex = std::min(
            static_cast<int>(owner->m_Items.size()),
            firstVisibleIndex + owner->m_MaxVisibleItems + 1);

        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin(),
            m_Geometry.GetMax(),
            style.PopupOutlineColor,
            style.CornerRadius,
            style.BorderThickness);

        const FGeometry contentGeometry = InsetGeometryByBorder(m_Geometry, style.BorderThickness);
        paintContext.DrawContext_.PushClipRect(contentGeometry.GetMin(), contentGeometry.GetMax(), true);
        for (int index = firstVisibleIndex; index < lastVisibleIndex; ++index) {
            const float rowY = contentGeometry.Position.Y + static_cast<float>(index) * itemHeight - scrollOffset;
            const FVector2 rowMin(contentGeometry.Position.X, rowY);
            const FVector2 rowMax(contentGeometry.Position.X + contentGeometry.Size.X, rowY + itemHeight);

            if (rowMax.Y <= contentGeometry.Position.Y ||
                rowMin.Y >= contentGeometry.Position.Y + contentGeometry.Size.Y) {
                continue;
            }

            const bool bSelected = owner->m_SelectedIndex == index;
            const bool bHovered = owner->m_HoveredPopupIndex == index || owner->m_HighlightedIndex == index;
            FColor rowColor = FColor::Transparent;
            if (bSelected) {
                rowColor = bHovered ? style.PopupRowSelectedHoveredColor : style.PopupRowSelectedColor;
            } else if (bHovered) {
                rowColor = style.PopupRowHoveredColor;
            }

            if (rowColor.A > 0.0f) {
                paintContext.DrawContext_.DrawRectFilled(
                    rowMin,
                    rowMax,
                    rowColor,
                    0.0f);
            }

            const float textY = rowMin.Y + std::max(0.0f, (itemHeight - style.FontSize) * 0.5f);
            paintContext.DrawContext_.DrawText(
                FVector2(rowMin.X + style.Padding.Left, textY),
                style.TextColor,
                owner->ResolveItemText(index),
                style.FontSize);
        }
        paintContext.DrawContext_.PopClipRect();
    }

    virtual FVector2 GetMinSize() const override
    {
        const std::shared_ptr<ImComboBox> owner = Owner_.lock();
        if (!owner) {
            return FVector2(0.0f, 0.0f);
        }

        const FGeometry ownerGeometry = owner->GetGeometry();
        return FVector2(
            ownerGeometry.Size.X,
            owner->ResolvePopupHeight());
    }

    virtual FReply OnInputEvent(const FInputEvent& event) override
    {
        const std::shared_ptr<ImComboBox> owner = Owner_.lock();
        if (!owner) {
            return FReply::Unhandled();
        }

        if (event.Type == EInputEventType::MouseMove) {
            owner->m_HoveredPopupIndex = ResolveIndexAt(event.MousePosition);
            if (owner->m_HoveredPopupIndex != InvalidComboIndex) {
                owner->m_HighlightedIndex = owner->m_HoveredPopupIndex;
                owner->EnsurePopupSelectionVisible();
            }
            owner->Invalidate(EInvalidateReason::Paint);
            return FReply::Handled();
        }

        if (event.Type == EInputEventType::MouseLeave) {
            owner->m_HoveredPopupIndex = InvalidComboIndex;
            owner->m_PressedPopupIndex = InvalidComboIndex;
            owner->Invalidate(EInvalidateReason::Paint);
            return FReply::Handled();
        }

        if (event.Type == EInputEventType::MouseWheel) {
            if (owner->m_Items.size() > static_cast<std::size_t>(owner->m_MaxVisibleItems)) {
                owner->m_PopupScrollOffset -= event.ScrollDelta.Y * owner->GetEffectiveStyle().PopupItemHeight;
                owner->ClampPopupScrollOffset();
                owner->Invalidate(EInvalidateReason::Paint);
            }
            return FReply::Handled();
        }

        if (event.Type == EInputEventType::MouseButtonDown &&
            event.MouseButton == EMouseButton::Left) {
            owner->m_PressedPopupIndex = ResolveIndexAt(event.MousePosition);
            if (owner->m_PressedPopupIndex != InvalidComboIndex) {
                owner->m_HighlightedIndex = owner->m_PressedPopupIndex;
                owner->Invalidate(EInvalidateReason::Paint);
                return FReply::Handled();
            }
        }

        if (event.Type == EInputEventType::MouseButtonUp &&
            event.MouseButton == EMouseButton::Left) {
            const int releasedIndex = ResolveIndexAt(event.MousePosition);
            const int pressedIndex = owner->m_PressedPopupIndex;
            owner->m_PressedPopupIndex = InvalidComboIndex;
            if (pressedIndex != InvalidComboIndex && pressedIndex == releasedIndex) {
                owner->SetSelectedIndexInternal(releasedIndex, true);
                if (Owner_.lock() == owner) {
                    owner->ClosePopup();
                }
                return FReply::Handled();
            }
            if (Owner_.lock() == owner) {
                owner->Invalidate(EInvalidateReason::Paint);
            }
        }

        return FReply::Unhandled();
    }

private:
    int ResolveIndexAt(const FVector2& position) const
    {
        const std::shared_ptr<ImComboBox> owner = Owner_.lock();
        if (!owner || owner->m_Items.empty()) {
            return InvalidComboIndex;
        }

        const FComboBoxStyle& style = owner->GetEffectiveStyle();
        const FGeometry contentGeometry = InsetGeometryByBorder(m_Geometry, style.BorderThickness);
        if (!contentGeometry.Contains(position)) {
            return InvalidComboIndex;
        }

        const float localY = position.Y - contentGeometry.Position.Y + owner->m_PopupScrollOffset;
        const int index = static_cast<int>(localY / style.PopupItemHeight);
        if (index < 0 || index >= static_cast<int>(owner->m_Items.size())) {
            return InvalidComboIndex;
        }

        return index;
    }

    std::weak_ptr<ImComboBox> Owner_;
};

ImComboBox::ImComboBox()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

ImComboBox::~ImComboBox()
{
    if (m_PopupWindow && m_PopupWindow->IsOpen()) {
        m_PopupWindow->Close();
    }

    m_bPopupOpen = false;
    m_PopupWindow.reset();
    m_PopupList.reset();
}

void ImComboBox::SetItems(const std::vector<std::string>& items)
{
    m_Items = items;
    SyncLocalizedItemsFromSerializableItems();
    if (m_SelectedIndex >= static_cast<int>(m_Items.size())) {
        m_SelectedIndex = InvalidComboIndex;
    }
    if (m_HighlightedIndex >= static_cast<int>(m_Items.size())) {
        m_HighlightedIndex = m_SelectedIndex >= 0 ? m_SelectedIndex : (m_Items.empty() ? InvalidComboIndex : 0);
    }
    m_PopupScrollOffset = 0.0f;
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetItems(const std::vector<FText>& items)
{
    m_Items.clear();
    m_ItemTexts = items;
    m_Items.reserve(items.size());
    for (const FText& item : items) {
        m_Items.push_back(item.IsLocalized()
            ? (item.GetDefaultText().empty() ? item.GetKey() : item.GetDefaultText())
            : item.GetInvariantText());
    }

    if (m_SelectedIndex >= static_cast<int>(m_Items.size())) {
        m_SelectedIndex = InvalidComboIndex;
    }
    if (m_HighlightedIndex >= static_cast<int>(m_Items.size())) {
        m_HighlightedIndex = m_SelectedIndex >= 0 ? m_SelectedIndex : (m_Items.empty() ? InvalidComboIndex : 0);
    }
    m_PopupScrollOffset = 0.0f;
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::AddItem(const std::string& item)
{
    m_Items.push_back(item);
    m_ItemTexts.push_back(FText::FromString(item));
    if (m_HighlightedIndex == InvalidComboIndex) {
        m_HighlightedIndex = 0;
    }
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::AddItem(const FText& item)
{
    m_ItemTexts.push_back(item);
    m_Items.push_back(item.IsLocalized()
        ? (item.GetDefaultText().empty() ? item.GetKey() : item.GetDefaultText())
        : item.GetInvariantText());
    if (m_HighlightedIndex == InvalidComboIndex) {
        m_HighlightedIndex = 0;
    }
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::ClearItems()
{
    m_Items.clear();
    m_ItemTexts.clear();
    m_SelectedIndex = InvalidComboIndex;
    m_HighlightedIndex = InvalidComboIndex;
    m_HoveredPopupIndex = InvalidComboIndex;
    m_PressedPopupIndex = InvalidComboIndex;
    m_PopupScrollOffset = 0.0f;
    ClosePopup();
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetSelectedIndex(int index)
{
    SetSelectedIndexInternal(index, true);
}

std::string ImComboBox::GetSelectedText() const
{
    if (!HasSelection()) {
        return {};
    }

    return ResolveItemText(m_SelectedIndex);
}

void ImComboBox::ClearSelection()
{
    SetSelectedIndexInternal(InvalidComboIndex, true);
}

void ImComboBox::SetPlaceholderText(const std::string& text)
{
    if (m_PlaceholderText == text) {
        return;
    }

    m_PlaceholderText = text;
    m_PlaceholderTextValue = FText::FromString(text);
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetPlaceholderText(const FText& text)
{
    if (m_PlaceholderTextValue == text) {
        return;
    }

    m_PlaceholderTextValue = text;
    m_PlaceholderText = text.IsLocalized()
        ? (text.GetDefaultText().empty() ? text.GetKey() : text.GetDefaultText())
        : text.GetInvariantText();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetMaxVisibleItems(int count)
{
    const int clamped = std::max(1, count);
    if (m_MaxVisibleItems == clamped) {
        return;
    }

    m_MaxVisibleItems = clamped;
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetStyle(const FComboBoxStyle& style)
{
    m_Style = style;
    m_bHasExplicitStyle = true;
    if (m_PopupWindow && m_PopupWindow->IsOpen()) {
        m_PopupWindow->SetStyle(BuildPopupWindowStyle());
    }
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImComboBox::SetDisabled(bool bDisabled)
{
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    if (m_bDisabled) {
        ClosePopup();
    }
    Invalidate(EInvalidateReason::Paint);
}

bool ImComboBox::IsPopupOpen() const
{
    return m_bPopupOpen && m_PopupWindow && m_PopupWindow->IsOpen();
}

void ImComboBox::OpenPopup()
{
    SyncPopupStateFromWindow();
    if (m_bDisabled || m_Items.empty() || IsPopupOpen() || GetApplication() == nullptr) {
        return;
    }

    ImWindowManager& windowManager = GetApplication()->GetWindowManager();
    FPopupOptions popupOptions;
    popupOptions.Title = "ComboPopup";
    popupOptions.Position = m_Geometry.Position;
    popupOptions.Size = FVector2(m_Geometry.Size.X, ResolvePopupHeight());
    popupOptions.ParentWindow = windowManager.FindWindowForWidget(shared_from_this());
    popupOptions.Style = BuildPopupWindowStyle();
    popupOptions.Style.bDrawShadow = true;

    std::shared_ptr<ImComboBox> self = std::static_pointer_cast<ImComboBox>(shared_from_this());
    if (!m_PopupList) {
        m_PopupList = std::make_shared<ImComboPopupList>(self);
    }

    popupOptions.RootWidget = m_PopupList;
    m_PopupWindow = windowManager.CreatePopup(popupOptions);
    m_bPopupOpen = m_PopupWindow != nullptr;
    m_HoveredPopupIndex = InvalidComboIndex;
    m_PressedPopupIndex = InvalidComboIndex;
    m_HighlightedIndex = HasSelection() ? m_SelectedIndex : (m_Items.empty() ? InvalidComboIndex : 0);
    m_PopupScrollOffset = 0.0f;
    UpdatePopupWindowLayout();
    EnsurePopupSelectionVisible();
    if (m_bPopupOpen) {
        OnPopupOpened.Broadcast(*this);
    }
    Invalidate(EInvalidateReason::Paint);
}

void ImComboBox::ClosePopup()
{
    SyncPopupStateFromWindow();
    if (!m_bPopupOpen) {
        return;
    }

    const std::shared_ptr<ImWindow> popupWindow = m_PopupWindow;
    m_PopupWindow.reset();

    if (popupWindow) {
        popupWindow->Close();
    }

    m_bPopupOpen = false;
    m_HoveredPopupIndex = InvalidComboIndex;
    m_PressedPopupIndex = InvalidComboIndex;
    OnPopupClosed.Broadcast(*this);
    Invalidate(EInvalidateReason::Paint);
}

void ImComboBox::TogglePopup()
{
    if (IsPopupOpen()) {
        ClosePopup();
    } else {
        OpenPopup();
    }
}

void ImComboBox::Paint(const FPaintContext& paintContext)
{
    SyncPopupStateFromWindow();
    if (!m_bVisible) {
        return;
    }

    if (IsPopupOpen()) {
        UpdatePopupWindowLayout();
    }

    const FComboBoxStyle& style = GetEffectiveStyle();

    FColor background = style.BackgroundColor;
    if (m_bDisabled) {
        background = style.DisabledBackgroundColor;
    } else if (m_bPressed || IsPopupOpen()) {
        background = style.PressedBackgroundColor;
    } else if (m_bHovered) {
        background = style.HoveredBackgroundColor;
    }

    const FColor outline = HasKeyboardFocus() ? style.FocusedOutlineColor : style.BorderColor;
    const std::string displayText = HasSelection() ? GetSelectedText() : ResolvePlaceholderText();
    const FColor textColor = HasSelection()
        ? (m_bDisabled ? style.DisabledTextColor : style.TextColor)
        : style.PlaceholderTextColor;
    const float borderInset = ResolveContentInset(style.BorderThickness);
    const float textY = m_Geometry.Position.Y +
        borderInset +
        std::max(0.0f, (m_Geometry.Size.Y - borderInset * 2.0f - style.FontSize) * 0.5f);
    const float arrowHalf = style.ArrowSize * 0.5f;
    const FVector2 arrowCenter(
        m_Geometry.Position.X + m_Geometry.Size.X - borderInset - style.Padding.Right - arrowHalf,
        m_Geometry.Position.Y + m_Geometry.Size.Y * 0.5f);

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        background,
        style.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        outline,
        style.CornerRadius,
        style.BorderThickness);

    const float textRight = arrowCenter.X - style.ArrowSize - 10.0f;
    const FVector2 clipMin(
        m_Geometry.Position.X + borderInset + style.Padding.Left,
        m_Geometry.Position.Y + borderInset);
    const FVector2 clipMax(
        textRight,
        m_Geometry.Position.Y + m_Geometry.Size.Y - borderInset);
    paintContext.DrawContext_.PushClipRect(clipMin, clipMax, true);
    paintContext.DrawContext_.DrawText(
        FVector2(m_Geometry.Position.X + borderInset + style.Padding.Left, textY),
        textColor,
        displayText,
        style.FontSize);
    paintContext.DrawContext_.PopClipRect();

    const FColor arrowColor = m_bDisabled ? style.DisabledTextColor : style.ArrowColor;
    const float direction = IsPopupOpen() ? -1.0f : 1.0f;
    paintContext.DrawContext_.PathLineTo(FVector2(arrowCenter.X - arrowHalf, arrowCenter.Y - 3.0f * direction));
    paintContext.DrawContext_.PathLineTo(FVector2(arrowCenter.X + arrowHalf, arrowCenter.Y - 3.0f * direction));
    paintContext.DrawContext_.PathLineTo(FVector2(arrowCenter.X, arrowCenter.Y + 3.0f * direction));
    paintContext.DrawContext_.PathFill(arrowColor);
}

FVector2 ImComboBox::GetMinSize() const
{
    const FComboBoxStyle& style = GetEffectiveStyle();
    float longestTextWidth = MeasureTextWidth(ResolvePlaceholderText());
    for (int index = 0; index < static_cast<int>(m_Items.size()); ++index) {
        longestTextWidth = std::max(longestTextWidth, MeasureTextWidth(ResolveItemText(index)));
    }

    const float borderInset = ResolveContentInset(style.BorderThickness);

    return FVector2(
        std::max(
            style.MinDesiredSize.X,
            borderInset * 2.0f + style.Padding.Left + longestTextWidth + style.Padding.Right + style.ArrowSize + 18.0f),
        std::max(
            style.MinDesiredSize.Y,
            borderInset * 2.0f + style.Padding.Top + style.FontSize + style.Padding.Bottom));
}

FReply ImComboBox::OnInputEvent(const FInputEvent& event)
{
    SyncPopupStateFromWindow();
    if (m_bDisabled) {
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseEnter) {
        SetHovered(true);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        SetHovered(false);
        SetPressed(false);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        SetHovered(true);
        SetPressed(true);
        TogglePopup();
        return FReply::Handled().SetKeyboardFocus(shared_from_this());
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_bPressed) {
        SetPressed(false);
        return FReply::Handled();
    }

    if (!HasKeyboardFocus() || event.Type != EInputEventType::KeyDown) {
        return FReply::Unhandled();
    }

    switch (event.Key) {
    case EKey::Enter:
    case EKey::Space:
        if (IsPopupOpen()) {
            CommitHighlightedItem();
        } else {
            OpenPopup();
        }
        return FReply::Handled();
    case EKey::Escape:
        if (IsPopupOpen()) {
            ClosePopup();
            return FReply::Handled();
        }
        break;
    case EKey::Down:
        if (!IsPopupOpen()) {
            OpenPopup();
            MoveHighlight(1);
        } else {
            MoveHighlight(1);
        }
        return FReply::Handled();
    case EKey::Up:
        if (!IsPopupOpen()) {
            OpenPopup();
            MoveHighlight(-1);
        } else {
            MoveHighlight(-1);
        }
        return FReply::Handled();
    default:
        break;
    }

    return FReply::Unhandled();
}

void ImComboBox::OnFocusChanged(bool bHasFocus)
{
    ImWidget::OnFocusChanged(bHasFocus);
    if (!bHasFocus && !IsPopupOpen()) {
        SetPressed(false);
    }
}

void ImComboBox::SyncPopupStateFromWindow()
{
    const std::shared_ptr<ImWindow> popupWindow = m_PopupWindow;
    if (popupWindow && !popupWindow->IsOpen() && m_bPopupOpen) {
        m_bPopupOpen = false;
        m_PopupWindow.reset();
        m_HoveredPopupIndex = InvalidComboIndex;
        m_PressedPopupIndex = InvalidComboIndex;
        OnPopupClosed.Broadcast(*this);
        Invalidate(EInvalidateReason::Paint);
    }
}

void ImComboBox::EnsurePopupSelectionVisible()
{
    if (m_HighlightedIndex == InvalidComboIndex) {
        return;
    }

    const float itemHeight = GetEffectiveStyle().PopupItemHeight;
    const float popupHeight = ResolvePopupContentHeight();
    const float itemTop = static_cast<float>(m_HighlightedIndex) * itemHeight;
    const float itemBottom = itemTop + itemHeight;

    if (itemTop < m_PopupScrollOffset) {
        m_PopupScrollOffset = itemTop;
    } else if (itemBottom > m_PopupScrollOffset + popupHeight) {
        m_PopupScrollOffset = itemBottom - popupHeight;
    }

    ClampPopupScrollOffset();
}

void ImComboBox::ClampPopupScrollOffset()
{
    const float itemHeight = GetEffectiveStyle().PopupItemHeight;
    const float popupHeight = ResolvePopupContentHeight();
    const float maxScroll = std::max(0.0f, static_cast<float>(m_Items.size()) * itemHeight - popupHeight);
    m_PopupScrollOffset = std::clamp(m_PopupScrollOffset, 0.0f, maxScroll);
}

void ImComboBox::UpdatePopupWindowLayout()
{
    if (!m_PopupWindow || !m_PopupWindow->IsOpen()) {
        return;
    }

    m_PopupWindow->SetStyle(BuildPopupWindowStyle());

    ImWindowManager& windowManager = GetApplication()->GetWindowManager();
    const std::shared_ptr<ImWindow> mainWindow = windowManager.GetMainWindow();
    const FGeometry viewportGeometry = mainWindow != nullptr
        ? mainWindow->GetWindowGeometry()
        : FGeometry(FVector2(0.0f, 0.0f), FVector2(1920.0f, 1080.0f));

    const float popupHeight = ResolvePopupHeight();
    const float spaceBelow = viewportGeometry.Position.Y + viewportGeometry.Size.Y - (m_Geometry.Position.Y + m_Geometry.Size.Y);
    const float popupY = spaceBelow >= popupHeight
        ? (m_Geometry.Position.Y + m_Geometry.Size.Y)
        : (m_Geometry.Position.Y - popupHeight);

    m_PopupWindow->SetPosition(FVector2(m_Geometry.Position.X, popupY));
    m_PopupWindow->SetSize(FVector2(m_Geometry.Size.X, popupHeight));
}

void ImComboBox::RefreshPopupWindowContent()
{
    if (m_PopupList) {
        m_PopupList->Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    }
    if (m_PopupWindow && m_PopupWindow->IsOpen()) {
        UpdatePopupWindowLayout();
    }
}

void ImComboBox::SetHovered(bool bHovered)
{
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);
}

void ImComboBox::SetPressed(bool bPressed)
{
    if (m_bPressed == bPressed) {
        return;
    }

    m_bPressed = bPressed;
    Invalidate(EInvalidateReason::Paint);
}

void ImComboBox::SetSelectedIndexInternal(int index, bool bBroadcast)
{
    const int clampedIndex =
        (index >= 0 && index < static_cast<int>(m_Items.size())) ? index : InvalidComboIndex;
    if (m_SelectedIndex == clampedIndex) {
        return;
    }

    m_SelectedIndex = clampedIndex;
    m_HighlightedIndex = clampedIndex >= 0 ? clampedIndex : (m_Items.empty() ? InvalidComboIndex : 0);
    EnsurePopupSelectionVisible();
    Invalidate(EInvalidateReason::Paint);
    if (bBroadcast) {
        OnSelectionChanged.Broadcast(*this, m_SelectedIndex);
    }
}

void ImComboBox::MoveSelection(int direction)
{
    if (m_Items.empty()) {
        return;
    }

    if (m_SelectedIndex == InvalidComboIndex) {
        SetSelectedIndexInternal(direction < 0 ? static_cast<int>(m_Items.size()) - 1 : 0, true);
        return;
    }

    SetSelectedIndexInternal(
        std::clamp(m_SelectedIndex + direction, 0, static_cast<int>(m_Items.size()) - 1),
        true);
}

void ImComboBox::MoveHighlight(int direction)
{
    if (m_Items.empty()) {
        return;
    }

    if (m_HighlightedIndex == InvalidComboIndex) {
        m_HighlightedIndex = HasSelection()
            ? m_SelectedIndex
            : (direction < 0 ? static_cast<int>(m_Items.size()) - 1 : 0);
    } else {
        m_HighlightedIndex = std::clamp(
            m_HighlightedIndex + direction,
            0,
            static_cast<int>(m_Items.size()) - 1);
    }

    EnsurePopupSelectionVisible();
    RefreshPopupWindowContent();
    Invalidate(EInvalidateReason::Paint);
}

void ImComboBox::CommitHighlightedItem()
{
    if (m_HighlightedIndex == InvalidComboIndex) {
        return;
    }

    SetSelectedIndexInternal(m_HighlightedIndex, true);
    ClosePopup();
}

void ImComboBox::SetPopupHighlightedIndex(int index)
{
    if (m_HighlightedIndex == index) {
        return;
    }

    m_HighlightedIndex = index;
    EnsurePopupSelectionVisible();
    RefreshPopupWindowContent();
}

float ImComboBox::MeasureTextWidth(const std::string& text) const
{
    return MeasureTextWidthWithFont(text, GetEffectiveStyle().FontSize);
}

std::string ImComboBox::ResolveItemText(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_Items.size())) {
        return {};
    }

    const std::size_t itemIndex = static_cast<std::size_t>(index);
    if (itemIndex < m_ItemTexts.size()) {
        const FText& itemText = m_ItemTexts[itemIndex];
        if (itemText.IsLocalized() || !itemText.GetInvariantText().empty()) {
            return itemText.Resolve();
        }
    }

    return m_Items[itemIndex];
}

std::string ImComboBox::ResolvePlaceholderText() const
{
    if (m_PlaceholderTextValue.IsLocalized() || !m_PlaceholderTextValue.GetInvariantText().empty()) {
        return m_PlaceholderTextValue.Resolve();
    }

    return m_PlaceholderText;
}

void ImComboBox::SyncLocalizedItemsFromSerializableItems()
{
    m_ItemTexts.clear();
    m_ItemTexts.reserve(m_Items.size());
    for (const std::string& item : m_Items) {
        m_ItemTexts.push_back(FText::FromString(item));
    }
}

float ImComboBox::ResolvePopupHeight() const
{
    const FComboBoxStyle& style = GetEffectiveStyle();
    const float visibleItems = static_cast<float>(std::min(m_MaxVisibleItems, static_cast<int>(m_Items.size())));
    const float borderInset = ResolveContentInset(style.BorderThickness);
    return std::max(
        style.PopupItemHeight + borderInset * 2.0f,
        visibleItems * style.PopupItemHeight + borderInset * 2.0f);
}

float ImComboBox::ResolvePopupContentHeight() const
{
    const float borderInset = ResolveContentInset(GetEffectiveStyle().BorderThickness);
    return std::max(0.0f, ResolvePopupHeight() - borderInset * 2.0f);
}

const FComboBoxStyle& ImComboBox::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveComboBoxStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

FWindowStyle ImComboBox::BuildPopupWindowStyle() const
{
    const FComboBoxStyle& style = GetEffectiveStyle();
    FWindowStyle windowStyle;
    windowStyle.BackgroundColor = style.BackgroundColor;
    windowStyle.InactiveBackgroundColor = style.BackgroundColor;
    windowStyle.BorderColor = style.PopupOutlineColor;
    windowStyle.ActiveBorderColor = style.PopupOutlineColor;
    windowStyle.CornerRadius = style.CornerRadius;
    windowStyle.BorderThickness = style.BorderThickness;
    return windowStyle;
}

} // namespace ImWidgetV4
