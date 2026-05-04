#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

class EditableTextTest : public ::testing::Test {
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
        EditableText = std::make_shared<ImEditableText>();
        EditableText->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(240.0f, 40.0f)));
        App->SetRootWidget(EditableText);
    }

    void TearDown() override {
        EditableText.reset();
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

    FInputEvent CreateModifiedKeyEvent(
        EInputEventType type,
        EKey key,
        bool bCtrl = false,
        bool bShift = false) {
        FInputEvent event;
        event.Type = type;
        event.Key = key;
        event.Modifiers = FInputModifiers(bCtrl, bShift, false, false);
        return event;
    }

    FInputEvent CreateTextEvent(unsigned int codepoint) {
        FInputEvent event;
        event.Type = EInputEventType::TextInput;
        event.Codepoint = codepoint;
        return event;
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(240.0f, 40.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImEditableText> EditableText;
};

TEST_F(EditableTextTest, ConstructionAndProperties) {
    EXPECT_NE(EditableText, nullptr);
    EXPECT_TRUE(EditableText->SupportsKeyboardFocus());
    EXPECT_EQ(EditableText->GetText(), "");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 0u);

    EditableText->SetHintText("Search");
    EXPECT_EQ(EditableText->GetHintText(), "Search");
}

TEST_F(EditableTextTest, TextInputChangesTextAndMovesCursor) {
    std::vector<std::string> changes;
    EditableText->OnTextChanged.AddLambda([&](ImEditableText&, const std::string& text) {
        changes.push_back(text);
    });

    App->SetKeyboardFocus(EditableText);
    AdvanceWithEvents({CreateTextEvent('A'), CreateTextEvent('B')});

    EXPECT_EQ(EditableText->GetText(), "AB");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 2u);
    ASSERT_EQ(changes.size(), 2u);
    EXPECT_EQ(changes[0], "A");
    EXPECT_EQ(changes[1], "AB");
}

TEST_F(EditableTextTest, NavigationAndDeletionWorkThroughApplicationRouting) {
    EditableText->SetText("ABCD");
    App->SetKeyboardFocus(EditableText);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::End)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 4u);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Left)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 3u);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Backspace)});
    EXPECT_EQ(EditableText->GetText(), "ABD");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 2u);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Home)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 0u);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::DeleteKey)});
    EXPECT_EQ(EditableText->GetText(), "BD");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 0u);
}

TEST_F(EditableTextTest, MouseClickPlacesCaretNearRequestedPosition) {
    EditableText->SetText("ABCD");

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(14.0f, 20.0f))});
    EXPECT_EQ(App->GetKeyboardFocus(), EditableText);
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 0u);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(120.0f, 20.0f))});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 4u);
}

TEST_F(EditableTextTest, ShiftSelectionCanBeReplacedAndDeleted) {
    EditableText->SetText("Slate");
    App->SetKeyboardFocus(EditableText);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::End)});

    AdvanceWithEvents({
        CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Left, false, true),
        CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Left, false, true)
    });
    EXPECT_TRUE(EditableText->HasSelection());
    EXPECT_EQ(EditableText->GetSelectionStartByteIndex(), 3u);
    EXPECT_EQ(EditableText->GetSelectionEndByteIndex(), 5u);

    AdvanceWithEvents({CreateTextEvent('X')});
    EXPECT_EQ(EditableText->GetText(), "SlaX");
    EXPECT_FALSE(EditableText->HasSelection());
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 4u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::A, true, false)});
    EXPECT_TRUE(EditableText->HasSelection());
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::DeleteKey)});
    EXPECT_EQ(EditableText->GetText(), "");
    EXPECT_FALSE(EditableText->HasSelection());
}

TEST_F(EditableTextTest, ClipboardShortcutsCopyCutAndPasteSelection) {
    EditableText->SetText("Slate");
    App->SetKeyboardFocus(EditableText);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::A, true, false)});
    EXPECT_TRUE(EditableText->HasSelection());

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::C, true, false)});
    EXPECT_EQ(EditableText->GetText(), "Slate");

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::X, true, false)});
    EXPECT_EQ(EditableText->GetText(), "");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 0u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::V, true, false)});
    EXPECT_EQ(EditableText->GetText(), "Slate");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 5u);
}

TEST_F(EditableTextTest, CtrlNavigationAndDeletionOperateOnWords) {
    EditableText->SetText("alpha beta_gamma 42 delta");
    App->SetKeyboardFocus(EditableText);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::End)});

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Left, true, false)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 20u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Left, true, false)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 17u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Backspace, true, false)});
    EXPECT_EQ(EditableText->GetText(), "alpha 42 delta");
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 6u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::Right, true, false)});
    EXPECT_EQ(EditableText->GetCursorByteIndex(), 9u);

    AdvanceWithEvents({CreateModifiedKeyEvent(EInputEventType::KeyDown, EKey::DeleteKey, true, false)});
    EXPECT_EQ(EditableText->GetText(), "alpha 42 ");
}

TEST_F(EditableTextTest, MouseDragSelectionUsesCaptureAndReleasesOnMouseUp) {
    EditableText->SetText("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), EditableText);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseMove, FVector2(220.0f, 20.0f))});
    EXPECT_TRUE(EditableText->HasSelection());
    EXPECT_GT(EditableText->GetSelectionEndByteIndex(), EditableText->GetSelectionStartByteIndex());

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(220.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(EditableTextTest, EnterAndFocusLossCommitText) {
    std::vector<std::string> commits;
    EditableText->OnTextCommitted.AddLambda([&](ImEditableText&, const std::string& text) {
        commits.push_back(text);
    });

    App->SetKeyboardFocus(EditableText);
    AdvanceWithEvents({CreateTextEvent('H'), CreateTextEvent('i')});
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});

    ASSERT_EQ(commits.size(), 1u);
    EXPECT_EQ(commits[0], "Hi");

    EditableText->SetText("Done");
    App->SetKeyboardFocus(EditableText);
    App->ClearKeyboardFocus();

    ASSERT_EQ(commits.size(), 2u);
    EXPECT_EQ(commits[1], "Done");
}

TEST_F(EditableTextTest, DisabledEditableTextDoesNotAcceptInput) {
    EditableText->SetDisabled(true);
    App->SetKeyboardFocus(EditableText);

    AdvanceWithEvents({CreateTextEvent('X'), CreateKeyEvent(EInputEventType::KeyDown, EKey::Backspace)});
    EXPECT_EQ(EditableText->GetText(), "");

    FReply reply = EditableText->OnInputEvent(
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(10.0f, 10.0f)));
    EXPECT_FALSE(reply.IsHandled());
}
