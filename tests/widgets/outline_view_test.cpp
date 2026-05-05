#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/OutlineView.h>
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

class TestOutlineContentWidget : public ImWidget {
public:
    explicit TestOutlineContentWidget(float minHeight = 20.0f)
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

        if (event.Type == EInputEventType::MouseButtonUp && bReleaseCaptureOnMouseUp) {
            return FReply::Handled().ReleaseMouseCapture();
        }

        return FReply::Unhandled();
    }

    float MinHeight = 20.0f;
    int MouseDownCount = 0;
    bool bRequestFocusOnMouseDown = false;
    bool bRequestCaptureOnMouseDown = false;
    bool bReleaseCaptureOnMouseUp = false;
};

class OutlineViewTest : public ::testing::Test {
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
        View = std::make_shared<ImOutlineView>();

        FOutlineViewStyle style;
        style.Padding = FMargin(0.0f);
        style.RowPadding = FMargin(4.0f, 4.0f, 2.0f, 2.0f);
        style.BorderThickness = 0.0f;
        style.RowMinHeight = 24.0f;
        style.ScrollbarThickness = 10.0f;
        style.ScrollbarPadding = 2.0f;
        style.WheelScrollStep = 18.0f;
        style.MinDesiredSize = FVector2(140.0f, 90.0f);
        View->SetStyle(style);

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
    std::shared_ptr<ImOutlineView> View;
};

} // namespace

TEST_F(OutlineViewTest, ChildWidgetsAffectRowHeightAndSelectionAutoExpandsAncestors)
{
    auto tallWidget = std::make_shared<TestOutlineContentWidget>(44.0f);
    auto shortWidget = std::make_shared<TestOutlineContentWidget>(12.0f);
    ImOutlineItem* root = View->AddRootItem(tallWidget);
    ImOutlineItem* child = View->AddChildItem(root, shortWidget);

    View->SetSelectedItem(child);
    Advance({});
    EXPECT_GT(tallWidget->GetGeometry().Size.Y, shortWidget->GetGeometry().Size.Y);
    EXPECT_EQ(View->GetSelectedItem(), child);
    EXPECT_TRUE(root->Expanded);
}

TEST_F(OutlineViewTest, ClickingContentSelectsRowBeforeRoutingToContentWidget)
{
    auto rootWidget = std::make_shared<TestOutlineContentWidget>(28.0f);
    auto childWidget = std::make_shared<TestOutlineContentWidget>(28.0f);
    ImOutlineItem* root = View->AddRootItem(rootWidget);
    ImOutlineItem* child = View->AddChildItem(root, childWidget);

    Advance({});
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(8.0f, 12.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(8.0f, 12.0f))
    });
    Advance({});

    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, childWidget->GetGeometry().GetCenter()),
        CreateMouseEvent(EInputEventType::MouseButtonUp, childWidget->GetGeometry().GetCenter())
    });

    EXPECT_EQ(View->GetSelectedItem(), child);
    EXPECT_EQ(childWidget->MouseDownCount, 1);
}

TEST_F(OutlineViewTest, RightClickSelectsItemAndBroadcastsContextMenuRequest)
{
    auto rootWidget = std::make_shared<TestOutlineContentWidget>(28.0f);
    ImOutlineItem* root = View->AddRootItem(rootWidget);

    ImOutlineItem* contextItem = nullptr;
    FVector2 contextPosition;
    View->OnItemContextMenuRequested.AddLambda([&](ImOutlineView&, ImOutlineItem& item, FVector2 position) {
        contextItem = &item;
        contextPosition = position;
    });

    Advance({});
    const FVector2 clickPoint = rootWidget->GetGeometry().GetCenter();
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, clickPoint, EMouseButton::Right)});

    EXPECT_EQ(View->GetSelectedItem(), root);
    ASSERT_NE(contextItem, nullptr);
    EXPECT_EQ(contextItem, root);
    EXPECT_EQ(contextPosition, clickPoint);
}

TEST_F(OutlineViewTest, KeyboardNavigationAndScrollingWorkTogether)
{
    std::vector<std::shared_ptr<TestOutlineContentWidget>> widgets;
    for (int index = 0; index < 8; ++index) {
        auto widget = std::make_shared<TestOutlineContentWidget>(22.0f);
        widgets.push_back(widget);
        View->AddRootItem(widget);
    }

    Advance({});
    App->SetKeyboardFocus(View);

    Advance({CreateKeyEvent(EKey::Down)});
    EXPECT_NE(View->GetSelectedItem(), nullptr);

    Advance({CreateKeyEvent(EKey::End)});
    EXPECT_TRUE(View->ScrollToItem(View->GetSelectedItem()));

    const float beforeWheel = View->GetScrollOffset();
    Advance({CreateWheelEvent(FVector2(40.0f, 40.0f), -2.0f)});
    EXPECT_GE(View->GetScrollOffset(), beforeWheel);
}

TEST_F(OutlineViewTest, RemovingFocusedCapturedSubtreeClearsApplicationInteractionState)
{
    auto rootWidget = std::make_shared<TestOutlineContentWidget>(28.0f);
    auto childWidget = std::make_shared<TestOutlineContentWidget>(28.0f);
    childWidget->bRequestFocusOnMouseDown = true;
    childWidget->bRequestCaptureOnMouseDown = true;
    childWidget->bReleaseCaptureOnMouseUp = false;

    ImOutlineItem* root = View->AddRootItem(rootWidget);
    ImOutlineItem* child = View->AddChildItem(root, childWidget);

    Advance({});
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(8.0f, 12.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(8.0f, 12.0f))
    });
    Advance({});

    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, childWidget->GetGeometry().GetCenter())});
    EXPECT_EQ(App->GetKeyboardFocus(), childWidget);
    EXPECT_EQ(App->GetMouseCapture(), childWidget);

    View->RemoveItem(root);
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
    EXPECT_EQ(View->GetSelectedItem(), nullptr);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
