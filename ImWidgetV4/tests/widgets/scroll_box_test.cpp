#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

namespace {

class FixedSizeWidget : public ImWidget {
public:
    explicit FixedSizeWidget(const FVector2& minSize)
        : MinSize(minSize)
    {
        SetHitTestVisible(true);
    }

    FVector2 GetMinSize() const override
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
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown) {
            ++MouseDownCount;
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    int MouseDownCount = 0;
};

class FixedStackWidget : public ImWidget {
public:
    FixedStackWidget(float rowHeight, float width)
        : RowHeight(rowHeight)
        , Width(width)
    {
        SetHitTestVisible(true);
    }

    void AddRow(const std::shared_ptr<ImWidget>& child)
    {
        AddChild(child);
    }

    FVector2 GetMinSize() const override
    {
        return FVector2(Width, RowHeight * static_cast<float>(m_Children.size()));
    }

    void Paint(const FPaintContext& paintContext) override
    {
        for (std::size_t index = 0; index < m_Children.size(); ++index) {
            const auto& child = m_Children[index];
            if (!child) {
                continue;
            }

            child->SetGeometry(FGeometry(
                FVector2(m_Geometry.Position.X, m_Geometry.Position.Y + RowHeight * static_cast<float>(index)),
                FVector2(m_Geometry.Size.X, RowHeight)));
            child->Paint(paintContext);
        }
    }

    float RowHeight = 0.0f;
    float Width = 0.0f;
};

class ScrollBoxTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
        }

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(640.0f, 480.0f);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
    }

    void TearDown() override
    {
        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    FInputEvent MouseEvent(EInputEventType type, const FVector2& position)
    {
        FInputEvent event;
        event.Type = type;
        event.MousePosition = position;
        event.MouseButton = EMouseButton::Left;
        return event;
    }

    FInputEvent WheelEvent(const FVector2& position, const FVector2& delta)
    {
        FInputEvent event;
        event.Type = EInputEventType::MouseWheel;
        event.MousePosition = position;
        event.ScrollDelta = delta;
        return event;
    }

    void AdvanceWithDraw(ImApplication& app, const std::vector<FInputEvent>& events, const FVector2& viewportSize)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = viewportSize.ToImVec2();

        ImDrawList drawList(ImGui::GetDrawListSharedData());
        drawList._ResetForNewFrame();
        DrawContext drawContext(&drawList);

        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = viewportSize;
        frameContext.DrawContext_ = &drawContext;
        app.AdvanceFrame(frameContext);
    }
};

TEST_F(ScrollBoxTest, RelayoutComputesDualAxisScrollRangeAndClampsOffsets)
{
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    style.WheelScrollStep = 10.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 60.0f));

    auto content = std::make_shared<FixedSizeWidget>(FVector2(180.0f, 120.0f));
    scrollBox->SetContent(content);
    scrollBox->Relayout();

    EXPECT_EQ(scrollBox->GetMaxScrollOffset(), FVector2(92.0f, 72.0f));
    EXPECT_EQ(content->GetGeometry().Position, FVector2(0.0f, 0.0f));
    EXPECT_EQ(content->GetGeometry().Size, FVector2(180.0f, 120.0f));

    scrollBox->SetScrollOffset(FVector2(999.0f, 999.0f));
    EXPECT_EQ(scrollBox->GetScrollOffset(), FVector2(92.0f, 72.0f));
}

TEST_F(ScrollBoxTest, HitTestClipsContentOutsideViewportAndKeepsScrollbarAreaOnSelf)
{
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 60.0f));

    auto content = std::make_shared<FixedSizeWidget>(FVector2(180.0f, 120.0f));
    scrollBox->SetContent(content);
    scrollBox->Relayout();

    std::vector<std::shared_ptr<ImWidget>> contentPath;
    EXPECT_TRUE(scrollBox->BuildHitTestPath(FVector2(40.0f, 20.0f), contentPath));
    ASSERT_EQ(contentPath.size(), 2u);
    EXPECT_EQ(contentPath.back(), content);

    std::vector<std::shared_ptr<ImWidget>> scrollbarPath;
    EXPECT_TRUE(scrollBox->BuildHitTestPath(FVector2(95.0f, 20.0f), scrollbarPath));
    ASSERT_EQ(scrollbarPath.size(), 1u);
    EXPECT_EQ(scrollbarPath.back(), scrollBox);
}

TEST_F(ScrollBoxTest, WheelScrollMovesBothAxesAndUpdatesHoverOnlyWhenNeeded)
{
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    style.WheelScrollStep = 10.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 60.0f));
    scrollBox->SetContent(std::make_shared<FixedSizeWidget>(FVector2(180.0f, 120.0f)));

    FReply verticalReply = scrollBox->OnInputEvent(WheelEvent(FVector2(40.0f, 20.0f), FVector2(0.0f, -3.0f)));
    EXPECT_TRUE(verticalReply.IsHandled());
    EXPECT_EQ(scrollBox->GetScrollOffset(), FVector2(0.0f, 30.0f));

    FReply horizontalReply = scrollBox->OnInputEvent(WheelEvent(FVector2(40.0f, 20.0f), FVector2(-2.0f, 0.0f)));
    EXPECT_TRUE(horizontalReply.IsHandled());
    EXPECT_EQ(scrollBox->GetScrollOffset(), FVector2(20.0f, 30.0f));

    EXPECT_FALSE(scrollBox->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(95.0f, 20.0f))).IsHandled());
    EXPECT_FALSE(scrollBox->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(95.0f, 20.0f))).IsHandled());
}

TEST_F(ScrollBoxTest, ScrollbarThumbDragUpdatesOffsetAndUsesMouseCapture)
{
    auto app = std::make_shared<ImApplication>();
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetContent(std::make_shared<FixedSizeWidget>(FVector2(180.0f, 120.0f)));
    app->SetRootWidget(scrollBox);

    AdvanceWithDraw(*app, {}, FVector2(100.0f, 60.0f));

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseButtonDown, FVector2(95.0f, 12.0f))}, FVector2(100.0f, 60.0f));
    EXPECT_EQ(app->GetMouseCapture(), scrollBox);

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseMove, FVector2(95.0f, 45.0f))}, FVector2(100.0f, 60.0f));
    EXPECT_GT(scrollBox->GetScrollOffset().Y, 45.0f);

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseButtonUp, FVector2(95.0f, 45.0f))}, FVector2(100.0f, 60.0f));
    EXPECT_EQ(app->GetMouseCapture(), nullptr);
}

TEST_F(ScrollBoxTest, HorizontalScrollbarThumbDragClampsWhenCursorLeavesTrack)
{
    auto app = std::make_shared<ImApplication>();
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetContent(std::make_shared<FixedSizeWidget>(FVector2(220.0f, 120.0f)));
    app->SetRootWidget(scrollBox);

    AdvanceWithDraw(*app, {}, FVector2(120.0f, 80.0f));

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 75.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_EQ(app->GetMouseCapture(), scrollBox);

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseMove, FVector2(260.0f, 75.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_FLOAT_EQ(scrollBox->GetScrollOffset().X, scrollBox->GetMaxScrollOffset().X);

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseButtonUp, FVector2(260.0f, 75.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_EQ(app->GetMouseCapture(), nullptr);
}

TEST_F(ScrollBoxTest, ScrollToWidgetBringsDescendantIntoViewAndRejectsOutsiders)
{
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    scrollBox->SetStyle(style);
    scrollBox->SetGeometry(FGeometry(0.0f, 0.0f, 120.0f, 60.0f));

    auto stack = std::make_shared<FixedStackWidget>(40.0f, 100.0f);
    std::shared_ptr<FixedSizeWidget> lastRow;
    for (int index = 0; index < 4; ++index) {
        auto row = std::make_shared<FixedSizeWidget>(FVector2(100.0f, 40.0f));
        stack->AddRow(row);
        lastRow = row;
    }

    scrollBox->SetContent(stack);

    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();
    DrawContext drawContext(&drawList);
    FPaintContext paintContext(
        drawContext,
        scrollBox->GetGeometry(),
        nullptr,
        FVector2(0.0f, 0.0f),
        false,
        1.0f / 60.0f);
    scrollBox->Paint(paintContext);

    ASSERT_TRUE(scrollBox->ScrollToWidget(lastRow));
    EXPECT_FLOAT_EQ(scrollBox->GetScrollOffset().Y, 100.0f);

    auto outsider = std::make_shared<FixedSizeWidget>(FVector2(20.0f, 20.0f));
    EXPECT_FALSE(scrollBox->ScrollToWidget(outsider));
}

TEST_F(ScrollBoxTest, ApplicationRoutingScrollsAndStillForwardsClicksToContent)
{
    auto app = std::make_shared<ImApplication>();
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    style.WheelScrollStep = 12.0f;
    scrollBox->SetStyle(style);

    auto clickSpy = std::make_shared<ClickSpyWidget>(FVector2(220.0f, 120.0f));
    scrollBox->SetContent(clickSpy);
    app->SetRootWidget(scrollBox);

    AdvanceWithDraw(*app, {}, FVector2(120.0f, 80.0f));

    AdvanceWithDraw(*app, {WheelEvent(FVector2(40.0f, 30.0f), FVector2(0.0f, -2.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_GT(scrollBox->GetScrollOffset().Y, 0.0f);

    AdvanceWithDraw(*app, {MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_EQ(clickSpy->MouseDownCount, 1);
}

TEST_F(ScrollBoxTest, UsesThemeResolvedStyleByDefault)
{
    auto app = std::make_shared<ImApplication>();
    auto scrollBox = std::make_shared<ImScrollBox>();
    app->SetRootWidget(scrollBox);

    ASSERT_TRUE(app->SetActiveTheme("Dark"));
    const FScrollBoxStyle expectedStyle = ResolveScrollBoxStyle(app->GetStyleSet());

    const FScrollBoxStyle& style = scrollBox->GetStyle();
    EXPECT_EQ(style.BackgroundColor.ToImU32(), expectedStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(style.ScrollbarThumbColor.ToImU32(), expectedStyle.ScrollbarThumbColor.ToImU32());
}

TEST_F(ScrollBoxTest, ExplicitStyleOverridesTheme)
{
    auto scrollBox = std::make_shared<ImScrollBox>();
    FScrollBoxStyle explicitStyle;
    explicitStyle.BackgroundColor = FColor::FromBytes(11, 22, 33);
    explicitStyle.ScrollbarThumbColor = FColor::FromBytes(44, 55, 66);
    scrollBox->SetStyle(explicitStyle);

    EXPECT_EQ(scrollBox->GetStyle().BackgroundColor.ToImU32(), explicitStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(scrollBox->GetStyle().ScrollbarThumbColor.ToImU32(), explicitStyle.ScrollbarThumbColor.ToImU32());
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
