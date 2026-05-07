#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class SwitchTest : public ::testing::Test {
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
        Switch = std::make_shared<ImSwitch>();
        Switch->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(52.0f, 28.0f)));
        App->SetRootWidget(Switch);
    }

    void TearDown() override {
        Switch.reset();
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
        frameContext.FrameInfo.ViewportSize = FVector2(52.0f, 28.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImSwitch> Switch;
};

TEST_F(SwitchTest, ConstructionAndStateApi) {
    EXPECT_NE(Switch, nullptr);
    EXPECT_TRUE(Switch->SupportsKeyboardFocus());
    EXPECT_FALSE(Switch->IsChecked());
    EXPECT_TRUE(Switch->IsEnabled());
    EXPECT_FALSE(Switch->IsDisabled());
    EXPECT_FALSE(Switch->IsHovered());
    EXPECT_FALSE(Switch->IsPressed());

    int changeCount = 0;
    bool lastState = false;
    ImSwitch* sender = nullptr;
    Switch->OnCheckStateChanged.AddLambda([&](ImSwitch& widget, bool checked) {
        ++changeCount;
        lastState = checked;
        sender = &widget;
    });

    Switch->SetChecked(true);
    EXPECT_TRUE(Switch->IsChecked());
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastState);
    EXPECT_EQ(sender, Switch.get());

    Switch->SetChecked(true);
    EXPECT_EQ(changeCount, 1);

    Switch->Toggle();
    EXPECT_FALSE(Switch->IsChecked());
    EXPECT_EQ(changeCount, 2);
}

TEST_F(SwitchTest, ClickTogglesStateAndUsesCapture) {
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(26.0f, 14.0f))});
    EXPECT_EQ(App->GetMouseCapture(), Switch);
    EXPECT_EQ(App->GetKeyboardFocus(), Switch);
    EXPECT_TRUE(Switch->IsPressed());

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(26.0f, 14.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_FALSE(Switch->IsPressed());
    EXPECT_TRUE(Switch->IsChecked());
}

TEST_F(SwitchTest, ReleaseOutsideDoesNotToggle) {
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(26.0f, 14.0f))});
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(120.0f, 60.0f))});

    EXPECT_FALSE(Switch->IsChecked());
    EXPECT_FALSE(Switch->IsPressed());
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(SwitchTest, KeyboardToggleRequiresFocus) {
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_FALSE(Switch->IsChecked());

    App->SetKeyboardFocus(Switch);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_TRUE(Switch->IsChecked());

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_FALSE(Switch->IsChecked());
}

TEST_F(SwitchTest, DisabledSwitchDoesNotRespond) {
    Switch->SetDisabled(true);

    FReply mouseReply = Switch->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 14.0f)));
    EXPECT_FALSE(mouseReply.IsHandled());
    EXPECT_FALSE(Switch->IsChecked());
    EXPECT_FALSE(Switch->IsPressed());

    App->SetKeyboardFocus(Switch);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_FALSE(Switch->IsChecked());
}

TEST_F(SwitchTest, CheckStateCallbackCanReplaceOwningWidgetTreeSafely) {
    std::weak_ptr<ImSwitch> oldSwitch = Switch;
    int callbackCount = 0;

    Switch->OnCheckStateChanged.AddLambda([&](ImSwitch&, bool) {
        ++callbackCount;

        auto replacement = std::make_shared<ImTextBlock>();
        replacement->SetText("Rebuilt");
        App->SetRootWidget(replacement);
        Switch.reset();
    });

    Switch->SetChecked(true);

    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(oldSwitch.expired());
}
