#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;

namespace {

class TestWidget : public ImWidget {
public:
    explicit TestWidget(const std::string& name, std::vector<std::string>* log = nullptr)
        : Log(log) {
        SetName(name);
    }

    void SetPreviewHandled(bool handled) { bPreviewHandled = handled; }
    void SetBubbleHandled(bool handled) { bBubbleHandled = handled; }
    void SetRequestCaptureOnMouseDown(bool enabled) { bRequestCaptureOnMouseDown = enabled; }
    void SetRequestReleaseOnMouseUp(bool enabled) { bRequestReleaseOnMouseUp = enabled; }
    void SetSupportsFocus(bool enabled) { SetSupportsKeyboardFocus(enabled); }

    int FocusChangeCount = 0;
    std::vector<std::string>* Log = nullptr;

    FReply OnPreviewInputEvent(const FInputEvent& event) override {
        if (Log) {
            Log->push_back("preview:" + GetName() + ":" + std::to_string(static_cast<int>(event.Type)));
        }

        if (bPreviewHandled) {
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    FReply OnInputEvent(const FInputEvent& event) override {
        if (Log) {
            Log->push_back("bubble:" + GetName() + ":" + std::to_string(static_cast<int>(event.Type)));
        }

        if (bRequestCaptureOnMouseDown &&
            event.Type == EInputEventType::MouseButtonDown) {
            return FReply::Handled().CaptureMouse(shared_from_this(), EMouseButton::Left);
        }

        if (bRequestReleaseOnMouseUp &&
            event.Type == EInputEventType::MouseButtonUp) {
            return FReply::Handled().ReleaseMouseCapture();
        }

        if (bBubbleHandled) {
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    void OnFocusChanged(bool bHasFocus) override {
        ImWidget::OnFocusChanged(bHasFocus);
        ++FocusChangeCount;
        LastFocusState = bHasFocus;
    }

    bool LastFocusState = false;

private:
    bool bPreviewHandled = false;
    bool bBubbleHandled = false;
    bool bRequestCaptureOnMouseDown = false;
    bool bRequestReleaseOnMouseUp = false;
};

FInputEvent MouseEvent(EInputEventType type, const FVector2& position) {
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = EMouseButton::Left;
    return event;
}

FInputEvent KeyEvent(EInputEventType type, EKey key) {
    FInputEvent event;
    event.Type = type;
    event.Key = key;
    return event;
}

} // namespace

class ApplicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        App = std::make_shared<ImApplication>();

        Root = std::make_shared<TestWidget>("root", &Log);
        Parent = std::make_shared<TestWidget>("parent", &Log);
        Leaf = std::make_shared<TestWidget>("leaf", &Log);

        Root->AddChild(Parent);
        Parent->AddChild(Leaf);

        Root->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(300.0f, 300.0f)));
        Parent->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(200.0f, 200.0f)));
        Leaf->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(100.0f, 100.0f)));

        App->SetRootWidget(Root);
    }

    void Advance(const std::vector<FInputEvent>& events) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(300.0f, 300.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<TestWidget> Root;
    std::shared_ptr<TestWidget> Parent;
    std::shared_ptr<TestWidget> Leaf;
    std::vector<std::string> Log;
};

TEST_F(ApplicationTest, BuildsCompleteFocusPathAndValidatesFocusTargets) {
    Leaf->SetSupportsFocus(true);
    auto outsider = std::make_shared<TestWidget>("outsider");
    outsider->SetSupportsFocus(true);

    App->SetKeyboardFocus(outsider);
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);

    App->SetKeyboardFocus(Leaf);
    ASSERT_EQ(App->GetKeyboardFocus(), Leaf);

    std::vector<std::shared_ptr<ImWidget>> focusPath = App->GetFocusPath();
    ASSERT_EQ(focusPath.size(), 3u);
    EXPECT_EQ(focusPath[0], Root);
    EXPECT_EQ(focusPath[1], Parent);
    EXPECT_EQ(focusPath[2], Leaf);

    EXPECT_EQ(Leaf->FocusChangeCount, 1);
    EXPECT_TRUE(Leaf->LastFocusState);

    App->ClearKeyboardFocus();
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(Leaf->FocusChangeCount, 2);
    EXPECT_FALSE(Leaf->LastFocusState);
}

TEST_F(ApplicationTest, PreviewCanInterceptBeforeBubble) {
    Parent->SetPreviewHandled(true);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))});

    ASSERT_EQ(Log.size(), 2u);
    EXPECT_EQ(Log[0], "preview:root:4");
    EXPECT_EQ(Log[1], "preview:parent:4");
}

TEST_F(ApplicationTest, BubbleOrderRunsFromLeafToRoot) {
    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))});

    ASSERT_EQ(Log.size(), 6u);
    EXPECT_EQ(Log[0], "preview:root:4");
    EXPECT_EQ(Log[1], "preview:parent:4");
    EXPECT_EQ(Log[2], "preview:leaf:4");
    EXPECT_EQ(Log[3], "bubble:leaf:4");
    EXPECT_EQ(Log[4], "bubble:parent:4");
    EXPECT_EQ(Log[5], "bubble:root:4");
}

TEST_F(ApplicationTest, CaptureRoutesSubsequentEventsUntilReleased) {
    Leaf->SetRequestCaptureOnMouseDown(true);
    Leaf->SetRequestReleaseOnMouseUp(true);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))});
    EXPECT_EQ(App->GetMouseCapture(), Leaf);

    Log.clear();
    Advance({MouseEvent(EInputEventType::MouseMove, FVector2(250.0f, 250.0f))});
    EXPECT_NE(
        std::find(Log.begin(), Log.end(), "bubble:leaf:1"),
        Log.end()
    );

    Log.clear();
    Advance({MouseEvent(EInputEventType::MouseButtonUp, FVector2(250.0f, 250.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    ASSERT_FALSE(Log.empty());
    EXPECT_EQ(Log.back(), "bubble:leaf:5");
}

TEST_F(ApplicationTest, HoverEnterLeaveOnlyFireOnTargetChanges) {
    Advance({
        MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(250.0f, 250.0f))
    });

    int enterCount = 0;
    int leaveCount = 0;
    for (const std::string& entry : Log) {
        if (entry == "bubble:leaf:2") {
            ++enterCount;
        }
        if (entry == "bubble:leaf:3") {
            ++leaveCount;
        }
    }

    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(leaveCount, 1);
}

TEST_F(ApplicationTest, LastFrameEventsReflectCurrentFrameOnly) {
    Advance({KeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    ASSERT_EQ(App->GetLastFrameEvents().size(), 1u);
    EXPECT_EQ(App->GetLastFrameEvents()[0].Type, EInputEventType::KeyDown);

    Advance({});
    EXPECT_TRUE(App->GetLastFrameEvents().empty());
}
