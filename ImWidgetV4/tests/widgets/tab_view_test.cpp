#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#define private public
#include <imwidgetv4/widgets/TabView.h>
#undef private
#include <imwidgetv4/style/StyleResolvers.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

namespace {

FInputEvent CreateMouseEvent(
    EInputEventType type,
    const FVector2& position,
    EMouseButton button = EMouseButton::Left,
    double timestamp = 0.0)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = button;
    event.Timestamp = timestamp;
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

const ImTabView::FTabGeometry* FindTabGeometry(const std::shared_ptr<ImTabView>& view, int index)
{
    for (const ImTabView::FTabGeometry& geometry : view->VisibleTabGeometries_) {
        if (geometry.Index == index) {
            return &geometry;
        }
    }

    return nullptr;
}

FVector2 GetTabCenter(const std::shared_ptr<ImTabView>& view, int index)
{
    const ImTabView::FTabGeometry* geometry = FindTabGeometry(view, index);
    EXPECT_NE(geometry, nullptr);
    return geometry != nullptr ? geometry->Geometry.GetCenter() : FVector2();
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

    EXPECT_TRUE(View->SetTabClosable(1, true));
    EXPECT_TRUE(View->SetTabDirty(1, true));
    EXPECT_TRUE(View->SetTabTitle(1, "Renamed"));
    EXPECT_TRUE(View->SetTabIcon(1, FImageBrush()));
    EXPECT_TRUE(View->IsTabClosable(1));
    EXPECT_TRUE(View->IsTabDirty(1));
    ASSERT_NE(View->GetTab(1), nullptr);
    EXPECT_EQ(View->GetTab(1)->Title, "Renamed");
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

TEST_F(TabViewTest, DisabledTabsDoNotActivateAndCompressedTabsRemainClickable)
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
    const FVector2 secondTabPoint = GetTabCenter(View, 1);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint)
    }, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), activeBeforeDisabledClick);

    const FVector2 thirdTabPoint = GetTabCenter(View, 2);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, thirdTabPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, thirdTabPoint)
    }, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), 2);
}

TEST_F(TabViewTest, GeometryResizeTriggersRelayoutWithoutExplicitDirty)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->SetActiveTab(1);

    Advance({}, FVector2(260.0f, 180.0f));
    const FGeometry initialViewGeometry = View->GetGeometry();
    const FGeometry initialContentGeometry = second->GetGeometry();
    EXPECT_LT(initialContentGeometry.Position.Y, initialViewGeometry.GetMax().Y);
    EXPECT_LE(initialContentGeometry.GetMax().X, initialViewGeometry.GetMax().X + 0.01f);

    Advance({}, FVector2(420.0f, 240.0f));
    const FGeometry resizedViewGeometry = View->GetGeometry();
    const FGeometry resizedContentGeometry = second->GetGeometry();

    EXPECT_GT(resizedViewGeometry.Size.X, initialViewGeometry.Size.X);
    EXPECT_GT(resizedContentGeometry.Size.X, initialContentGeometry.Size.X);
    EXPECT_GT(resizedContentGeometry.Size.Y, initialContentGeometry.Size.Y);
    EXPECT_GE(resizedContentGeometry.Position.Y, resizedViewGeometry.Position.Y + 20.0f);
    EXPECT_LE(resizedContentGeometry.GetMax().X, resizedViewGeometry.GetMax().X + 0.01f);
    EXPECT_LE(resizedContentGeometry.GetMax().Y, resizedViewGeometry.GetMax().Y + 0.01f);
}

TEST_F(TabViewTest, WideningViewportRestoresTabsFromCompressedState)
{
    FTabViewStyle style = MakeCompactStyle();
    style.TabMinWidth = 70.0f;
    View->SetStyle(style);

    View->AddTab("Short", std::make_shared<TestContentWidget>());
    View->AddTab("A much longer title", std::make_shared<TestContentWidget>());
    View->AddTab("Mid", std::make_shared<TestContentWidget>());

    Advance({}, FVector2(150.0f, 120.0f));
    Advance({CreateMouseEvent(EInputEventType::MouseMove, GetTabCenter(View, 1))}, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetToolTipText(), "A much longer title");

    Advance({}, FVector2(360.0f, 120.0f));
    Advance({CreateMouseEvent(EInputEventType::MouseMove, GetTabCenter(View, 1))}, FVector2(360.0f, 120.0f));
    EXPECT_TRUE(View->GetToolTipText().empty());

    const FVector2 thirdTabPoint = GetTabCenter(View, 2);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, thirdTabPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, thirdTabPoint)
    }, FVector2(360.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), 2);
}

TEST_F(TabViewTest, BottomTabStripPlacementMovesContentAboveTabs)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->SetActiveTab(1);
    View->SetTabStripPlacement(ETabStripPlacement::Bottom);

    Advance({}, FVector2(220.0f, 140.0f));

    const FGeometry viewGeometry = View->GetGeometry();
    const FGeometry contentGeometry = second->GetGeometry();
    EXPECT_EQ(View->GetTabStripPlacement(), ETabStripPlacement::Bottom);
    EXPECT_EQ(contentGeometry.Position.X, viewGeometry.Position.X);
    EXPECT_EQ(contentGeometry.Position.Y, viewGeometry.Position.Y);
    EXPECT_LE(contentGeometry.GetMax().Y, viewGeometry.GetMax().Y - 20.0f);
}

TEST_F(TabViewTest, BottomTabStripPlacementSupportsTabClicks)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->SetTabStripPlacement(ETabStripPlacement::Bottom);

    Advance({}, FVector2(180.0f, 120.0f));

    const FVector2 secondTabPoint(90.0f, 106.0f);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint)});
    Advance({CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint)});

    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    EXPECT_EQ(App->GetKeyboardFocus(), View);
}

TEST_F(TabViewTest, CloseButtonRemovesTabAndBroadcastsClose)
{
    auto first = std::make_shared<TestContentWidget>();
    auto second = std::make_shared<TestContentWidget>();
    View->AddTab("First", first);
    View->AddTab("Second", second);
    View->SetTabClosable(0, true);
    Advance({});

    int closedIndex = -1;
    View->OnTabClosed.AddLambda([&](ImTabView&, int index) {
        closedIndex = index;
    });

    const FVector2 closeButtonPoint(58.0f, 14.0f);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, closeButtonPoint)});
    Advance({CreateMouseEvent(EInputEventType::MouseButtonUp, closeButtonPoint)});

    EXPECT_EQ(closedIndex, 0);
    EXPECT_EQ(View->GetTabCount(), 1);
    ASSERT_NE(View->GetTab(0), nullptr);
    EXPECT_EQ(View->GetTab(0)->Title, "Second");
}

TEST_F(TabViewTest, NarrowViewportKeepsAllTabsReachableByCompression)
{
    FTabViewStyle style = MakeCompactStyle();
    style.TabMinWidth = 70.0f;
    View->SetStyle(style);

    View->AddTab("First", std::make_shared<TestContentWidget>());
    View->AddTab("Second", std::make_shared<TestContentWidget>());
    View->AddTab("Third", std::make_shared<TestContentWidget>());
    Advance({}, FVector2(150.0f, 120.0f));

    const FVector2 thirdTabPoint = GetTabCenter(View, 2);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, thirdTabPoint)}, FVector2(150.0f, 120.0f));
    Advance({CreateMouseEvent(EInputEventType::MouseButtonUp, thirdTabPoint)}, FVector2(150.0f, 120.0f));
    EXPECT_EQ(View->GetActiveTabIndex(), 2);
}

TEST_F(TabViewTest, MiddleClickClosesClosableTabWithoutActivatingAnotherFirst)
{
    View->AddTab("First", std::make_shared<TestContentWidget>());
    View->AddTab("Second", std::make_shared<TestContentWidget>());
    View->SetTabClosable(1, true);
    Advance({});

    const FVector2 secondTabPoint(90.0f, 14.0f);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint, EMouseButton::Middle)});

    EXPECT_EQ(View->GetTabCount(), 1);
    ASSERT_NE(View->GetTab(0), nullptr);
    EXPECT_EQ(View->GetTab(0)->Title, "First");
    EXPECT_EQ(View->GetActiveTabIndex(), 0);
}

TEST_F(TabViewTest, MostRecentlyActiveClosePolicyOverridesLeftNeighborFallback)
{
    View->AddTab("Zero", std::make_shared<TestContentWidget>());
    View->AddTab("One", std::make_shared<TestContentWidget>());
    View->AddTab("Two", std::make_shared<TestContentWidget>());
    View->AddTab("Three", std::make_shared<TestContentWidget>());
    View->SetCloseActivationPolicy(ETabCloseActivationPolicy::MostRecentlyActive);

    EXPECT_TRUE(View->SetActiveTab(1));
    EXPECT_TRUE(View->SetActiveTab(3));
    EXPECT_TRUE(View->RemoveTab(3));

    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    ASSERT_NE(View->GetTab(1), nullptr);
    EXPECT_EQ(View->GetTab(1)->Title, "One");
}

TEST_F(TabViewTest, ClippedTabTitlePublishesTooltipText)
{
    FTabViewStyle style = MakeCompactStyle();
    style.TabMinWidth = 72.0f;
    View->SetStyle(style);

    View->AddTab("A very long clipped title", std::make_shared<TestContentWidget>());
    View->AddTab("Short", std::make_shared<TestContentWidget>());
    Advance({}, FVector2(160.0f, 120.0f));

    Advance({CreateMouseEvent(EInputEventType::MouseMove, GetTabCenter(View, 0))}, FVector2(160.0f, 120.0f));
    EXPECT_EQ(View->GetToolTipText(), "A very long clipped title");

    Advance({CreateMouseEvent(EInputEventType::MouseMove, FVector2(80.0f, 56.0f))}, FVector2(160.0f, 120.0f));
    EXPECT_TRUE(View->GetToolTipText().empty());
}

TEST_F(TabViewTest, DoubleClickBroadcastsDedicatedTabEvent)
{
    View->AddTab("One", std::make_shared<TestContentWidget>());
    View->AddTab("Two", std::make_shared<TestContentWidget>());
    Advance({});

    int doubleClickedIndex = -1;
    int invokedCount = 0;
    View->OnTabInvoked.AddLambda([&](ImTabView&, int) {
        ++invokedCount;
    });
    View->OnTabDoubleClicked.AddLambda([&](ImTabView&, int index) {
        doubleClickedIndex = index;
    });

    const FVector2 secondTabPoint = GetTabCenter(View, 1);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint, EMouseButton::Left, 1.00),
        CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint, EMouseButton::Left, 1.05),
        CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint, EMouseButton::Left, 1.20),
        CreateMouseEvent(EInputEventType::MouseButtonUp, secondTabPoint, EMouseButton::Left, 1.25)
    });

    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    EXPECT_EQ(invokedCount, 2);
    EXPECT_EQ(doubleClickedIndex, 1);
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

TEST_F(TabViewTest, RightClickActivatesTabAndBroadcastsContextMenuRequest)
{
    View->AddTab("One", std::make_shared<TestContentWidget>());
    View->AddTab("Two", std::make_shared<TestContentWidget>());
    Advance({});

    int requestedIndex = -1;
    FVector2 requestedPosition(-1.0f, -1.0f);
    View->OnTabContextMenuRequested.AddLambda([&](ImTabView&, int index, FVector2 position) {
        requestedIndex = index;
        requestedPosition = position;
    });

    const FVector2 secondTabPoint = GetTabCenter(View, 1);
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, secondTabPoint, EMouseButton::Right)});

    EXPECT_EQ(View->GetActiveTabIndex(), 1);
    EXPECT_EQ(requestedIndex, 1);
    EXPECT_EQ(requestedPosition.X, secondTabPoint.X);
    EXPECT_EQ(requestedPosition.Y, secondTabPoint.Y);
    EXPECT_EQ(App->GetKeyboardFocus(), View);
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

TEST_F(TabViewTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    auto themedView = std::make_shared<ImTabView>();
    App->SetRootWidget(themedView);
    const FTabViewStyle expectedStyle = ResolveTabViewStyle(App->GetStyleSet());

    const FTabViewStyle& style = themedView->GetStyle();
    EXPECT_EQ(style.ActiveTabColor.ToImU32(), expectedStyle.ActiveTabColor.ToImU32());
    EXPECT_EQ(style.TextColor.ToImU32(), expectedStyle.TextColor.ToImU32());
}

TEST_F(TabViewTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FTabViewStyle explicitStyle = MakeCompactStyle();
    explicitStyle.ActiveTabColor = FColor::FromBytes(10, 20, 30);
    explicitStyle.TextColor = FColor::FromBytes(200, 210, 220);
    View->SetStyle(explicitStyle);

    EXPECT_EQ(View->GetStyle().ActiveTabColor.ToImU32(), explicitStyle.ActiveTabColor.ToImU32());
    EXPECT_EQ(View->GetStyle().TextColor.ToImU32(), explicitStyle.TextColor.ToImU32());
}
