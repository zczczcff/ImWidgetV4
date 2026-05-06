#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <memory>

using namespace ImWidgetV4;

namespace {

class FixedSizeWidget : public ImWidget {
public:
    explicit FixedSizeWidget(const FVector2& minSize)
        : MinSize(minSize) {}

    virtual FVector2 GetMinSize() const override {
        return MinSize;
    }

    FVector2 MinSize;
};

class EventSpyWidget : public FixedSizeWidget {
public:
    explicit EventSpyWidget(const FVector2& minSize = FVector2(40.0f, 40.0f))
        : FixedSizeWidget(minSize) {}

    virtual FReply OnInputEvent(const FInputEvent& event) override {
        if (event.Type == EInputEventType::MouseButtonDown) {
            ++MouseDownCount;
        }

        if (event.Type == EInputEventType::MouseButtonUp) {
            ++MouseUpCount;
            if (GetGeometry().Contains(event.MousePosition)) {
                ++ClickCount;
                return FReply::Handled();
            }
        }

        return FReply::Unhandled();
    }

    int MouseDownCount = 0;
    int MouseUpCount = 0;
    int ClickCount = 0;
};

class TestHorizontalSplitter : public ImHorizontalSplitter {
public:
    int HoveredBarIndex() const { return GetHoveredBarIndex(); }
    int DraggingBarIndex() const { return GetDraggingBarIndex(); }
    const std::vector<FGeometry>& PartGeometries() const { return GetPartGeometries(); }
    const std::vector<FGeometry>& BarGeometries() const { return GetBarGeometries(); }
};

class TestVerticalSplitter : public ImVerticalSplitter {
public:
    int HoveredBarIndex() const { return GetHoveredBarIndex(); }
    int DraggingBarIndex() const { return GetDraggingBarIndex(); }
    const std::vector<FGeometry>& PartGeometries() const { return GetPartGeometries(); }
    const std::vector<FGeometry>& BarGeometries() const { return GetBarGeometries(); }
};

FInputEvent MouseEvent(
    EInputEventType type,
    const FVector2& position,
    EMouseButton button = EMouseButton::Left) {
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = button;
    return event;
}

void AdvanceApp(
    ImApplication& app,
    const std::vector<FInputEvent>& events,
    const FVector2& viewportSize) {
    FFrameContext frameContext;
    frameContext.InputEvents = &events;
    frameContext.FrameInfo.ViewportSize = viewportSize;
    app.AdvanceFrame(frameContext);
}

void ExpectFloatNear(float actual, float expected) {
    EXPECT_NEAR(actual, expected, 0.01f);
}

} // namespace

TEST(SplitterTest, HorizontalLayoutRespectsRatiosAndBarWidth) {
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 304.0f, 80.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 20.0f)), 1.0f, 10.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 20.0f)), 2.0f, 10.0f);
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ASSERT_EQ(splitter->BarGeometries().size(), 1u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.X, 100.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.X, 200.0f);
    ExpectFloatNear(splitter->BarGeometries()[0].Position.X, 100.0f);
    ExpectFloatNear(splitter->BarGeometries()[0].Size.X, 4.0f);
}

TEST(SplitterTest, VerticalLayoutRespectsRatiosAndBarHeight) {
    auto splitter = std::make_shared<TestVerticalSplitter>();
    FVerticalSplitterStyle style;
    style.BarHeight = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 90.0f, 308.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(20.0f, 10.0f)), 1.0f, 10.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(20.0f, 10.0f)), 1.0f, 10.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(20.0f, 10.0f)), 1.0f, 10.0f);
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 3u);
    ASSERT_EQ(splitter->BarGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.Y, 100.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.Y, 100.0f);
    ExpectFloatNear(splitter->PartGeometries()[2].Size.Y, 100.0f);
    ExpectFloatNear(splitter->BarGeometries()[0].Position.Y, 100.0f);
    ExpectFloatNear(splitter->BarGeometries()[1].Position.Y, 204.0f);
}

TEST(SplitterTest, HorizontalLayoutCompressesBelowTotalMinimumSize) {
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 80.0f, 60.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(0.0f, 10.0f)), 1.0f, 60.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(0.0f, 10.0f)), 1.0f, 40.0f);
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.X, 45.6f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.X, 30.4f);
}

TEST(SplitterTest, HorizontalMainAxisIgnoresChildDesiredWidthForResizeLimits) {
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 304.0f, 60.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(500.0f, 10.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(500.0f, 10.0f)), 1.0f, 30.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonDown, FVector2(151.0f, 20.0f)));
    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(400.0f, 20.0f)));
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.X, 270.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.X, 30.0f);
    ExpectFloatNear(splitter->GetMinSize().X, 64.0f);
}

TEST(SplitterTest, HorizontalHoverTracksOnlyBarChanges) {
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 308.0f, 80.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 10.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 10.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 10.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(101.0f, 20.0f)));
    EXPECT_EQ(splitter->HoveredBarIndex(), 0);

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(102.0f, 20.0f)));
    EXPECT_EQ(splitter->HoveredBarIndex(), 0);

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(205.0f, 20.0f)));
    EXPECT_EQ(splitter->HoveredBarIndex(), 1);

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseLeave, FVector2(400.0f, 20.0f)));
    EXPECT_EQ(splitter->HoveredBarIndex(), -1);
}

TEST(SplitterTest, ApplicationRoutesDragCaptureAndReleaseForHorizontalBar) {
    ImApplication app;
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 304.0f, 80.0f));
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->Relayout();
    app.SetRootWidget(splitter);

    AdvanceApp(app, {MouseEvent(EInputEventType::MouseButtonDown, FVector2(151.0f, 20.0f))}, FVector2(304.0f, 80.0f));
    EXPECT_EQ(app.GetMouseCapture(), splitter);
    EXPECT_EQ(splitter->DraggingBarIndex(), 0);

    AdvanceApp(app, {MouseEvent(EInputEventType::MouseMove, FVector2(400.0f, 20.0f))}, FVector2(304.0f, 80.0f));
    splitter->Relayout();
    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.X, 270.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.X, 30.0f);

    AdvanceApp(app, {MouseEvent(EInputEventType::MouseButtonUp, FVector2(400.0f, 20.0f))}, FVector2(304.0f, 80.0f));
    EXPECT_EQ(app.GetMouseCapture(), nullptr);
    EXPECT_EQ(splitter->DraggingBarIndex(), -1);
}

TEST(SplitterTest, PaneContentStillReceivesApplicationRoutedClicks) {
    ImApplication app;
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 304.0f, 80.0f));

    auto leftPane = std::make_shared<EventSpyWidget>();
    auto rightPane = std::make_shared<EventSpyWidget>();
    splitter->AddPart(leftPane, 1.0f, 30.0f);
    splitter->AddPart(rightPane, 1.0f, 30.0f);
    splitter->Relayout();
    app.SetRootWidget(splitter);

    AdvanceApp(app, {
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(20.0f, 20.0f))
    }, FVector2(304.0f, 80.0f));

    EXPECT_EQ(leftPane->MouseDownCount, 1);
    EXPECT_EQ(leftPane->MouseUpCount, 1);
    EXPECT_EQ(leftPane->ClickCount, 1);
    EXPECT_EQ(rightPane->ClickCount, 0);
    EXPECT_EQ(app.GetMouseCapture(), nullptr);
}

TEST(SplitterTest, VerticalDragUpdatesAdjacentHeightsOnly) {
    auto splitter = std::make_shared<TestVerticalSplitter>();
    FVerticalSplitterStyle style;
    style.BarHeight = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 304.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 151.0f)));
    EXPECT_EQ(splitter->DraggingBarIndex(), 0);
    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 181.0f)));
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.Y, 180.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.Y, 120.0f);

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonUp, FVector2(20.0f, 181.0f)));
    EXPECT_EQ(splitter->DraggingBarIndex(), -1);
}

TEST(SplitterTest, VerticalMainAxisIgnoresChildDesiredHeightForResizeLimits) {
    auto splitter = std::make_shared<TestVerticalSplitter>();
    FVerticalSplitterStyle style;
    style.BarHeight = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 304.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 500.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 500.0f)), 1.0f, 30.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 151.0f)));
    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 400.0f)));
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    ExpectFloatNear(splitter->PartGeometries()[0].Size.Y, 270.0f);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.Y, 30.0f);
    ExpectFloatNear(splitter->GetMinSize().Y, 64.0f);
}

TEST(SplitterTest, HorizontalDragKeepsStableWidthsAcrossRepeatedMoves) {
    auto splitter = std::make_shared<TestHorizontalSplitter>();
    FHorizontalSplitterStyle style;
    style.BarWidth = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 304.0f, 80.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 3.0f, 30.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonDown, FVector2(76.0f, 20.0f)));
    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(116.0f, 20.0f)));
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    const float firstLeftWidth = splitter->PartGeometries()[0].Size.X;
    const float firstRightWidth = splitter->PartGeometries()[1].Size.X;

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(116.0f, 20.0f)));
    splitter->Relayout();

    ExpectFloatNear(splitter->PartGeometries()[0].Size.X, firstLeftWidth);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.X, firstRightWidth);
}

TEST(SplitterTest, VerticalDragKeepsStableHeightsAcrossRepeatedMoves) {
    auto splitter = std::make_shared<TestVerticalSplitter>();
    FVerticalSplitterStyle style;
    style.BarHeight = 4.0f;
    splitter->SetSplitterStyle(style);
    splitter->SetGeometry(FGeometry(0.0f, 0.0f, 100.0f, 304.0f));

    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 1.0f, 30.0f);
    splitter->AddPart(std::make_shared<FixedSizeWidget>(FVector2(10.0f, 10.0f)), 3.0f, 30.0f);
    splitter->Relayout();

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 76.0f)));
    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 116.0f)));
    splitter->Relayout();

    ASSERT_EQ(splitter->PartGeometries().size(), 2u);
    const float firstTopHeight = splitter->PartGeometries()[0].Size.Y;
    const float firstBottomHeight = splitter->PartGeometries()[1].Size.Y;

    splitter->OnInputEvent(MouseEvent(EInputEventType::MouseMove, FVector2(20.0f, 116.0f)));
    splitter->Relayout();

    ExpectFloatNear(splitter->PartGeometries()[0].Size.Y, firstTopHeight);
    ExpectFloatNear(splitter->PartGeometries()[1].Size.Y, firstBottomHeight);
}
