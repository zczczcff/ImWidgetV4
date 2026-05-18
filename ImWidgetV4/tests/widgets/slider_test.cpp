#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <cmath>
#include <memory>

using namespace ImWidgetV4;

class SliderTest : public ::testing::Test {
protected:
    void SetUp() override {
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
        Slider = std::make_shared<ImSlider>();
        Slider->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(240.0f, 40.0f)));
        Slider->SetRange(0.0f, 100.0f);
        App->SetRootWidget(Slider);
    }

    void TearDown() override {
        Slider.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    FInputEvent CreateMouseEvent(
        EInputEventType type,
        const FVector2& position,
        EMouseButton button = EMouseButton::Left) {
        FInputEvent event;
        event.Type = type;
        event.MousePosition = position;
        event.MouseButton = button;
        return event;
    }

    FInputEvent CreateKeyEvent(EInputEventType type, EKey key) {
        FInputEvent event;
        event.Type = type;
        event.Key = key;
        return event;
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(240.0f, 40.0f);
        App->AdvanceFrame(frameContext);
    }

    static void ExpectNear(float actual, float expected) {
        EXPECT_NEAR(actual, expected, 0.75f);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImSlider> Slider;
};

TEST_F(SliderTest, ConstructionAndRangeClamping) {
    EXPECT_NE(Slider, nullptr);
    EXPECT_TRUE(Slider->SupportsKeyboardFocus());
    EXPECT_FLOAT_EQ(Slider->GetMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(Slider->GetMaxValue(), 100.0f);
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);

    Slider->SetValue(120.0f);
    EXPECT_FLOAT_EQ(Slider->GetValue(), 100.0f);

    Slider->SetRange(50.0f, 10.0f);
    EXPECT_FLOAT_EQ(Slider->GetMinValue(), 10.0f);
    EXPECT_FLOAT_EQ(Slider->GetMaxValue(), 50.0f);
    EXPECT_FLOAT_EQ(Slider->GetValue(), 50.0f);
}

TEST_F(SliderTest, ClickSetsValueAndCapturesMouse) {
    float lastValue = -1.0f;
    ImSlider* sender = nullptr;
    Slider->OnValueChanged.AddLambda([&](ImSlider& slider, float value) {
        sender = &slider;
        lastValue = value;
    });

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(220.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), Slider);
    EXPECT_EQ(App->GetKeyboardFocus(), Slider);
    EXPECT_TRUE(Slider->IsDragging());
    ExpectNear(Slider->GetValue(), 100.0f);
    ExpectNear(lastValue, Slider->GetValue());
    EXPECT_EQ(sender, Slider.get());

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(220.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_FALSE(Slider->IsDragging());
}

TEST_F(SliderTest, DragUpdatesValueThroughApplicationRouting) {
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(16.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), Slider);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseMove, FVector2(224.0f, 20.0f))});
    EXPECT_GT(Slider->GetValue(), 95.0f);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(224.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_FALSE(Slider->IsDragging());
}

TEST_F(SliderTest, KeyboardAdjustmentsRequireFocus) {
    Slider->SetStep(10.0f);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Right)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);

    App->SetKeyboardFocus(Slider);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Right)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 10.0f);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::End)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 100.0f);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Home)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Left)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);
}

TEST_F(SliderTest, DisabledSliderDoesNotRespond) {
    Slider->SetDisabled(true);

    FReply mouseReply = Slider->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(120.0f, 20.0f)));
    EXPECT_FALSE(mouseReply.IsHandled());
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);
    EXPECT_FALSE(Slider->IsDragging());

    App->SetKeyboardFocus(Slider);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Right)});
    EXPECT_FLOAT_EQ(Slider->GetValue(), 0.0f);
}

TEST_F(SliderTest, ValueChangedCallbackCanReplaceOwningWidgetTreeSafely) {
    std::weak_ptr<ImSlider> oldSlider = Slider;
    int callbackCount = 0;

    Slider->OnValueChanged.AddLambda([&](ImSlider&, float) {
        ++callbackCount;

        auto replacement = std::make_shared<ImTextBlock>();
        replacement->SetText("Rebuilt");
        App->SetRootWidget(replacement);
        Slider.reset();
    });

    Slider->SetValue(42.0f);

    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(oldSlider.expired());
}

TEST_F(SliderTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FSliderStyle expectedStyle = ResolveSliderStyle(App->GetStyleSet());

    const FSliderStyle& style = Slider->GetStyle();
    EXPECT_EQ(style.TrackColor.ToImU32(), expectedStyle.TrackColor.ToImU32());
    EXPECT_EQ(style.ThumbColor.ToImU32(), expectedStyle.ThumbColor.ToImU32());
}

TEST_F(SliderTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FSliderStyle explicitStyle;
    explicitStyle.TrackColor = FColor::FromBytes(10, 20, 30);
    explicitStyle.ThumbColor = FColor::FromBytes(200, 210, 220);
    Slider->SetStyle(explicitStyle);

    EXPECT_EQ(Slider->GetStyle().TrackColor.ToImU32(), explicitStyle.TrackColor.ToImU32());
    EXPECT_EQ(Slider->GetStyle().ThumbColor.ToImU32(), explicitStyle.ThumbColor.ToImU32());
}
