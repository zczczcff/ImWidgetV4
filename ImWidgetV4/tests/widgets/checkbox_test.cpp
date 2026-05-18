#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class CheckBoxTest : public ::testing::Test {
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
        CheckBox = std::make_shared<ImCheckBox>();
        CheckBox->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(180.0f, 32.0f)));
        App->SetRootWidget(CheckBox);
    }

    void TearDown() override {
        CheckBox.reset();
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
        frameContext.FrameInfo.ViewportSize = FVector2(180.0f, 32.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImCheckBox> CheckBox;
};

TEST_F(CheckBoxTest, Construction) {
    EXPECT_NE(CheckBox, nullptr);
    EXPECT_TRUE(CheckBox->SupportsKeyboardFocus());
    EXPECT_FALSE(CheckBox->IsChecked());
    EXPECT_FALSE(CheckBox->IsDisabled());
    EXPECT_FALSE(CheckBox->IsHovered());
    EXPECT_FALSE(CheckBox->IsPressed());
}

TEST_F(CheckBoxTest, LabelAndCheckedProperties) {
    CheckBox->SetLabel("Enable Feature");
    EXPECT_EQ(CheckBox->GetLabel(), "Enable Feature");

    int changeCount = 0;
    bool lastState = false;
    ImCheckBox* sender = nullptr;
    CheckBox->OnCheckStateChanged.AddLambda([&](ImCheckBox& checkBox, bool checked) {
        ++changeCount;
        lastState = checked;
        sender = &checkBox;
    });

    CheckBox->SetChecked(true);
    EXPECT_TRUE(CheckBox->IsChecked());
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastState);
    EXPECT_EQ(sender, CheckBox.get());
}

TEST_F(CheckBoxTest, ClickTogglesStateAndUsesCapture) {
    bool checked = false;
    CheckBox->OnCheckStateChanged.AddLambda([&](ImCheckBox&, bool isChecked) {
        checked = isChecked;
    });

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 16.0f))});
    EXPECT_EQ(App->GetMouseCapture(), CheckBox);
    EXPECT_TRUE(CheckBox->IsPressed());
    EXPECT_EQ(App->GetKeyboardFocus(), CheckBox);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(20.0f, 16.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_FALSE(CheckBox->IsPressed());
    EXPECT_TRUE(CheckBox->IsChecked());
    EXPECT_TRUE(checked);
}

TEST_F(CheckBoxTest, ReleaseOutsideDoesNotToggle) {
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 16.0f))});
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(220.0f, 50.0f))});

    EXPECT_FALSE(CheckBox->IsChecked());
    EXPECT_FALSE(CheckBox->IsPressed());
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(CheckBoxTest, KeyboardToggleRequiresFocus) {
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_FALSE(CheckBox->IsChecked());

    App->SetKeyboardFocus(CheckBox);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_TRUE(CheckBox->IsChecked());

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_FALSE(CheckBox->IsChecked());
}

TEST_F(CheckBoxTest, DisabledCheckBoxDoesNotRespond) {
    CheckBox->SetDisabled(true);

    FReply reply = CheckBox->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(10.0f, 10.0f)));
    EXPECT_FALSE(reply.IsHandled());
    EXPECT_FALSE(CheckBox->IsChecked());
    EXPECT_FALSE(CheckBox->IsPressed());
}

TEST_F(CheckBoxTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FCheckBoxStyle expectedStyle = ResolveCheckBoxStyle(App->GetStyleSet());

    const FCheckBoxStyle& style = CheckBox->GetStyle();
    EXPECT_EQ(style.CheckedBackgroundColor.ToImU32(), expectedStyle.CheckedBackgroundColor.ToImU32());
    EXPECT_EQ(style.TextColor.ToImU32(), expectedStyle.TextColor.ToImU32());
}

TEST_F(CheckBoxTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FCheckBoxStyle explicitStyle;
    explicitStyle.CheckedBackgroundColor = FColor::FromBytes(20, 30, 40);
    explicitStyle.TextColor = FColor::FromBytes(200, 210, 220);
    CheckBox->SetStyle(explicitStyle);

    EXPECT_EQ(CheckBox->GetStyle().CheckedBackgroundColor.ToImU32(), explicitStyle.CheckedBackgroundColor.ToImU32());
    EXPECT_EQ(CheckBox->GetStyle().TextColor.ToImU32(), explicitStyle.TextColor.ToImU32());
}
