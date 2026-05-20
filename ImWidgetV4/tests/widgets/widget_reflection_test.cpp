#include <gtest/gtest.h>
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/DragDrop.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>

using namespace ImWidgetV4;

TEST(WidgetReflectionTest, TextBlockRegistersInheritedAndOwnProperties)
{
    ImTextBlock textBlock;

    EXPECT_TRUE(textBlock.HasProperty("Name", "ImWidget"));
    EXPECT_TRUE(textBlock.HasProperty("Visible", "ImWidget"));
    EXPECT_TRUE(textBlock.HasProperty("Text", "ImTextBlock"));
    EXPECT_TRUE(textBlock.HasProperty("TextColor", "ImTextBlock"));
    EXPECT_TRUE(textBlock.HasProperty("FontSize", "ImTextBlock"));
    EXPECT_TRUE(textBlock.HasProperty("WrapText", "ImTextBlock"));
    EXPECT_STREQ(textBlock.GetTypeDesc().Name, "ReflectableObject");
    const Reflection::FTypeDesc& widgetTypeDesc = ImWidget::StaticTypeDesc();
    const Reflection::FPropertyDesc* visibleDesc =
        Reflection::FindProperty(widgetTypeDesc, "Visible", "ImWidget");
    ASSERT_NE(visibleDesc, nullptr);
    Reflection::FPropertyHandle visibleProperty(&textBlock, visibleDesc);
    ASSERT_NE(visibleProperty.GetConstAs<bool>(), nullptr);
    EXPECT_TRUE(*visibleProperty.GetConstAs<bool>());

    auto textAlignment = textBlock.GetPropertyAsOptional("TextAlignment");
    auto verticalAlignment = textBlock.GetPropertyAsOptional("VerticalAlignment");
    EXPECT_TRUE(textAlignment.IsValid());
    EXPECT_TRUE(verticalAlignment.IsValid());
    EXPECT_EQ(textAlignment.GetOptionString(), "Center");
    EXPECT_EQ(verticalAlignment.GetOptionString(), "Center");
}

TEST(WidgetReflectionTest, TextBlockJsonRoundTripPreservesProperties)
{
    ImTextBlock original;
    original.SetName("Title");
    original.SetVisible(false);
    original.SetText("Hello Reflection");
    original.SetTextColor(FColor::FromBytes(12, 34, 56, 200));
    original.SetFontSize(22.0f);
    original.SetWrapText(true);
    original.SetTextAlignment(ETextAlignment::Right);
    original.SetVerticalAlignment(EVerticalAlignment::Bottom);

    json serialized = original.ToJson();

    EXPECT_EQ(serialized["Type"], "ImTextBlock");
    EXPECT_EQ(serialized["Properties"]["ImWidget::Name"], "Title");
    EXPECT_EQ(serialized["Properties"]["ImWidget::Visible"], false);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::Text"], "Hello Reflection");
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::FontSize"], 22.0f);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::WrapText"], true);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::TextAlignment"], "Right");
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::VerticalAlignment"], "Bottom");
    ASSERT_TRUE(serialized["Properties"]["ImTextBlock::TextColor"].is_array());
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::TextColor"][0], 12);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::TextColor"][1], 34);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::TextColor"][2], 56);
    EXPECT_EQ(serialized["Properties"]["ImTextBlock::TextColor"][3], 200);

    ImTextBlock restored;
    restored.FromJson(serialized);

    EXPECT_EQ(restored.GetName(), "Title");
    EXPECT_FALSE(restored.IsVisible());
    EXPECT_EQ(restored.GetText(), "Hello Reflection");
    EXPECT_FLOAT_EQ(restored.GetFontSize(), 22.0f);
    EXPECT_TRUE(restored.GetWrapText());
    EXPECT_EQ(restored.GetTextAlignment(), ETextAlignment::Right);
    EXPECT_EQ(restored.GetVerticalAlignment(), EVerticalAlignment::Bottom);
    EXPECT_EQ(restored.GetTextColor().ToImU32(), FColor::FromBytes(12, 34, 56, 200).ToImU32());
}

TEST(WidgetReflectionTest, SlotAndStyleClassesAreReflectable)
{
    ImPaddingSlot slot;
    slot.SetSlotPosition(FVector2(10.0f, 20.0f));
    slot.SetSlotSize(FVector2(200.0f, 80.0f));
    slot.PaddingLeft = 4.0f;
    slot.PaddingRight = 5.0f;
    slot.PaddingTop = 6.0f;
    slot.PaddingBottom = 7.0f;

    json slotJson = slot.ToJson();
    EXPECT_EQ(slotJson["Type"], "ImPaddingSlot");
    EXPECT_EQ(slotJson["Properties"]["ImSlot::SlotPosition"][0], 10.0f);
    EXPECT_EQ(slotJson["Properties"]["ImSlot::SlotPosition"][1], 20.0f);
    EXPECT_EQ(slotJson["Properties"]["ImSlot::SlotSize"][0], 200.0f);
    EXPECT_EQ(slotJson["Properties"]["ImSlot::SlotSize"][1], 80.0f);
    EXPECT_EQ(slotJson["Properties"]["ImPaddingSlot::PaddingLeft"], 4.0f);
    EXPECT_EQ(slotJson["Properties"]["ImPaddingSlot::PaddingBottom"], 7.0f);
    EXPECT_STREQ(slot.GetTypeDesc().Name, "ImPaddingSlot");
    const Reflection::FPropertyDesc* slotPositionDesc =
        Reflection::FindProperty(slot.GetTypeDesc(), "SlotPosition", "ImSlot");
    const Reflection::FPropertyDesc* paddingLeftDesc =
        Reflection::FindProperty(slot.GetTypeDesc(), "PaddingLeft", "ImPaddingSlot");
    ASSERT_NE(slotPositionDesc, nullptr);
    ASSERT_NE(paddingLeftDesc, nullptr);
    Reflection::FPropertyHandle slotPositionProperty(&slot, slotPositionDesc);
    Reflection::FPropertyHandle paddingLeftProperty(&slot, paddingLeftDesc);
    ASSERT_NE(slotPositionProperty.GetConstAs<FVector2>(), nullptr);
    ASSERT_NE(paddingLeftProperty.GetConstAs<float>(), nullptr);
    EXPECT_FLOAT_EQ(slotPositionProperty.GetConstAs<FVector2>()->X, 10.0f);
    EXPECT_FLOAT_EQ(*paddingLeftProperty.GetConstAs<float>(), 4.0f);

    FButtonStateStyle stateStyle;
    stateStyle.BackgroundColor = FColor::FromBytes(1, 2, 3, 255);
    stateStyle.BorderColor = FColor::FromBytes(4, 5, 6, 255);
    stateStyle.TextColor = FColor::FromBytes(7, 8, 9, 255);
    stateStyle.BorderThickness = 3.0f;
    stateStyle.CornerRadius = 9.0f;
    stateStyle.bHasBorder = true;

    json stateJson = stateStyle.ToJson();
    EXPECT_EQ(stateJson["Type"], "FButtonStateStyle");
    EXPECT_EQ(stateJson["Properties"]["FButtonStateStyle::BorderThickness"], 3.0f);
    EXPECT_EQ(stateJson["Properties"]["FButtonStateStyle::CornerRadius"], 9.0f);
    EXPECT_EQ(stateJson["Properties"]["FButtonStateStyle::HasBorder"], true);
    EXPECT_STREQ(stateStyle.GetTypeDesc().Name, "FButtonStateStyle");
    const Reflection::FPropertyDesc* cornerRadiusDesc =
        Reflection::FindProperty(stateStyle.GetTypeDesc(), "CornerRadius", "FButtonStateStyle");
    ASSERT_NE(cornerRadiusDesc, nullptr);
    Reflection::FPropertyHandle cornerRadiusProperty(&stateStyle, cornerRadiusDesc);
    ASSERT_NE(cornerRadiusProperty.GetConstAs<float>(), nullptr);
    EXPECT_FLOAT_EQ(*cornerRadiusProperty.GetConstAs<float>(), 9.0f);

    FButtonStyle buttonStyle;
    buttonStyle.Normal.CornerRadius = 12.0f;
    EXPECT_STREQ(buttonStyle.GetTypeDesc().Name, "FButtonStyle");
    const Reflection::FPropertyDesc* normalStyleDesc =
        Reflection::FindProperty(buttonStyle.GetTypeDesc(), "Normal", "FButtonStyle");
    ASSERT_NE(normalStyleDesc, nullptr);
    ASSERT_NE(normalStyleDesc->StructType, nullptr);
    EXPECT_STREQ(normalStyleDesc->StructType->Name, "FButtonStateStyle");
    Reflection::FPropertyHandle normalStyleProperty(&buttonStyle, normalStyleDesc);
    const FButtonStateStyle* normalStyle = normalStyleProperty.GetConstAs<FButtonStateStyle>();
    ASSERT_NE(normalStyle, nullptr);
    EXPECT_FLOAT_EQ(normalStyle->CornerRadius, 12.0f);
}

TEST(WidgetReflectionTest, ButtonJsonRoundTripIncludesNestedStyle)
{
    auto button = std::make_shared<ImButton>();
    button->SetName("ConfirmButton");
    button->SetText("Confirm");
    button->SetDisabled(true);

    FButtonStyle style = FButtonStyle::CreatePrimary();
    style.Normal.CornerRadius = 11.0f;
    style.Hovered.bHasBorder = true;
    style.Disabled.TextColor = FColor::FromBytes(90, 91, 92, 255);
    button->SetStyle(style);

    json serialized = button->ToJson();

    EXPECT_EQ(serialized["Type"], "ImButton");
    EXPECT_FALSE(serialized["Properties"].contains("ImButton::Text"));
    EXPECT_EQ(serialized["Properties"]["ImButton::Disabled"], true);
    ASSERT_TRUE(serialized["Properties"]["ImButton::Style"].is_object());

    const json& styleJson = serialized["Properties"]["ImButton::Style"]["Properties"];
    EXPECT_EQ(styleJson["FButtonStyle::Normal"]["Properties"]["FButtonStateStyle::CornerRadius"], 11.0f);
    EXPECT_EQ(styleJson["FButtonStyle::Hovered"]["Properties"]["FButtonStateStyle::HasBorder"], true);

    auto restored = std::make_shared<ImButton>();
    restored->FromJson(serialized);

    EXPECT_EQ(restored->GetName(), "ConfirmButton");
    EXPECT_TRUE(restored->GetText().empty());
    EXPECT_TRUE(restored->IsDisabled());
    EXPECT_FLOAT_EQ(restored->GetStyle().Normal.CornerRadius, 11.0f);
    EXPECT_TRUE(restored->GetStyle().Hovered.bHasBorder);
    EXPECT_EQ(
        restored->GetStyle().Disabled.TextColor.ToImU32(),
        FColor::FromBytes(90, 91, 92, 255).ToImU32());
    EXPECT_STREQ(restored->GetTypeDesc().Name, "ImButton");
    const Reflection::FPropertyDesc* disabledDesc =
        Reflection::FindProperty(restored->GetTypeDesc(), "Disabled", "ImButton");
    const Reflection::FPropertyDesc* styleDesc =
        Reflection::FindProperty(restored->GetTypeDesc(), "Style", "ImButton");
    const Reflection::FPropertyDesc* inheritedVisibleDesc =
        Reflection::FindProperty(restored->GetTypeDesc(), "Visible", "ImWidget");
    ASSERT_NE(disabledDesc, nullptr);
    ASSERT_NE(styleDesc, nullptr);
    ASSERT_NE(inheritedVisibleDesc, nullptr);
    ASSERT_NE(styleDesc->StructType, nullptr);
    EXPECT_STREQ(styleDesc->StructType->Name, "FButtonStyle");
    Reflection::FPropertyHandle disabledProperty(restored.get(), disabledDesc);
    ASSERT_NE(disabledProperty.GetConstAs<bool>(), nullptr);
    EXPECT_TRUE(*disabledProperty.GetConstAs<bool>());
}

TEST(WidgetReflectionTest, RemainingWidgetsStylesAndSlotsRegisterProperties)
{
    FMargin margin(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_TRUE(margin.HasProperty("Left", "FMargin"));
    EXPECT_TRUE(margin.HasProperty("Bottom", "FMargin"));
    EXPECT_STREQ(margin.GetTypeDesc().Name, "FMargin");
    const Reflection::FPropertyDesc* leftMarginDesc =
        Reflection::FindProperty(margin.GetTypeDesc(), "Left", "FMargin");
    ASSERT_NE(leftMarginDesc, nullptr);
    Reflection::FPropertyHandle leftMarginProperty(&margin, leftMarginDesc);
    ASSERT_NE(leftMarginProperty.GetConstAs<float>(), nullptr);
    EXPECT_FLOAT_EQ(*leftMarginProperty.GetConstAs<float>(), 1.0f);

    ImButton button;
    EXPECT_FALSE(button.HasProperty("Text", "ImButton"));

    ImBoxSlot boxSlot;
    EXPECT_TRUE(boxSlot.HasProperty("FillCoefficient", "ImBoxSlot"));

    ImHorizontalBox horizontalBox;
    ImVerticalBox verticalBox;
    EXPECT_TRUE(horizontalBox.HasProperty("Spacing", "ImHorizontalBox"));
    EXPECT_TRUE(verticalBox.HasProperty("Spacing", "ImVerticalBox"));

    FCheckBoxStyle checkBoxStyle;
    ImCheckBox checkBox;
    EXPECT_TRUE(checkBoxStyle.HasProperty("Padding", "FCheckBoxStyle"));
    EXPECT_TRUE(checkBox.HasProperty("Checked", "ImCheckBox"));
    EXPECT_TRUE(checkBox.HasProperty("Style", "ImCheckBox"));

    FEditableTextStyle editableTextStyle;
    ImEditableText editableText;
    EXPECT_TRUE(editableTextStyle.HasProperty("SelectionBackgroundColor", "FEditableTextStyle"));
    EXPECT_TRUE(editableText.HasProperty("Text", "ImEditableText"));
    EXPECT_TRUE(editableText.HasProperty("HintText", "ImEditableText"));

    FSliderStyle sliderStyle;
    ImSlider slider;
    EXPECT_TRUE(sliderStyle.HasProperty("ShowValueText", "FSliderStyle"));
    EXPECT_TRUE(slider.HasProperty("Value", "ImSlider"));
    EXPECT_TRUE(slider.HasProperty("MaxValue", "ImSlider"));

    FSwitchStyle switchStyle;
    ImSwitch switchWidget;
    EXPECT_TRUE(switchStyle.HasProperty("ThumbInset", "FSwitchStyle"));
    EXPECT_TRUE(switchWidget.HasProperty("Checked", "ImSwitch"));
    EXPECT_TRUE(switchWidget.HasProperty("Style", "ImSwitch"));

    FTitleBarStyle titleBarStyle;
    ImTitleBar titleBar;
    EXPECT_TRUE(titleBarStyle.HasProperty("DragRegionMinWidth", "FTitleBarStyle"));
    EXPECT_TRUE(titleBar.HasProperty("ShowSystemButtons", "ImTitleBar"));
    EXPECT_TRUE(titleBar.HasProperty("Style", "ImTitleBar"));

    FComboBoxStyle comboStyle;
    ImComboBox comboBox;
    EXPECT_TRUE(comboStyle.HasProperty("PopupRowSelectedColor", "FComboBoxStyle"));
    EXPECT_TRUE(comboBox.HasProperty("Items", "ImComboBox"));
    EXPECT_TRUE(comboBox.HasProperty("SelectedIndex", "ImComboBox"));

    FColorPickerStyle colorPickerStyle;
    ImColorPicker colorPicker;
    EXPECT_TRUE(colorPickerStyle.HasProperty("HueBarWidth", "FColorPickerStyle"));
    EXPECT_TRUE(colorPickerStyle.HasProperty("ShowAlphaBar", "FColorPickerStyle"));
    EXPECT_TRUE(colorPicker.HasProperty("Color", "ImColorPicker"));
    EXPECT_TRUE(colorPicker.HasProperty("Style", "ImColorPicker"));

    FDesignerSurfaceStyle designerSurfaceStyle;
    ImDesignerSurface designerSurface;
    EXPECT_TRUE(designerSurfaceStyle.HasProperty("TransformHandleSize", "FDesignerSurfaceStyle"));
    EXPECT_TRUE(designerSurface.HasProperty("Style", "ImDesignerSurface"));

    FScrollBoxStyle scrollStyle;
    ImScrollBox scrollBox;
    EXPECT_TRUE(scrollStyle.HasProperty("ScrollbarThumbColor", "FScrollBoxStyle"));
    EXPECT_TRUE(scrollBox.HasProperty("ScrollOffset", "ImScrollBox"));

    FExpandableBoxStyle expandableStyle;
    ImExpandableBox expandableBox;
    EXPECT_TRUE(expandableStyle.HasProperty("HeaderPadding", "FExpandableBoxStyle"));
    EXPECT_TRUE(expandableBox.HasProperty("Expanded", "ImExpandableBox"));

    ImCanvasPanelSlot canvasSlot;
    ImCanvasPanel canvasPanel;
    EXPECT_TRUE(canvasSlot.HasProperty("RelativePosition", "ImCanvasPanelSlot"));
    EXPECT_TRUE(canvasPanel.HasProperty("DesiredSize", "ImCanvasPanel"));

    FHorizontalSplitterStyle horizontalSplitterStyle;
    ImHorizontalSplitterSlot horizontalSplitterSlot;
    ImHorizontalSplitter horizontalSplitter;
    EXPECT_TRUE(horizontalSplitterStyle.HasProperty("BarWidth", "FHorizontalSplitterStyle"));
    EXPECT_TRUE(horizontalSplitterSlot.HasProperty("Ratio", "ImHorizontalSplitterSlot"));
    EXPECT_TRUE(horizontalSplitter.HasProperty("Style", "ImHorizontalSplitter"));

    FVerticalSplitterStyle verticalSplitterStyle;
    ImVerticalSplitterSlot verticalSplitterSlot;
    ImVerticalSplitter verticalSplitter;
    EXPECT_TRUE(verticalSplitterStyle.HasProperty("BarHeight", "FVerticalSplitterStyle"));
    EXPECT_TRUE(verticalSplitterSlot.HasProperty("MinSize", "ImVerticalSplitterSlot"));
    EXPECT_TRUE(verticalSplitter.HasProperty("Style", "ImVerticalSplitter"));

    ImImage image;
    EXPECT_TRUE(image.HasProperty("DesiredSize", "ImImage"));
    EXPECT_TRUE(image.HasProperty("StretchMode", "ImImage"));
    auto stretchMode = image.GetPropertyAsOptional("StretchMode");
    ASSERT_TRUE(stretchMode.IsValid());
    EXPECT_EQ(stretchMode.GetOptionString(), "KeepAspect");

    FTextListStyle textListStyle;
    ImTextList textList;
    EXPECT_TRUE(textListStyle.HasProperty("AutoScrollSpeed", "FTextListStyle"));
    EXPECT_TRUE(textList.HasProperty("Items", "ImTextList"));
    EXPECT_TRUE(textList.HasProperty("ScrollOffset", "ImTextList"));

    FTextOutlineViewStyle textOutlineStyle;
    ImTextOutlineView textOutlineView;
    ImTextOutlineItem textOutlineItem;
    EXPECT_TRUE(textOutlineStyle.HasProperty("RowHeight", "FTextOutlineViewStyle"));
    EXPECT_TRUE(textOutlineView.HasProperty("ScrollOffset", "ImTextOutlineView"));
    EXPECT_TRUE(textOutlineItem.HasProperty("Text", "ImTextOutlineItem"));
    EXPECT_TRUE(textOutlineItem.HasProperty("Expanded", "ImTextOutlineItem"));

    FOutlineViewStyle outlineStyle;
    ImOutlineView outlineView;
    ImOutlineItem outlineItem;
    EXPECT_TRUE(outlineStyle.HasProperty("RowMinHeight", "FOutlineViewStyle"));
    EXPECT_TRUE(outlineView.HasProperty("ScrollOffset", "ImOutlineView"));
    EXPECT_TRUE(outlineItem.HasProperty("Expanded", "ImOutlineItem"));

    FListViewStyle listViewStyle;
    ImListView listView;
    EXPECT_TRUE(listViewStyle.HasProperty("RowMinHeight", "FListViewStyle"));
    EXPECT_TRUE(listView.HasProperty("ItemCount", "ImListView"));
    EXPECT_TRUE(listView.HasProperty("SelectedIndex", "ImListView"));
    EXPECT_TRUE(listView.HasProperty("ScrollOffset", "ImListView"));

    FTabViewStyle tabViewStyle;
    ImTabView tabView;
    EXPECT_TRUE(tabViewStyle.HasProperty("TabMinWidth", "FTabViewStyle"));
    EXPECT_TRUE(tabViewStyle.HasProperty("CloseButtonSize", "FTabViewStyle"));
    EXPECT_TRUE(tabView.HasProperty("ActiveTabIndex", "ImTabView"));
    EXPECT_TRUE(tabView.HasProperty("CloseActivationPolicy", "ImTabView"));
    EXPECT_TRUE(tabView.HasProperty("Style", "ImTabView"));
}

TEST(WidgetReflectionTest, RemainingWidgetsSerializeExpectedEditableProperties)
{
    ImSlider slider;
    slider.SetRange(-10.0f, 10.0f);
    slider.SetValue(3.5f);
    slider.SetStep(0.5f);

    json sliderJson = slider.ToJson();
    EXPECT_EQ(sliderJson["Properties"]["ImSlider::MinValue"], -10.0f);
    EXPECT_EQ(sliderJson["Properties"]["ImSlider::MaxValue"], 10.0f);
    EXPECT_EQ(sliderJson["Properties"]["ImSlider::Value"], 3.5f);
    EXPECT_EQ(sliderJson["Properties"]["ImSlider::Step"], 0.5f);

    ImComboBox comboBox;
    comboBox.SetItems({"Alpha", "Beta", "Gamma"});
    comboBox.SetSelectedIndex(1);
    comboBox.SetPlaceholderText("Choose");

    json comboJson = comboBox.ToJson();
    ASSERT_TRUE(comboJson["Properties"]["ImComboBox::Items"].is_array());
    EXPECT_EQ(comboJson["Properties"]["ImComboBox::Items"][1], "Beta");
    EXPECT_EQ(comboJson["Properties"]["ImComboBox::SelectedIndex"], 1);
    EXPECT_EQ(comboJson["Properties"]["ImComboBox::PlaceholderText"], "Choose");

    ImImage image;
    image.SetStretchMode(EImageStretchMode::Fill);
    json imageJson = image.ToJson();
    EXPECT_EQ(imageJson["Properties"]["ImImage::StretchMode"], "Fill");

    ImImage restoredImage;
    restoredImage.FromJson(imageJson);
    EXPECT_EQ(restoredImage.GetStretchMode(), EImageStretchMode::Fill);
}

TEST(WidgetReflectionTest, StackAllocatedWidgetsBroadcastWithoutSharedOwnership)
{
    ImSlider slider;
    slider.SetRange(0.0f, 10.0f);
    float sliderValue = -1.0f;
    slider.OnValueChanged.AddLambda([&sliderValue](ImSlider&, float value) {
        sliderValue = value;
    });
    EXPECT_NO_THROW(slider.SetValue(4.0f));
    EXPECT_FLOAT_EQ(sliderValue, 4.0f);

    ImCheckBox checkBox;
    bool checkBoxState = false;
    checkBox.OnCheckStateChanged.AddLambda([&checkBoxState](ImCheckBox&, bool checked) {
        checkBoxState = checked;
    });
    EXPECT_NO_THROW(checkBox.SetChecked(true));
    EXPECT_TRUE(checkBoxState);

    ImSwitch switchWidget;
    bool switchState = false;
    switchWidget.OnCheckStateChanged.AddLambda([&switchState](ImSwitch&, bool checked) {
        switchState = checked;
    });
    EXPECT_NO_THROW(switchWidget.SetChecked(true));
    EXPECT_TRUE(switchState);
}

TEST(WidgetReflectionTest, DragDropPayloadIsReflectable)
{
    FDragDropPayload payload;
    json serialized = payload.ToJson();
    EXPECT_EQ(serialized["Type"], "FDragDropPayload");
}
