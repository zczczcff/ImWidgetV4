#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

namespace {

class FixedSizeWidget : public ImWidget {
public:
    explicit FixedSizeWidget(const FVector2& minSize)
        : MinSize(minSize)
    {
    }

    virtual FVector2 GetMinSize() const override
    {
        return MinSize;
    }

    FVector2 MinSize;
};

class ClickSpyWidget : public FixedSizeWidget {
public:
    explicit ClickSpyWidget(const FVector2& minSize)
        : FixedSizeWidget(minSize)
    {
        SetHitTestVisible(true);
    }

    virtual FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown) {
            ++MouseDownCount;
            return FReply::Handled();
        }

        if (event.Type == EInputEventType::MouseButtonUp) {
            ++MouseUpCount;
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    int MouseDownCount = 0;
    int MouseUpCount = 0;
};

class CanvasEventSpy : public ImCanvasPanel {
public:
    virtual FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown) {
            ++MouseDownCount;
        }

        return FReply::Unhandled();
    }

    int MouseDownCount = 0;
};

FInputEvent MouseEvent(EInputEventType type, const FVector2& position)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = EMouseButton::Left;
    return event;
}

void AdvanceApp(ImApplication& app, const std::vector<FInputEvent>& events, const FVector2& viewportSize)
{
    FFrameContext frameContext;
    frameContext.InputEvents = &events;
    frameContext.FrameInfo.ViewportSize = viewportSize;
    app.AdvanceFrame(frameContext);
}

void AdvanceAppWithDraw(ImApplication& app, const std::vector<FInputEvent>& events, const FVector2& viewportSize)
{
    if (!ImGui::GetCurrentContext()) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        io.Fonts->Build();
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = viewportSize.ToImVec2();
    io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();
    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();
    DrawContext drawContext(&drawList);

    FFrameContext frameContext;
    frameContext.InputEvents = &events;
    frameContext.FrameInfo.ViewportSize = viewportSize;
    frameContext.DrawContext_ = &drawContext;
    app.AdvanceFrame(frameContext);

    ImGui::EndFrame();
}

} // namespace

TEST(CanvasPanelTest, AddChildAtPositionsChildUsingRelativeCoordinates)
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetGeometry(FGeometry(10.0f, 20.0f, 400.0f, 200.0f));

    auto child = std::make_shared<FixedSizeWidget>(FVector2(36.0f, 24.0f));
    ImCanvasPanelSlot* slot = canvas->AddChildAt(child, FVector2(0.25f, 0.5f));
    ASSERT_NE(slot, nullptr);

    canvas->Relayout();

    EXPECT_EQ(child->GetGeometry().Position, FVector2(110.0f, 120.0f));
    EXPECT_EQ(child->GetGeometry().Size, FVector2(36.0f, 24.0f));
    EXPECT_TRUE(slot->GetAutoSize());
}

TEST(CanvasPanelTest, AutoSizeChildrenKeepDesiredSizeWhenPanelResizes)
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    auto child = std::make_shared<FixedSizeWidget>(FVector2(80.0f, 30.0f));
    canvas->AddChildAt(child, FVector2(0.1f, 0.2f));

    canvas->SetGeometry(FGeometry(0.0f, 0.0f, 300.0f, 200.0f));
    canvas->Relayout();
    EXPECT_EQ(child->GetGeometry().Size, FVector2(80.0f, 30.0f));
    EXPECT_EQ(child->GetGeometry().Position, FVector2(30.0f, 40.0f));

    canvas->SetGeometry(FGeometry(0.0f, 0.0f, 500.0f, 400.0f));
    canvas->Relayout();
    EXPECT_EQ(child->GetGeometry().Size, FVector2(80.0f, 30.0f));
    EXPECT_EQ(child->GetGeometry().Position, FVector2(50.0f, 80.0f));
}

TEST(CanvasPanelTest, ExplicitRelativeSizeTracksPanelResize)
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    auto child = std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f));
    ImCanvasPanelSlot* slot = canvas->AddChildAt(child, FVector2(0.2f, 0.25f), FVector2(0.5f, 0.4f));
    ASSERT_NE(slot, nullptr);
    EXPECT_FALSE(slot->GetAutoSize());

    canvas->SetGeometry(FGeometry(20.0f, 40.0f, 200.0f, 100.0f));
    canvas->Relayout();
    EXPECT_EQ(child->GetGeometry().Position, FVector2(60.0f, 65.0f));
    EXPECT_EQ(child->GetGeometry().Size, FVector2(100.0f, 40.0f));

    canvas->SetGeometry(FGeometry(20.0f, 40.0f, 400.0f, 300.0f));
    canvas->Relayout();
    EXPECT_EQ(child->GetGeometry().Position, FVector2(100.0f, 115.0f));
    EXPECT_EQ(child->GetGeometry().Size, FVector2(200.0f, 120.0f));
}

TEST(CanvasPanelTest, MinSizeReturnsDesiredSize)
{
    ImCanvasPanel canvas;
    EXPECT_EQ(canvas.GetMinSize(), FVector2(100.0f, 100.0f));

    canvas.SetDesiredSize(FVector2(320.0f, 180.0f));
    EXPECT_EQ(canvas.GetMinSize(), FVector2(320.0f, 180.0f));
}

TEST(CanvasPanelTest, HitTestPrefersLastAddedOverlappingChild)
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetGeometry(FGeometry(0.0f, 0.0f, 300.0f, 200.0f));

    auto back = std::make_shared<FixedSizeWidget>(FVector2(20.0f, 20.0f));
    auto front = std::make_shared<FixedSizeWidget>(FVector2(20.0f, 20.0f));
    canvas->AddChildAt(back, FVector2(0.2f, 0.2f), FVector2(0.4f, 0.4f));
    canvas->AddChildAt(front, FVector2(0.25f, 0.25f), FVector2(0.4f, 0.4f));
    canvas->Relayout();

    std::vector<std::shared_ptr<ImWidget>> path;
    EXPECT_TRUE(canvas->BuildHitTestPath(FVector2(100.0f, 80.0f), path));
    ASSERT_EQ(path.size(), 2u);
    EXPECT_EQ(path.front(), canvas);
    EXPECT_EQ(path.back(), front);
}

TEST(CanvasPanelTest, EmptyAreaHitTestReturnsPanelOnly)
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetGeometry(FGeometry(0.0f, 0.0f, 300.0f, 200.0f));
    canvas->AddChildAt(std::make_shared<FixedSizeWidget>(FVector2(40.0f, 30.0f)), FVector2(0.1f, 0.1f));
    canvas->Relayout();

    std::vector<std::shared_ptr<ImWidget>> path;
    EXPECT_TRUE(canvas->BuildHitTestPath(FVector2(280.0f, 180.0f), path));
    ASSERT_EQ(path.size(), 1u);
    EXPECT_EQ(path.back(), canvas);
}

TEST(CanvasPanelTest, ApplicationRoutesOverlapClicksToTopmostChild)
{
    auto app = std::make_shared<ImApplication>();
    auto canvas = std::make_shared<ImCanvasPanel>();
    auto back = std::make_shared<ClickSpyWidget>(FVector2(20.0f, 20.0f));
    auto front = std::make_shared<ClickSpyWidget>(FVector2(20.0f, 20.0f));

    canvas->AddChildAt(back, FVector2(0.15f, 0.2f), FVector2(0.4f, 0.4f));
    canvas->AddChildAt(front, FVector2(0.2f, 0.25f), FVector2(0.4f, 0.4f));
    app->SetRootWidget(canvas);

    AdvanceAppWithDraw(*app, {}, FVector2(300.0f, 200.0f));
    AdvanceAppWithDraw(
        *app,
        {
            MouseEvent(EInputEventType::MouseButtonDown, FVector2(90.0f, 90.0f)),
            MouseEvent(EInputEventType::MouseButtonUp, FVector2(90.0f, 90.0f))
        },
        FVector2(300.0f, 200.0f));

    EXPECT_EQ(back->MouseDownCount, 0);
    EXPECT_EQ(back->MouseUpCount, 0);
    EXPECT_EQ(front->MouseDownCount, 1);
    EXPECT_EQ(front->MouseUpCount, 1);
}

TEST(CanvasPanelTest, ApplicationRoutesEmptyAreaToPanelAndUpdatesScaledGeometry)
{
    auto app = std::make_shared<ImApplication>();
    auto canvas = std::make_shared<CanvasEventSpy>();
    auto child = std::make_shared<FixedSizeWidget>(FVector2(20.0f, 20.0f));
    canvas->AddChildAt(child, FVector2(0.5f, 0.5f), FVector2(0.25f, 0.25f));
    app->SetRootWidget(canvas);

    AdvanceAppWithDraw(*app, {}, FVector2(200.0f, 120.0f));
    EXPECT_EQ(child->GetGeometry().Position, FVector2(100.0f, 60.0f));
    EXPECT_EQ(child->GetGeometry().Size, FVector2(50.0f, 30.0f));

    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, FVector2(10.0f, 10.0f))},
        FVector2(200.0f, 120.0f));
    EXPECT_EQ(canvas->MouseDownCount, 1);

    AdvanceAppWithDraw(*app, {}, FVector2(320.0f, 240.0f));
    EXPECT_EQ(child->GetGeometry().Position, FVector2(160.0f, 120.0f));
    EXPECT_EQ(child->GetGeometry().Size, FVector2(80.0f, 60.0f));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
