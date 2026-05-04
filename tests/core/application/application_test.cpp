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
    void SetRequestFocusOnMouseDown(bool enabled) { bRequestFocusOnMouseDown = enabled; }
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

        if (event.Type == EInputEventType::MouseButtonDown) {
            FReply reply = FReply::Unhandled();
            if (bRequestFocusOnMouseDown) {
                reply = FReply::Handled().SetKeyboardFocus(shared_from_this());
            }
            if (bRequestCaptureOnMouseDown) {
                if (!reply.IsHandled()) {
                    reply = FReply::Handled();
                }
                reply.CaptureMouse(shared_from_this(), EMouseButton::Left);
            }
            if (reply.IsHandled()) {
                return reply;
            }
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
    bool bRequestFocusOnMouseDown = false;
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

TEST_F(ApplicationTest, SetRootWidgetCreatesMainWindowCompatibilityShell) {
    auto rootWidget = std::make_shared<TestWidget>("compat-root");

    App->SetRootWidget(rootWidget);

    ASSERT_NE(App->GetWindowManager().GetMainWindow(), nullptr);
    EXPECT_EQ(App->GetRootWidget(), rootWidget);
    EXPECT_EQ(App->GetWindowManager().GetMainWindow()->GetRootWidget(), rootWidget);
    EXPECT_FALSE(App->GetWindowManager().GetMainWindow()->HasTitleBar());
    EXPECT_FALSE(App->GetWindowManager().GetMainWindow()->IsMovable());
}

TEST_F(ApplicationTest, WindowManagerPreservesBringToFrontOrdering) {
    FWindowOptions firstOptions;
    firstOptions.Title = "First";
    firstOptions.Position = FVector2(10.0f, 10.0f);
    firstOptions.Size = FVector2(100.0f, 100.0f);
    firstOptions.RootWidget = std::make_shared<TestWidget>("first-root");

    FWindowOptions secondOptions = firstOptions;
    secondOptions.Title = "Second";
    secondOptions.Position = FVector2(140.0f, 10.0f);
    secondOptions.RootWidget = std::make_shared<TestWidget>("second-root");

    const auto first = App->GetWindowManager().CreateWindow(firstOptions);
    const auto second = App->GetWindowManager().CreateWindow(secondOptions);

    auto openWindows = App->GetWindowManager().GetOpenWindows();
    ASSERT_EQ(openWindows.size(), 3u);
    EXPECT_EQ(openWindows.back(), second);

    App->GetWindowManager().BringToFront(first);
    openWindows = App->GetWindowManager().GetOpenWindows();
    ASSERT_EQ(openWindows.size(), 3u);
    EXPECT_EQ(openWindows.back(), first);
}

TEST_F(ApplicationTest, ClickingWindowContentActivatesWindowAndRoutesToItsTree) {
    std::vector<std::string> leftLog;
    std::vector<std::string> rightLog;

    FWindowOptions leftOptions;
    leftOptions.Title = "Left";
    leftOptions.Position = FVector2(10.0f, 10.0f);
    leftOptions.Size = FVector2(120.0f, 90.0f);
    leftOptions.RootWidget = std::make_shared<TestWidget>("left-root", &leftLog);

    FWindowOptions rightOptions;
    rightOptions.Title = "Right";
    rightOptions.Position = FVector2(160.0f, 10.0f);
    rightOptions.Size = FVector2(120.0f, 90.0f);
    rightOptions.RootWidget = std::make_shared<TestWidget>("right-root", &rightLog);

    const auto leftWindow = App->GetWindowManager().CreateWindow(leftOptions);
    const auto rightWindow = App->GetWindowManager().CreateWindow(rightOptions);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(180.0f, 56.0f))});

    EXPECT_EQ(App->GetWindowManager().GetActiveWindow(), rightWindow);
    EXPECT_TRUE(leftLog.empty());
    EXPECT_NE(
        std::find(rightLog.begin(), rightLog.end(), "bubble:right-root:4"),
        rightLog.end());
}

TEST_F(ApplicationTest, TitleBarDragMovesWindowUntilMouseRelease) {
    FWindowOptions options;
    options.Title = "Draggable";
    options.Position = FVector2(20.0f, 20.0f);
    options.Size = FVector2(140.0f, 100.0f);
    options.RootWidget = std::make_shared<TestWidget>("drag-root");
    const auto window = App->GetWindowManager().CreateWindow(options);

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(32.0f, 32.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(210.0f, 160.0f))
    });
    EXPECT_EQ(window->GetPosition(), FVector2(198.0f, 148.0f));

    Advance({MouseEvent(EInputEventType::MouseButtonUp, FVector2(210.0f, 160.0f))});
    EXPECT_EQ(window->GetPosition(), FVector2(198.0f, 148.0f));
}

TEST_F(ApplicationTest, ClosingActiveWindowClearsFocusAndCapture) {
    auto widget = std::make_shared<TestWidget>("focus-capture");
    widget->SetSupportsFocus(true);
    widget->SetRequestFocusOnMouseDown(true);
    widget->SetRequestCaptureOnMouseDown(true);

    FWindowOptions options;
    options.Title = "Interactive";
    options.Position = FVector2(10.0f, 10.0f);
    options.Size = FVector2(180.0f, 120.0f);
    options.RootWidget = widget;
    const auto window = App->GetWindowManager().CreateWindow(options);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(40.0f, 60.0f))});
    EXPECT_EQ(App->GetKeyboardFocus(), widget);
    EXPECT_EQ(App->GetMouseCapture(), widget);

    App->GetWindowManager().CloseWindow(window);
    Advance({});
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(ApplicationTest, ClickingOutsidePopupClosesPopupChain) {
    FWindowOptions normalOptions;
    normalOptions.Title = "Normal";
    normalOptions.Position = FVector2(10.0f, 10.0f);
    normalOptions.Size = FVector2(140.0f, 100.0f);
    normalOptions.RootWidget = std::make_shared<TestWidget>("normal-root");
    App->GetWindowManager().CreateWindow(normalOptions);

    FPopupOptions parentOptions;
    parentOptions.Position = FVector2(60.0f, 60.0f);
    parentOptions.Size = FVector2(120.0f, 80.0f);
    parentOptions.RootWidget = std::make_shared<TestWidget>("popup-root");
    const auto parentPopup = App->GetWindowManager().CreatePopup(parentOptions);

    FPopupOptions childOptions;
    childOptions.Position = FVector2(90.0f, 90.0f);
    childOptions.Size = FVector2(100.0f, 70.0f);
    childOptions.RootWidget = std::make_shared<TestWidget>("child-popup-root");
    childOptions.ParentWindow = parentPopup;
    const auto childPopup = App->GetWindowManager().CreatePopup(childOptions);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(260.0f, 260.0f))});
    EXPECT_FALSE(parentPopup->IsOpen());
    EXPECT_FALSE(childPopup->IsOpen());
}

TEST_F(ApplicationTest, ClosingParentPopupRecursivelyClosesChildren) {
    FPopupOptions parentOptions;
    parentOptions.Position = FVector2(30.0f, 30.0f);
    parentOptions.Size = FVector2(120.0f, 80.0f);
    parentOptions.RootWidget = std::make_shared<TestWidget>("popup-root");
    const auto parentPopup = App->GetWindowManager().CreatePopup(parentOptions);

    FPopupOptions childOptions;
    childOptions.Position = FVector2(60.0f, 60.0f);
    childOptions.Size = FVector2(100.0f, 70.0f);
    childOptions.RootWidget = std::make_shared<TestWidget>("child-popup-root");
    childOptions.ParentWindow = parentPopup;
    const auto childPopup = App->GetWindowManager().CreatePopup(childOptions);

    App->GetWindowManager().CloseWindow(parentPopup);

    EXPECT_FALSE(parentPopup->IsOpen());
    EXPECT_FALSE(childPopup->IsOpen());
}

TEST_F(ApplicationTest, ModalBlocksLowerWindowsUntilClosed) {
    std::vector<std::string> lowerLog;
    std::vector<std::string> modalLog;

    FWindowOptions lowerOptions;
    lowerOptions.Title = "Lower";
    lowerOptions.Position = FVector2(10.0f, 10.0f);
    lowerOptions.Size = FVector2(160.0f, 120.0f);
    lowerOptions.RootWidget = std::make_shared<TestWidget>("lower-root", &lowerLog);
    const auto lowerWindow = App->GetWindowManager().CreateWindow(lowerOptions);

    FPopupOptions modalOptions;
    modalOptions.Title = "Modal";
    modalOptions.Position = FVector2(90.0f, 60.0f);
    modalOptions.Size = FVector2(140.0f, 110.0f);
    modalOptions.RootWidget = std::make_shared<TestWidget>("modal-root", &modalLog);
    const auto modalWindow = App->GetWindowManager().CreateModal(modalOptions);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(30.0f, 70.0f))});
    EXPECT_TRUE(lowerLog.empty());
    EXPECT_EQ(App->GetWindowManager().GetActiveWindow(), modalWindow);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(120.0f, 120.0f))});
    EXPECT_NE(
        std::find(modalLog.begin(), modalLog.end(), "bubble:modal-root:4"),
        modalLog.end());

    App->GetWindowManager().CloseWindow(modalWindow);
    lowerLog.clear();
    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(30.0f, 70.0f))});
    EXPECT_NE(
        std::find(lowerLog.begin(), lowerLog.end(), "bubble:lower-root:4"),
        lowerLog.end());
    EXPECT_EQ(App->GetWindowManager().GetActiveWindow(), lowerWindow);
}
