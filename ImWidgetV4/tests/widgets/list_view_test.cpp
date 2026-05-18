#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace ImWidgetV4;

namespace {

FListViewStyle MakeCompactListStyle()
{
    FListViewStyle style;
    style.Padding = FMargin(0.0f);
    style.RowPadding = FMargin(4.0f, 4.0f, 2.0f, 2.0f);
    style.BorderThickness = 0.0f;
    style.RowMinHeight = 22.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    style.ThumbMinLength = 20.0f;
    style.WheelScrollStep = 16.0f;
    style.MinDesiredSize = FVector2(140.0f, 90.0f);
    return style;
}

FInputEvent CreateMouseEvent(EInputEventType type, const FVector2& position, EMouseButton button = EMouseButton::Left)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = button;
    return event;
}

FInputEvent CreateWheelEvent(const FVector2& position, float deltaY)
{
    FInputEvent event;
    event.Type = EInputEventType::MouseWheel;
    event.MousePosition = position;
    event.ScrollDelta = FVector2(0.0f, deltaY);
    return event;
}

FInputEvent CreateKeyEvent(EKey key)
{
    FInputEvent event;
    event.Type = EInputEventType::KeyDown;
    event.Key = key;
    return event;
}

class TestRowWidget : public ImWidget {
public:
    explicit TestRowWidget(float minHeight = 18.0f)
        : MinHeight(minHeight)
    {
        SetHitTestVisible(true);
        SetSupportsKeyboardFocus(true);
    }

    FVector2 GetMinSize() const override
    {
        return FVector2(80.0f, MinHeight);
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown) {
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

    float MinHeight = 18.0f;
    int MouseDownCount = 0;
    bool bRequestFocusOnMouseDown = false;
    bool bRequestCaptureOnMouseDown = false;
};

class ListViewTest : public ::testing::Test {
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
        View = std::make_shared<ImListView>();
        View->SetStyle(MakeCompactListStyle());
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

    void Advance(const std::vector<FInputEvent>& events, const FVector2& viewportSize = FVector2(140.0f, 90.0f))
    {
        DrawContext drawContext(ImGui::GetBackgroundDrawList());
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = viewportSize;
        frameContext.DrawContext_ = &drawContext;
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImListView> View;
};

} // namespace

TEST_F(ListViewTest, DefaultStateAndSelectionClampingAreStable)
{
    EXPECT_EQ(View->GetItemCount(), 0u);
    EXPECT_FALSE(View->HasSelection());
    EXPECT_EQ(View->GetSelectedIndex(), -1);
    EXPECT_NE(View->GetEmptyContent(), nullptr);

    View->SetItemCount(3);
    View->SetSelectedIndex(1);
    EXPECT_EQ(View->GetSelectedIndex(), 1);

    View->SetSelectedIndex(99);
    EXPECT_EQ(View->GetSelectedIndex(), -1);

    View->SetSelectedIndex(-1);
    EXPECT_EQ(View->GetSelectedIndex(), -1);

    EXPECT_FALSE(View->ScrollToIndex(-1));
    EXPECT_FALSE(View->ScrollToIndex(8));
}

TEST_F(ListViewTest, GeneratesVisibleRowsOnlyAndRefreshRegeneratesThem)
{
    std::unordered_map<std::size_t, int> generateCounts;
    View->SetItemCount(1000);
    View->SetOnGenerateRow([&](std::size_t index) {
        ++generateCounts[index];
        return std::make_shared<TestRowWidget>(18.0f + static_cast<float>(index % 3));
    });

    Advance({});
    EXPECT_GT(generateCounts.size(), 0u);
    EXPECT_LT(generateCounts.size(), 40u);

    const std::size_t beforeScrollGenerated = generateCounts.size();
    Advance({CreateWheelEvent(FVector2(40.0f, 40.0f), -6.0f)});
    EXPECT_GE(generateCounts.size(), beforeScrollGenerated);

    const std::size_t beforeRefreshTotal = generateCounts.size();
    View->RequestRefresh();
    Advance({});
    EXPECT_GE(generateCounts.size(), beforeRefreshTotal);
    EXPECT_LT(generateCounts.size(), 80u);
}

TEST_F(ListViewTest, AdaptiveRowHeightUpdatesScrollRange)
{
    View->SetItemCount(6);
    View->SetOnGenerateRow([&](std::size_t index) {
        return std::make_shared<TestRowWidget>(index == 0 ? 56.0f : 12.0f);
    });

    Advance({});
    EXPECT_GT(View->GetMaxScrollOffset(), 0.0f);
}

TEST_F(ListViewTest, LeftClickSelectsBeforeRoutingToRowWidget)
{
    std::vector<std::shared_ptr<TestRowWidget>> rows;
    View->SetItemCount(3);
    View->SetOnGenerateRow([&](std::size_t) {
        auto row = std::make_shared<TestRowWidget>(22.0f);
        rows.push_back(row);
        return row;
    });

    Advance({});
    ASSERT_FALSE(rows.empty());

    const FVector2 clickPoint = rows[1]->GetGeometry().GetCenter();
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, clickPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, clickPoint)
    });

    EXPECT_EQ(View->GetSelectedIndex(), 1);
    EXPECT_EQ(rows[1]->MouseDownCount, 1);
}

TEST_F(ListViewTest, RightClickSelectsAndBroadcastsContextRequest)
{
    std::vector<std::shared_ptr<TestRowWidget>> rows;
    View->SetItemCount(3);
    View->SetOnGenerateRow([&](std::size_t) {
        auto row = std::make_shared<TestRowWidget>(22.0f);
        rows.push_back(row);
        return row;
    });

    int requestedIndex = -1;
    FVector2 requestedPosition;
    View->OnItemContextMenuRequested.AddLambda([&](ImListView&, int index, FVector2 position) {
        requestedIndex = index;
        requestedPosition = position;
    });

    Advance({});
    const FVector2 clickPoint = rows[2]->GetGeometry().GetCenter();
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, clickPoint, EMouseButton::Right)});

    EXPECT_EQ(View->GetSelectedIndex(), 2);
    EXPECT_EQ(requestedIndex, 2);
    EXPECT_EQ(requestedPosition, clickPoint);
}

TEST_F(ListViewTest, KeyboardNavigationWheelAndScrollbarDragWork)
{
    View->SetItemCount(12);
    View->SetOnGenerateRow([&](std::size_t index) {
        return std::make_shared<TestRowWidget>(index % 2 == 0 ? 18.0f : 28.0f);
    });

    Advance({});
    App->SetKeyboardFocus(View);

    Advance({CreateKeyEvent(EKey::Down)});
    EXPECT_EQ(View->GetSelectedIndex(), 0);

    Advance({CreateKeyEvent(EKey::End)});
    EXPECT_EQ(View->GetSelectedIndex(), 11);
    EXPECT_GT(View->GetScrollOffset(), 0.0f);

    const float beforeWheel = View->GetScrollOffset();
    Advance({CreateWheelEvent(FVector2(40.0f, 40.0f), -2.0f)});
    EXPECT_GE(View->GetScrollOffset(), beforeWheel);

    View->SetScrollOffset(0.0f);
    Advance({});
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(135.0f, 15.0f))});
    EXPECT_EQ(App->GetMouseCapture(), View);

    const float beforeDrag = View->GetScrollOffset();
    Advance({CreateMouseEvent(EInputEventType::MouseMove, FVector2(135.0f, 70.0f))});
    EXPECT_GT(View->GetScrollOffset(), beforeDrag);

    Advance({CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(135.0f, 70.0f))});
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(ListViewTest, UnrealizedRowsClearFocusAndMouseCapture)
{
    std::vector<std::shared_ptr<TestRowWidget>> rows;
    View->SetItemCount(10);
    View->SetOnGenerateRow([&](std::size_t index) {
        auto row = std::make_shared<TestRowWidget>(28.0f);
        if (index == 0) {
            row->bRequestFocusOnMouseDown = true;
            row->bRequestCaptureOnMouseDown = true;
        }
        rows.push_back(row);
        return row;
    });

    Advance({});
    ASSERT_FALSE(rows.empty());

    const FVector2 clickPoint = rows.front()->GetGeometry().GetCenter();
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, clickPoint)});
    EXPECT_EQ(App->GetKeyboardFocus(), rows.front());
    EXPECT_EQ(App->GetMouseCapture(), rows.front());

    View->SetScrollOffset(500.0f);
    Advance({});
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(ListViewTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    auto themedView = std::make_shared<ImListView>();
    App->SetRootWidget(themedView);
    const FListViewStyle expectedStyle = ResolveListViewStyle(App->GetStyleSet());

    const FListViewStyle& style = themedView->GetStyle();
    EXPECT_EQ(style.BackgroundColor.ToImU32(), expectedStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(style.ScrollbarThumbColor.ToImU32(), expectedStyle.ScrollbarThumbColor.ToImU32());
}

TEST_F(ListViewTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FListViewStyle explicitStyle = MakeCompactListStyle();
    explicitStyle.BackgroundColor = FColor::FromBytes(10, 20, 30);
    explicitStyle.ScrollbarThumbColor = FColor::FromBytes(40, 50, 60);
    View->SetStyle(explicitStyle);

    EXPECT_EQ(View->GetStyle().BackgroundColor.ToImU32(), explicitStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(View->GetStyle().ScrollbarThumbColor.ToImU32(), explicitStyle.ScrollbarThumbColor.ToImU32());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
