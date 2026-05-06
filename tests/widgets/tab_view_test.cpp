#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

namespace {

FInputEvent CreateMouseEvent(EInputEventType type, const FVector2& position, EMouseButton button = EMouseButton::Left)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = button;
    return event;
}

FInputEvent CreateKeyEvent(EKey key, bool bCtrl = false, bool bShift = false)
{
    FInputEvent event;
    event.Type = EInputEventType::KeyDown;
    event.Key = key;
    event.Modifiers = FInputModifiers(bCtrl, bShift, false, false);
    return event;
}

FTabViewStyle MakeCompactStyle()
{
    FTabViewStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.TabHeight = 28.0f;
    style.TabMinWidth = 64.0f;
    style.TabSpacing = 2.0f;
    style.TabPadding = FMargin(8.0f, 8.0f, 4.0f, 4.0f);
    style.MinDesiredSize = FVector2(180.0f, 120.0f);
    return style;
}

class TestContentWidget : public ImWidget {
public:
    TestContentWidget()
    {
        SetHitTestVisible(true);
        SetSupportsKeyboardFocus(true);
    }

    FVector2 GetMinSize() const override
    {
        return MinSize;
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown && event.MouseButton == EMouseButton::Left) {
            ++MouseDownCount;
            FReply reply = FReply::Handled();
            if (bRequestFocusOnMouseDown) {
                reply.SetKeyboardFocus(shared_from_this());
            }
            if (bRequestCaptureOnMouseDown) {
                reply.CaptureMouse(shared_from_this(), EMouseButton::Left);
            }
            return reply;
        }

        return FReply::Unhandled();
    }

    FVector2 MinSize {80.0f, 48.0f};
    int MouseDownCount = 0;
    bool bRequestFocusOnMouseDown = false;
    bool bRequestCaptureOnMouseDown = false;
};

class TabViewTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
            io.DisplaySize = ImVec2(640.0f, 480.0f);
            io.DeltaTime = 1.0f / 60.0f;
        }

        ImGui::NewFrame();

        App = std::make_shared<ImApplication>();
        View = std::make_shared<ImTabView>();
        View->SetStyle(MakeCompactStyle());
        App->SetRootWidget(View);
    }

    void TearDown() override
    {
        View.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    void Advance(const std::vector<FInputEvent>& events, const FVector2& viewportSize = FVector2(180.0f, 120.0f))
    {
        DrawContext drawContext(ImGui::GetBackgroundDrawList());
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = viewportSize;
        frameContext.DrawContext_ = &drawContext;
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImTabView> View;
};

} // namespace

TEST_F(TabViewTest, DefaultStateAndAddTabActivationAreStable)
{
    EXPECT_EQ(View->GetTabCount(), 0);
    EXPECT_EQ(View->GetActiveTabIndex(), -1);
    EXPECT_EQ(View->GetActiveContent(), nullptr);

    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();

    EXPECT_EQ(View->AddTab("First", first), 0);
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
    EXPECT_EQ(View->GetActiveContent(), first);

    EXPECT_EQ(View->AddTab("Second", second), 1);
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
    EXPECT_TRUE(View->SetActiveTab(1));
    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    EXPECT_EQ(View->GetActiveContent(), second);

    EXPECT_FALSE(View->SetActiveTab(8));
    EXPECT_EQ(View->GetActiveTabIndex(), 1);
}

TEST_F(TabViewTest, RemoveAndClearTabsUpdateActiveSelection)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    auto third = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->AddTab("Third", third);

    View->SetActiveTab(2);
    EXPECT_TRUE(View->RemoveTab(2));
    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    ASSERT_NE(View->GetTab(1), nullptr);
    EXPECT_EQ(View->GetTab(1)->Title, "Second");

    EXPECT_TRUE(View->RemoveTab(0));
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
    ASSERT_NE(View->GetTab(0), nullptr);
    EXPECT_EQ(View->GetTab(0)->Title, "Second");

    View->ClearTabs();
    EXPECT_EQ(View->GetTabCount(), 0);
    EXPECT_EQ(View->GetActiveTabIndex(), -1);
    EXPECT_EQ(View->GetActiveContent(), nullptr);
}

TEST_F(TabViewTest, MinSizeIncludesActiveContentAndOnlyActiveContentReceivesInput)
{
    auto first = std::make_shared<TestContentWidget>();
    first->MinSize = FVector2(150.0f, 70.0f);
    auto second = std::make_shared<TestContentWidget>();
    second->MinSize = FVector2(110.0f, 50.0f);

    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->SetActiveTab(1);
    Advance({});

    const FVector2 minSize = View->GetMinSize();
    EXPECT_GE(minSize.X, 180.0f);
    EXPECT_GE(minSize.Y, 120.0f);

    const FVector2 contentClick = second->GetGeometry().GetCenter();
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, contentClick)});
    EXPECT_EQ(second->MouseDownCount, 1);
    EXPECT_EQ(first->MouseDownCount, 0);
}

TEST_F(TabViewTest, ClickingTabSwitchesSelectionAndRequestsFocus)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    Advance({});

    int invokedIndex = -1;
    int changedIndex = -1;
    View->OnTabInvoked.AddLambda([&](ImTabView&, int index) { invokedIndex = index; });
    View->OnActiveTabChanged.AddLambda([&](ImTabView&, int index) { changedIndex = index; });

    const FVector2 secondTabPoint(90.0f, 14.0f);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint)});
    EXPECT_EQ(App->GetMouseCapture(), View);
    EXPECT_EQ(App->GetKeyboardFocus(), View);

    Advance({CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint)});
    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    EXPECT_EQ(invokedIndex, 1);
    EXPECT_EQ(changedIndex, 1);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(TabViewTest, DisabledAndOverflowTabsDoNotActivate)
{
    FTabViewStyle style = MakeCompactStyle();
    style.TabMinWidth = 70.0f;
    View->SetStyle(style);

    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    auto third = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->AddTab("Third", third);
    View->SetTabEnabled(1, false);
    Advance({}, FVector2(150.0f, 120.0f));

    const int activeBeforeDisabledClick = View->GetActiveTabIndex();
    const FVector2 secondTabPoint(105.0f, 14.0f);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint)
    }, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), activeBeforeDisabledClick);

    const FVector2 overflowPoint(145.0f, 14.0f);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, overflowPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, overflowPoint)
    }, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), activeBeforeDisabledClick);
}

TEST_F(TabViewTest, KeyboardNavigationSupportsCtrlTabAndHomeEnd)
{
    View->AddTab("One", std::make_shared<TestContentWidget>());
    View->AddTab("Two", std::make_shared<TestContentWidget>());
    View->AddTab("Three", std::make_shared<TestContentWidget>());
    Advance({});

    App->SetKeyboardFocus(View);
    Advance({CreateKeyEvent(EKey::Tab, true, false)});
    EXPECT_EQ(View->GetActiveTabIndex(), 1);

    Advance({CreateKeyEvent(EKey::Tab, true, true)});
    EXPECT_EQ(View->GetActiveTabIndex(), 0);

    Advance({CreateKeyEvent(EKey::End)});
    EXPECT_EQ(View->GetActiveTabIndex(), 2);

    Advance({CreateKeyEvent(EKey::Home)});
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
}

TEST_F(TabViewTest, RemovingActiveTabClearsFocusAndCaptureFromItsContent)
{
    auto first = std::make_shared<TestContentWidget>();
    first->bRequestFocusOnMouseDown = true;
    first->bRequestCaptureOnMouseDown = true;
    auto second = std::make_shared<TestContentWidget>();

    View->AddTab("First", first);
    View->AddTab("Second", second);
    Advance({});

    const FVector2 contentClick = first->GetGeometry().GetCenter();
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, contentClick)});
    EXPECT_EQ(App->GetKeyboardFocus(), first);
    EXPECT_EQ(App->GetMouseCapture(), first);

    EXPECT_TRUE(View->RemoveTab(0));
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
    EXPECT_EQ(View->GetActiveContent(), second);
}
