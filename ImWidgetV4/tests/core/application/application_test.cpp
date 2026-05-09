#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imgui.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;

namespace {

class MockApplicationBackend : public ImApplicationBackend {
public:
    ~MockApplicationBackend() override = default;

    bool Initialize() override { return true; }
    void Shutdown() override {}
    void Run() override {}
    bool ShouldClose() const override { return bShouldClose; }
    void SetWindowTitle(const std::string& title) override
    {
        WindowTitle = title;
        ++SetWindowTitleCallCount;
    }
    void SetWindowSize(int width, int height) override
    {
        WindowWidth = width;
        WindowHeight = height;
    }
    void GetWindowSize(int& width, int& height) const override
    {
        width = WindowWidth;
        height = WindowHeight;
    }
    void BeginFrame() override {}
    void EndFrame() override {}
    void SetApplication(ImApplication* app) override
    {
        Application = app;
        if (Application != nullptr) {
            Application->SetBackend(this);
        }
    }
    ImApplication* GetApplication() const override { return Application; }
    void RequestClose() override { bShouldClose = true; }
    std::string GetBackendName() const override { return "Mock"; }
    ImTextureID CreateTextureFromRGBA(const std::uint8_t*, int, int) override
    {
        ++CreateTextureCallCount;
        return reinterpret_cast<ImTextureID>(NextTextureIdValue++);
    }
    void ReleaseTexture(ImTextureID textureId) override
    {
        ReleasedTextures.push_back(textureId);
    }
    bool SetWindowIconFromRGBA(const std::uint8_t* rgbaPixels, int width, int height) override
    {
        IconWidth = width;
        IconHeight = height;
        IconPixels.assign(
            rgbaPixels,
            rgbaPixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U));
        ++SetWindowIconCallCount;
        return true;
    }
    void ClearWindowIcon() override
    {
        IconWidth = 0;
        IconHeight = 0;
        IconPixels.clear();
        ++ClearWindowIconCallCount;
    }
    FPathDialogResult OpenFileDialog(const FOpenFileDialogOptions& options) override
    {
        LastOpenFileDialogOptions = options;
        ++OpenFileDialogCallCount;
        return NextOpenFileDialogResult;
    }
    FPathDialogResult OpenFolderDialog(const FOpenFolderDialogOptions& options) override
    {
        LastOpenFolderDialogOptions = options;
        ++OpenFolderDialogCallCount;
        return NextOpenFolderDialogResult;
    }
    FPathDialogResult SaveFileDialog(const FSaveFileDialogOptions& options) override
    {
        LastSaveFileDialogOptions = options;
        ++SaveFileDialogCallCount;
        return NextSaveFileDialogResult;
    }
    bool IsUsingCustomHostChrome() const override { return bUseCustomHostChrome; }
    bool SupportsHostWindowDrag() const override { return bSupportsHostWindowDrag; }
    bool SupportsHostWindowMinimize() const override { return bSupportsHostWindowMinimize; }
    bool SupportsHostWindowMaximize() const override { return bSupportsHostWindowMaximize; }
    bool SupportsHostWindowClose() const override { return bSupportsHostWindowClose; }
    bool IsHostWindowMaximized() const override { return bHostWindowMaximized; }
    bool BeginHostWindowDrag() override
    {
        ++BeginHostWindowDragCallCount;
        return bSupportsHostWindowDrag;
    }
    bool MinimizeHostWindow() override
    {
        ++MinimizeHostWindowCallCount;
        return bSupportsHostWindowMinimize;
    }
    bool ToggleHostWindowMaximize() override
    {
        ++ToggleHostWindowMaximizeCallCount;
        if (bSupportsHostWindowMaximize) {
            bHostWindowMaximized = !bHostWindowMaximized;
        }
        return bSupportsHostWindowMaximize;
    }
    bool CloseHostWindow() override
    {
        ++CloseHostWindowCallCount;
        return bSupportsHostWindowClose;
    }

    ImApplication* Application = nullptr;
    std::string WindowTitle;
    int WindowWidth = 0;
    int WindowHeight = 0;
    int SetWindowTitleCallCount = 0;
    int SetWindowIconCallCount = 0;
    int ClearWindowIconCallCount = 0;
    int OpenFileDialogCallCount = 0;
    int OpenFolderDialogCallCount = 0;
    int SaveFileDialogCallCount = 0;
    int CreateTextureCallCount = 0;
    int BeginHostWindowDragCallCount = 0;
    int MinimizeHostWindowCallCount = 0;
    int ToggleHostWindowMaximizeCallCount = 0;
    int CloseHostWindowCallCount = 0;
    int IconWidth = 0;
    int IconHeight = 0;
    std::vector<std::uint8_t> IconPixels;
    std::vector<ImTextureID> ReleasedTextures;
    FOpenFileDialogOptions LastOpenFileDialogOptions;
    FOpenFolderDialogOptions LastOpenFolderDialogOptions;
    FSaveFileDialogOptions LastSaveFileDialogOptions;
    FPathDialogResult NextOpenFileDialogResult;
    FPathDialogResult NextOpenFolderDialogResult;
    FPathDialogResult NextSaveFileDialogResult;
    bool bUseCustomHostChrome = false;
    bool bSupportsHostWindowDrag = false;
    bool bSupportsHostWindowMinimize = false;
    bool bSupportsHostWindowMaximize = false;
    bool bSupportsHostWindowClose = false;
    bool bHostWindowMaximized = false;
    bool bShouldClose = false;
    std::uintptr_t NextTextureIdValue = 1;
};

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

    void Advance(const std::vector<FInputEvent>& events, double currentTime = 0.0) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(300.0f, 300.0f);
        frameContext.FrameInfo.CurrentTime = currentTime;
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

TEST_F(ApplicationTest, ToolTipAppearsAfterHoverDelayAndClosesOnMouseDown)
{
    FToolTipStyle toolTipStyle = App->GetToolTipStyle();
    toolTipStyle.ShowDelaySeconds = 0.1;
    App->SetToolTipStyle(toolTipStyle);
    Leaf->SetToolTipText("Leaf tooltip");

    Advance({MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 20.0f))}, 0.0);
    EXPECT_EQ(App->GetWindowManager().GetOpenWindows().size(), 1u);

    Advance({}, 0.05);
    EXPECT_EQ(App->GetWindowManager().GetOpenWindows().size(), 1u);

    Advance({}, 0.11);
    auto openWindows = App->GetWindowManager().GetOpenWindows();
    ASSERT_EQ(openWindows.size(), 2u);
    EXPECT_EQ(openWindows.back()->GetKind(), EWindowKind::Tooltip);
    EXPECT_FALSE(openWindows.back()->IsHitTestVisible());

    Log.clear();
    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))}, 0.12);
    EXPECT_EQ(App->GetWindowManager().GetOpenWindows().size(), 1u);
    EXPECT_NE(
        std::find(Log.begin(), Log.end(), "bubble:leaf:4"),
        Log.end());
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

TEST_F(ApplicationTest, ClickingMainWindowDoesNotBlockLaterFloatingWindowInteraction) {
    std::vector<std::string> floatingLog;

    auto mainRoot = std::make_shared<TestWidget>("main-root");
    App->SetRootWidget(mainRoot);

    FWindowOptions floatingOptions;
    floatingOptions.Title = "Floating";
    floatingOptions.Position = FVector2(120.0f, 80.0f);
    floatingOptions.Size = FVector2(160.0f, 110.0f);
    floatingOptions.RootWidget = std::make_shared<TestWidget>("floating-root", &floatingLog);
    const auto floatingWindow = App->GetWindowManager().CreateWindow(floatingOptions);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))});
    EXPECT_EQ(App->GetWindowManager().GetActiveWindow(), App->GetWindowManager().GetMainWindow());

    floatingLog.clear();
    Advance({MouseEvent(EInputEventType::MouseButtonDown, FVector2(150.0f, 120.0f))});

    EXPECT_EQ(App->GetWindowManager().GetActiveWindow(), floatingWindow);
    EXPECT_NE(
        std::find(floatingLog.begin(), floatingLog.end(), "bubble:floating-root:4"),
        floatingLog.end());
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

TEST(ApplicationFontTest, ApplicationConfiguresDefaultFontForEmptyAtlas) {
    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    ASSERT_NE(context, nullptr);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.FontDefault = nullptr;
    ASSERT_EQ(io.Fonts->Fonts.Size, 0);

    {
        ImApplication application;
        EXPECT_GT(io.Fonts->Fonts.Size, 0);
        EXPECT_NE(io.FontDefault, nullptr);
    }

    ImGui::DestroyContext(context);
}

TEST(ApplicationIniSettingsTest, ApplicationSynchronizesIniFilenameWithCurrentContext) {
    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    ASSERT_NE(context, nullptr);

    ImApplication application;
    const std::filesystem::path iniPath =
        std::filesystem::temp_directory_path() / "imwidgetv4" / "application_test_imgui.ini";

    application.SetIniSettingsPath(iniPath);

    EXPECT_EQ(application.GetIniSettingsPath(), iniPath);
    ASSERT_NE(ImGui::GetIO().IniFilename, nullptr);
    EXPECT_EQ(std::string(ImGui::GetIO().IniFilename), iniPath.string());

    application.SetIniSettingsPath({});
    EXPECT_TRUE(application.GetIniSettingsPath().empty());
    EXPECT_EQ(ImGui::GetIO().IniFilename, nullptr);

    ImGui::DestroyContext(context);
}

TEST(ApplicationHostChromeTest, ApplicationTitleSynchronizesWhenBackendIsBound)
{
    ImApplication application;
    MockApplicationBackend backend;

    application.SetApplicationTitle("Before bind");
    application.SetBackend(&backend);
    EXPECT_EQ(backend.WindowTitle, "Before bind");

    application.SetApplicationTitle("After bind");
    EXPECT_EQ(backend.WindowTitle, "After bind");
    EXPECT_GE(backend.SetWindowTitleCallCount, 2);
}

TEST(ApplicationHostChromeTest, ApplicationIconSynchronizesAndCanBeCleared)
{
    ImApplication application;
    MockApplicationBackend backend;
    application.SetBackend(&backend);

    const FImageBrush iconBrush = application.GetCoreIconBrush(ECoreIcon::Save);
    ASSERT_TRUE(iconBrush.IsValid());

    application.SetApplicationIcon(iconBrush);
    EXPECT_NE(application.GetApplicationIcon().TextureId, nullptr);
    EXPECT_EQ(backend.IconWidth, 32);
    EXPECT_EQ(backend.IconHeight, 32);
    ASSERT_FALSE(backend.IconPixels.empty());
    EXPECT_EQ(backend.IconPixels[3], 0);
    EXPECT_GT(backend.SetWindowIconCallCount, 0);

    application.SetApplicationIcon(FImageBrush());
    EXPECT_TRUE(application.GetApplicationIcon().TextureId == nullptr);
    EXPECT_TRUE(backend.IconPixels.empty());
    EXPECT_GT(backend.ClearWindowIconCallCount, 0);
}

TEST(ApplicationFileDialogTest, ReturnsUnsupportedWhenBackendIsMissing)
{
    ImApplication application;

    const FPathDialogResult openFileResult = application.OpenFileDialog(FOpenFileDialogOptions());
    const FPathDialogResult openFolderResult = application.OpenFolderDialog(FOpenFolderDialogOptions());
    const FPathDialogResult saveFileResult = application.SaveFileDialog(FSaveFileDialogOptions());

    EXPECT_EQ(openFileResult.Code, EPathDialogResultCode::Unsupported);
    EXPECT_EQ(openFolderResult.Code, EPathDialogResultCode::Unsupported);
    EXPECT_EQ(saveFileResult.Code, EPathDialogResultCode::Unsupported);
}

TEST(ApplicationFileDialogTest, ForwardsOptionsAndReturnsBackendResults)
{
    ImApplication application;
    MockApplicationBackend backend;
    application.SetBackend(&backend);

    backend.NextOpenFileDialogResult.Code = EPathDialogResultCode::Accepted;
    backend.NextOpenFileDialogResult.Path = std::filesystem::path("C:/temp/input.txt");

    backend.NextOpenFolderDialogResult.Code = EPathDialogResultCode::Cancelled;

    backend.NextSaveFileDialogResult.Code = EPathDialogResultCode::Error;
    backend.NextSaveFileDialogResult.ErrorMessage = "save failed";

    FOpenFileDialogOptions openFileOptions;
    openFileOptions.Title = "Open Asset";
    openFileOptions.InitialDirectory = std::filesystem::path("C:/project/assets");
    openFileOptions.Filters = {
        FFileDialogFilter {"Images", {"*.png", "*.jpg"}},
        FFileDialogFilter {"All Files", {"*.*"}}
    };
    openFileOptions.DefaultFilterIndex = 1;

    FOpenFolderDialogOptions openFolderOptions;
    openFolderOptions.Title = "Select Output Folder";
    openFolderOptions.InitialDirectory = std::filesystem::path("C:/project/output");

    FSaveFileDialogOptions saveFileOptions;
    saveFileOptions.Title = "Save Snapshot";
    saveFileOptions.InitialDirectory = std::filesystem::path("C:/project/snapshots");
    saveFileOptions.DefaultFileName = "capture";
    saveFileOptions.DefaultExtension = "png";
    saveFileOptions.Filters = {
        FFileDialogFilter {"PNG", {"*.png"}}
    };
    saveFileOptions.DefaultFilterIndex = 0;
    saveFileOptions.bPromptOverwrite = false;

    const FPathDialogResult openFileResult = application.OpenFileDialog(openFileOptions);
    const FPathDialogResult openFolderResult = application.OpenFolderDialog(openFolderOptions);
    const FPathDialogResult saveFileResult = application.SaveFileDialog(saveFileOptions);

    EXPECT_EQ(backend.OpenFileDialogCallCount, 1);
    EXPECT_EQ(backend.OpenFolderDialogCallCount, 1);
    EXPECT_EQ(backend.SaveFileDialogCallCount, 1);

    EXPECT_EQ(backend.LastOpenFileDialogOptions.Title, openFileOptions.Title);
    EXPECT_EQ(backend.LastOpenFileDialogOptions.InitialDirectory, openFileOptions.InitialDirectory);
    ASSERT_EQ(backend.LastOpenFileDialogOptions.Filters.size(), 2u);
    EXPECT_EQ(backend.LastOpenFileDialogOptions.Filters[0].Label, "Images");
    ASSERT_EQ(backend.LastOpenFileDialogOptions.Filters[0].Patterns.size(), 2u);
    EXPECT_EQ(backend.LastOpenFileDialogOptions.Filters[0].Patterns[0], "*.png");
    EXPECT_EQ(backend.LastOpenFileDialogOptions.DefaultFilterIndex, 1);

    EXPECT_EQ(backend.LastOpenFolderDialogOptions.Title, openFolderOptions.Title);
    EXPECT_EQ(backend.LastOpenFolderDialogOptions.InitialDirectory, openFolderOptions.InitialDirectory);

    EXPECT_EQ(backend.LastSaveFileDialogOptions.Title, saveFileOptions.Title);
    EXPECT_EQ(backend.LastSaveFileDialogOptions.InitialDirectory, saveFileOptions.InitialDirectory);
    EXPECT_EQ(backend.LastSaveFileDialogOptions.DefaultFileName, "capture");
    EXPECT_EQ(backend.LastSaveFileDialogOptions.DefaultExtension, "png");
    EXPECT_FALSE(backend.LastSaveFileDialogOptions.bPromptOverwrite);
    ASSERT_EQ(backend.LastSaveFileDialogOptions.Filters.size(), 1u);
    EXPECT_EQ(backend.LastSaveFileDialogOptions.Filters[0].Label, "PNG");

    EXPECT_EQ(openFileResult.Code, EPathDialogResultCode::Accepted);
    EXPECT_EQ(openFileResult.Path, std::filesystem::path("C:/temp/input.txt"));
    EXPECT_EQ(openFolderResult.Code, EPathDialogResultCode::Cancelled);
    EXPECT_EQ(saveFileResult.Code, EPathDialogResultCode::Error);
    EXPECT_EQ(saveFileResult.ErrorMessage, "save failed");
}

TEST(ApplicationRuntimeTextureTest, RecreatesBackendTexturesAfterBackendResourcesAreLost)
{
    ImApplication application;
    MockApplicationBackend backend;
    backend.SetApplication(&application);

    const ImTextureID runtimeTextureId = application.CreateRuntimeTextureFromRgba(
        std::vector<std::uint8_t>(4U * 4U * 4U, 255U),
        4,
        4);
    ASSERT_NE(runtimeTextureId, nullptr);

    const ImTextureID firstResolvedTextureId = application.ResolveTextureForPaint(runtimeTextureId);
    ASSERT_NE(firstResolvedTextureId, nullptr);
    EXPECT_NE(firstResolvedTextureId, runtimeTextureId);
    EXPECT_EQ(backend.CreateTextureCallCount, 1);

    application.NotifyBackendTextureResourcesLost();

    const ImTextureID secondResolvedTextureId = application.ResolveTextureForPaint(runtimeTextureId);
    ASSERT_NE(secondResolvedTextureId, nullptr);
    EXPECT_NE(secondResolvedTextureId, runtimeTextureId);
    EXPECT_NE(secondResolvedTextureId, firstResolvedTextureId);
    EXPECT_EQ(backend.CreateTextureCallCount, 2);
}
