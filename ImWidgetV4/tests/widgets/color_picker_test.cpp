#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class ColorPickerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
            io.DisplaySize = ImVec2(1920.0f, 1080.0f);
            io.DeltaTime = 1.0f / 60.0f;
        }

        ImGui::NewFrame();

        App = std::make_shared<ImApplication>();
        Picker = std::make_shared<ImColorPicker>();
        Picker->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(240.0f, 180.0f)));
        App->SetRootWidget(Picker);
    }

    void TearDown() override
    {
        Picker.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    FInputEvent CreateMouseEvent(
        EInputEventType type,
        const FVector2& position,
        EMouseButton button = EMouseButton::Left)
    {
        FInputEvent event;
        event.Type = type;
        event.MousePosition = position;
        event.MouseButton = button;
        return event;
    }

    FInputEvent CreateKeyEvent(EKey key, const FInputModifiers& modifiers = {})
    {
        FInputEvent event;
        event.Type = EInputEventType::KeyDown;
        event.Key = key;
        event.Modifiers = modifiers;
        return event;
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events)
    {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(240.0f, 180.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImColorPicker> Picker;
};

TEST_F(ColorPickerTest, ReflectableColorRoundTripIsStable)
{
    Picker->SetColor(FColor::FromBytes(64, 128, 192, 210));
    json serialized = Picker->ToJson();

    EXPECT_EQ(serialized["Properties"]["ImColorPicker::Color"][0], 64);
    EXPECT_EQ(serialized["Properties"]["ImColorPicker::Color"][1], 128);
    EXPECT_EQ(serialized["Properties"]["ImColorPicker::Color"][2], 192);
    EXPECT_EQ(serialized["Properties"]["ImColorPicker::Color"][3], 210);

    ImColorPicker restored;
    restored.FromJson(serialized);
    EXPECT_EQ(restored.GetColor().ToImU32(), FColor::FromBytes(64, 128, 192, 210).ToImU32());
}

TEST_F(ColorPickerTest, MouseInteractionChangesAndCommitsColor)
{
    int changedCount = 0;
    int committedCount = 0;
    FColor lastChanged;
    FColor lastCommitted;

    Picker->OnColorChanged.AddLambda([&](ImColorPicker&, const FColor& color) {
        ++changedCount;
        lastChanged = color;
    });
    Picker->OnColorCommitted.AddLambda([&](ImColorPicker&, const FColor& color) {
        ++committedCount;
        lastCommitted = color;
    });

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f))});
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseMove, FVector2(140.0f, 120.0f))});
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(140.0f, 120.0f))});

    EXPECT_GT(changedCount, 0);
    EXPECT_EQ(committedCount, 1);
    EXPECT_EQ(lastCommitted.ToImU32(), Picker->GetColor().ToImU32());
    EXPECT_EQ(lastChanged.ToImU32(), Picker->GetColor().ToImU32());
}

TEST_F(ColorPickerTest, KeyboardAdjustmentChangesAndCommitsColor)
{
    App->SetKeyboardFocus(Picker);
    Picker->SetColor(FColor::FromBytes(255, 64, 64, 255));
    const ImU32 before = Picker->GetColor().ToImU32();

    AdvanceWithEvents({CreateKeyEvent(EKey::Right)});
    const ImU32 afterSaturationAdjust = Picker->GetColor().ToImU32();
    EXPECT_NE(afterSaturationAdjust, before);

    for (int index = 0; index < 8; ++index) {
        AdvanceWithEvents({CreateKeyEvent(EKey::Up, FInputModifiers(true, false, false, false))});
    }
    const ImU32 afterHueAdjust = Picker->GetColor().ToImU32();
    EXPECT_NE(afterHueAdjust, afterSaturationAdjust);

    AdvanceWithEvents({CreateKeyEvent(EKey::PageDown)});
    EXPECT_LT(Picker->GetColor().A, 1.0f);
}
