#include <gtest/gtest.h>
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/TextBlock.h>

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

    auto textAlignment = textBlock.GetPropertyAsOptional("TextAlignment");
    auto verticalAlignment = textBlock.GetPropertyAsOptional("VerticalAlignment");
    EXPECT_TRUE(textAlignment.IsValid());
    EXPECT_TRUE(verticalAlignment.IsValid());
    EXPECT_EQ(textAlignment.GetOptionString(), "Left");
    EXPECT_EQ(verticalAlignment.GetOptionString(), "Top");
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
    EXPECT_EQ(serialized["Properties"]["ImButton::Text"], "Confirm");
    EXPECT_EQ(serialized["Properties"]["ImButton::Disabled"], true);
    ASSERT_TRUE(serialized["Properties"]["ImButton::Style"].is_object());

    const json& styleJson = serialized["Properties"]["ImButton::Style"]["Properties"];
    EXPECT_EQ(styleJson["FButtonStyle::Normal"]["Properties"]["FButtonStateStyle::CornerRadius"], 11.0f);
    EXPECT_EQ(styleJson["FButtonStyle::Hovered"]["Properties"]["FButtonStateStyle::HasBorder"], true);

    auto restored = std::make_shared<ImButton>();
    restored->FromJson(serialized);

    EXPECT_EQ(restored->GetName(), "ConfirmButton");
    EXPECT_EQ(restored->GetText(), "Confirm");
    EXPECT_TRUE(restored->IsDisabled());
    EXPECT_FLOAT_EQ(restored->GetStyle().Normal.CornerRadius, 11.0f);
    EXPECT_TRUE(restored->GetStyle().Hovered.bHasBorder);
    EXPECT_EQ(
        restored->GetStyle().Disabled.TextColor.ToImU32(),
        FColor::FromBytes(90, 91, 92, 255).ToImU32());
}
