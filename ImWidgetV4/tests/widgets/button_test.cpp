#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class ButtonTest : public ::testing::Test {
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
        Button = std::make_shared<ImButton>();
        Button->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(100.0f, 30.0f)));
        App->SetRootWidget(Button);
    }

    void TearDown() override {
        Button.reset();
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
        event.Timestamp = 0.0;
        return event;
    }

    FInputEvent CreateKeyEvent(EInputEventType type, EKey key) {
        FInputEvent event;
        event.Type = type;
        event.Key = key;
        event.Timestamp = 0.0;
        return event;
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(100.0f, 30.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImButton> Button;
};

TEST_F(ButtonTest, Construction) {
    EXPECT_NE(Button, nullptr);
    EXPECT_TRUE(Button->IsVisible());
    EXPECT_TRUE(Button->IsHitTestVisible());
    EXPECT_TRUE(Button->SupportsKeyboardFocus());
    EXPECT_FALSE(Button->IsDisabled());
    EXPECT_FALSE(Button->IsHovered());
    EXPECT_FALSE(Button->IsPressed());
}

TEST_F(ButtonTest, TextProperty) {
    EXPECT_EQ(Button->GetText(), "");

    Button->SetText("Click Me");
    EXPECT_EQ(Button->GetText(), "Click Me");
}

TEST_F(ButtonTest, DisabledState) {
    EXPECT_FALSE(Button->IsDisabled());
    Button->SetDisabled(true);
    EXPECT_TRUE(Button->IsDisabled());
    Button->SetDisabled(false);
    EXPECT_FALSE(Button->IsDisabled());
}

TEST_F(ButtonTest, ClickEvent) {
    bool clicked = false;
    ImButton* sender = nullptr;
    Button->OnClicked.AddLambda([&](ImButton& button) {
        clicked = true;
        sender = &button;
    });

    FReply downReply = Button->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50.0f, 15.0f)));
    EXPECT_TRUE(downReply.IsHandled());
    EXPECT_TRUE(Button->IsPressed());
    EXPECT_FALSE(clicked);

    FReply upReply = Button->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(50.0f, 15.0f)));
    EXPECT_TRUE(upReply.IsHandled());
    EXPECT_FALSE(Button->IsPressed());
    EXPECT_TRUE(clicked);
    EXPECT_EQ(sender, Button.get());
}

TEST_F(ButtonTest, ClickOutsideDoesNotTrigger) {
    bool clicked = false;
    Button->OnClicked.AddLambda([&](ImButton&) { clicked = true; });

    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50.0f, 15.0f)));
    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(200.0f, 200.0f)));

    EXPECT_FALSE(Button->IsPressed());
    EXPECT_FALSE(clicked);
}

TEST_F(ButtonTest, DisabledButtonDoesNotRespond) {
    bool clicked = false;
    Button->OnClicked.AddLambda([&](ImButton&) { clicked = true; });
    Button->SetDisabled(true);

    FReply reply = Button->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50.0f, 15.0f)));
    EXPECT_FALSE(reply.IsHandled());
    EXPECT_FALSE(Button->IsPressed());
    EXPECT_FALSE(clicked);
}

TEST_F(ButtonTest, HoverUsesExplicitEnterLeaveEvents) {
    bool hoverBegin = false;
    bool hoverEnd = false;
    ImButton* hoverBeginSender = nullptr;
    ImButton* hoverEndSender = nullptr;

    Button->OnHoverBegin.AddLambda([&](ImButton& button) {
        hoverBegin = true;
        hoverBeginSender = &button;
    });
    Button->OnHoverEnd.AddLambda([&](ImButton& button) {
        hoverEnd = true;
        hoverEndSender = &button;
    });

    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseEnter, FVector2(50.0f, 15.0f)));
    EXPECT_TRUE(Button->IsHovered());
    EXPECT_TRUE(hoverBegin);
    EXPECT_FALSE(hoverEnd);
    EXPECT_EQ(hoverBeginSender, Button.get());

    hoverBegin = false;
    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseLeave, FVector2(200.0f, 200.0f)));
    EXPECT_FALSE(Button->IsHovered());
    EXPECT_FALSE(hoverBegin);
    EXPECT_TRUE(hoverEnd);
    EXPECT_EQ(hoverEndSender, Button.get());
}

TEST_F(ButtonTest, KeyboardActivation) {
    bool clicked = false;
    Button->OnClicked.AddLambda([&](ImButton&) { clicked = true; });

    App->SetKeyboardFocus(Button);
    EXPECT_TRUE(Button->HasKeyboardFocus());

    FReply enterReply = Button->OnInputEvent(CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter));
    EXPECT_TRUE(enterReply.IsHandled());
    EXPECT_TRUE(clicked);

    clicked = false;
    FReply spaceReply = Button->OnInputEvent(CreateKeyEvent(EInputEventType::KeyDown, EKey::Space));
    EXPECT_TRUE(spaceReply.IsHandled());
    EXPECT_TRUE(clicked);
}

TEST_F(ButtonTest, KeyboardWithoutFocus) {
    bool clicked = false;
    Button->OnClicked.AddLambda([&](ImButton&) { clicked = true; });

    FReply reply = Button->OnInputEvent(CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter));
    EXPECT_FALSE(reply.IsHandled());
    EXPECT_FALSE(clicked);
}

TEST_F(ButtonTest, PressedAndReleasedCallbacks) {
    bool pressed = false;
    bool released = false;
    ImButton* pressedSender = nullptr;
    ImButton* releasedSender = nullptr;

    Button->OnPressed.AddLambda([&](ImButton& button) {
        pressed = true;
        pressedSender = &button;
    });
    Button->OnReleased.AddLambda([&](ImButton& button) {
        released = true;
        releasedSender = &button;
    });

    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50.0f, 15.0f)));
    EXPECT_TRUE(pressed);
    EXPECT_FALSE(released);
    EXPECT_EQ(pressedSender, Button.get());

    pressed = false;
    Button->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(50.0f, 15.0f)));
    EXPECT_FALSE(pressed);
    EXPECT_TRUE(released);
    EXPECT_EQ(releasedSender, Button.get());
}

TEST_F(ButtonTest, MinSizeWithText) {
    Button->SetText("Click Me");
    const FVector2 minSize = Button->GetMinSize();
    EXPECT_GT(minSize.X, 0.0f);
    EXPECT_GT(minSize.Y, 0.0f);

    Button->SetText("This is a much longer button text");
    const FVector2 longerMinSize = Button->GetMinSize();
    EXPECT_GT(longerMinSize.X, minSize.X);
}

TEST_F(ButtonTest, MinSizeTracksTextBlockContentInParentLayout) {
    auto root = std::make_shared<ImVerticalBox>();
    auto button = std::make_shared<ImButton>();
    auto textBlock = std::make_shared<ImTextBlock>();
    textBlock->SetText("A much longer button label that should expand the button");
    textBlock->SetFontSize(28.0f);
    button->SetContent(textBlock);
    root->AddChild(button);
    App->SetRootWidget(root);

    const FVector2 contentMinSize = textBlock->GetMinSize();
    const FVector2 buttonMinSize = button->GetMinSize();
    EXPECT_GE(buttonMinSize.X, contentMinSize.X + 20.0f);
    EXPECT_GE(buttonMinSize.Y, contentMinSize.Y + 10.0f);

    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();
    DrawContext drawContext(&drawList);

    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportSize = FVector2(900.0f, 200.0f);
    frameContext.DrawContext_ = &drawContext;
    App->AdvanceFrame(frameContext);

    EXPECT_GE(button->GetGeometry().Size.X, buttonMinSize.X);
    EXPECT_GE(button->GetGeometry().Size.Y, buttonMinSize.Y);
    EXPECT_GE(textBlock->GetGeometry().Size.X, contentMinSize.X);
    EXPECT_GE(textBlock->GetGeometry().Size.Y, contentMinSize.Y);
}

TEST_F(ButtonTest, TextBlockMinSizeIsStableAcrossAssignedWidths) {
    auto textBlock = std::make_shared<ImTextBlock>();
    textBlock->SetText("A wrapped text block should not feed its current layout width back into min size computation.");
    textBlock->SetWrapText(true);
    textBlock->SetFontSize(18.0f);

    textBlock->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(120.0f, 80.0f)));
    const FVector2 narrowMinSize = textBlock->GetMinSize();

    textBlock->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(360.0f, 80.0f)));
    const FVector2 wideMinSize = textBlock->GetMinSize();

    EXPECT_NEAR(narrowMinSize.X, wideMinSize.X, 0.01f);
    EXPECT_NEAR(narrowMinSize.Y, wideMinSize.Y, 0.01f);
    EXPECT_GT(narrowMinSize.X, 0.0f);
    EXPECT_GT(narrowMinSize.Y, 0.0f);
}

TEST_F(ButtonTest, TextBlockEnablesToolTipWhenTextIsClipped) {
    auto textBlock = std::make_shared<ImTextBlock>();
    textBlock->SetText("This text is intentionally too long for the assigned geometry.");
    textBlock->SetFontSize(18.0f);
    textBlock->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(90.0f, 20.0f)));

    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();
    DrawContext drawContext(&drawList);
    FPaintContext paintContext(
        drawContext,
        textBlock->GetGeometry(),
        nullptr,
        FVector2(0.0f, 0.0f),
        false,
        1.0f / 60.0f);
    textBlock->Paint(paintContext);

    EXPECT_TRUE(textBlock->HasToolTip());
    EXPECT_EQ(textBlock->GetToolTipText(), textBlock->GetText());

    textBlock->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(1200.0f, 40.0f)));
    textBlock->Paint(paintContext);
    EXPECT_FALSE(textBlock->HasToolTip());
}

TEST_F(ButtonTest, ApplicationRoutesHoverCaptureAndClick) {
    bool hoverBegin = false;
    bool hoverEnd = false;
    bool clicked = false;

    Button->OnHoverBegin.AddLambda([&](ImButton&) { hoverBegin = true; });
    Button->OnHoverEnd.AddLambda([&](ImButton&) { hoverEnd = true; });
    Button->OnClicked.AddLambda([&](ImButton&) { clicked = true; });

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseMove, FVector2(50.0f, 15.0f))});
    EXPECT_TRUE(Button->IsHovered());
    EXPECT_TRUE(hoverBegin);
    EXPECT_FALSE(hoverEnd);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50.0f, 15.0f))});
    EXPECT_EQ(App->GetMouseCapture(), Button);
    EXPECT_EQ(App->GetKeyboardFocus(), Button);
    EXPECT_TRUE(Button->IsPressed());

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(200.0f, 200.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_FALSE(Button->IsPressed());
    EXPECT_FALSE(clicked);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseMove, FVector2(200.0f, 200.0f))});
    EXPECT_FALSE(Button->IsHovered());
    EXPECT_TRUE(hoverEnd);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
