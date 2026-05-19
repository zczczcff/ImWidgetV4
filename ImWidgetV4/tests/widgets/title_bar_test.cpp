#include <gtest/gtest.h>
#include <algorithm>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>

using namespace ImWidgetV4;

namespace {

class MockTitleBarBackend : public ImApplicationBackend {
public:
    bool Initialize() override { return true; }
    void Shutdown() override {}
    void Run() override {}
    bool ShouldClose() const override { return false; }
    void SetWindowTitle(const std::string&) override {}
    void SetWindowSize(int, int) override {}
    void GetWindowSize(int& width, int& height) const override
    {
        width = 0;
        height = 0;
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
    void RequestClose() override {}
    std::string GetBackendName() const override { return "MockTitleBar"; }
    ImTextureID CreateTextureFromRGBA(const std::uint8_t*, int, int) override { return nullptr; }
    void ReleaseTexture(ImTextureID) override {}

    bool SupportsHostWindowDrag() const override { return bSupportsDrag; }
    bool SupportsHostWindowMinimize() const override { return bSupportsMinimize; }
    bool SupportsHostWindowMaximize() const override { return bSupportsMaximize; }
    bool SupportsHostWindowClose() const override { return bSupportsClose; }
    bool IsHostWindowMaximized() const override { return bMaximized; }
    bool BeginHostWindowDrag() override
    {
        ++BeginDragCalls;
        return bSupportsDrag;
    }
    bool MinimizeHostWindow() override
    {
        ++MinimizeCalls;
        return bSupportsMinimize;
    }
    bool ToggleHostWindowMaximize() override
    {
        ++ToggleMaximizeCalls;
        if (bSupportsMaximize) {
            bMaximized = !bMaximized;
        }
        return bSupportsMaximize;
    }
    bool CloseHostWindow() override
    {
        ++CloseCalls;
        return bSupportsClose;
    }

    ImApplication* Application = nullptr;
    bool bSupportsDrag = true;
    bool bSupportsMinimize = true;
    bool bSupportsMaximize = true;
    bool bSupportsClose = true;
    bool bMaximized = false;
    int BeginDragCalls = 0;
    int MinimizeCalls = 0;
    int ToggleMaximizeCalls = 0;
    int CloseCalls = 0;
};

FInputEvent MouseEvent(EInputEventType type, const FVector2& position, double timestamp = 0.0)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = EMouseButton::Left;
    event.Timestamp = timestamp;
    return event;
}

class TitleBarTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        App = std::make_shared<ImApplication>();
        Backend.SetApplication(App.get());
        TitleBar = std::make_shared<ImTitleBar>();
        App->SetRootWidget(TitleBar);
    }

    void Advance(
        const std::vector<FInputEvent>& events = {},
        const FVector2& viewportSize = FVector2(420.0f, 44.0f))
    {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = viewportSize;
        App->AdvanceFrame(frameContext);
    }

    FVector2 GetButtonCenter(const FGeometry& geometry) const
    {
        return FVector2(
            geometry.Position.X + geometry.Size.X * 0.5f,
            geometry.Position.Y + geometry.Size.Y * 0.5f);
    }

    FVector2 FindHitPointForWidget(const std::shared_ptr<ImWidget>& widget) const
    {
        std::vector<std::shared_ptr<ImWidget>> hitPath;
        for (int y = 2; y < 42; ++y) {
            for (int x = 2; x < 418; ++x) {
                hitPath.clear();
                if (TitleBar->BuildHitTestPath(FVector2(static_cast<float>(x), static_cast<float>(y)), hitPath) &&
                    std::find(hitPath.begin(), hitPath.end(), widget) != hitPath.end()) {
                    return FVector2(static_cast<float>(x), static_cast<float>(y));
                }
            }
        }

        return FVector2(0.0f, 0.0f);
    }

    std::shared_ptr<ImApplication> App;
    MockTitleBarBackend Backend;
    std::shared_ptr<ImTitleBar> TitleBar;
};

} // namespace

TEST_F(TitleBarTest, ChildClickDoesNotTriggerHostDrag)
{
    auto childButton = std::make_shared<ImButton>();
    childButton->SetText("Menu");
    bool bClicked = false;
    childButton->OnClicked.AddLambda([&bClicked](ImButton&) { bClicked = true; });
    TitleBar->AddLeadingItem(childButton);

    Advance();
    const FVector2 clickPoint = FindHitPointForWidget(childButton);
    ASSERT_NE(clickPoint, FVector2(0.0f, 0.0f));
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, clickPoint),
        MouseEvent(EInputEventType::MouseButtonUp, clickPoint)
    });

    EXPECT_TRUE(bClicked);
    EXPECT_EQ(Backend.BeginDragCalls, 0);
}

TEST_F(TitleBarTest, DefaultStyleHasNoOuterPaddingOrItemSpacing)
{
    const FTitleBarStyle& style = TitleBar->GetStyle();
    EXPECT_FLOAT_EQ(style.Padding.Left, 0.0f);
    EXPECT_FLOAT_EQ(style.Padding.Top, 0.0f);
    EXPECT_FLOAT_EQ(style.Padding.Right, 0.0f);
    EXPECT_FLOAT_EQ(style.Padding.Bottom, 0.0f);
    EXPECT_FLOAT_EQ(style.ItemSpacing, 0.0f);
}

TEST_F(TitleBarTest, RepeatedDragRegionPressesAlwaysBeginHostWindowDrag)
{
    Advance();
    const FVector2 dragPoint(200.0f, 18.0f);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, dragPoint, 0.0)});
    EXPECT_EQ(Backend.BeginDragCalls, 1);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 0);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, dragPoint, 0.2)});
    EXPECT_EQ(Backend.BeginDragCalls, 2);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 0);

    Advance({MouseEvent(EInputEventType::MouseButtonDown, dragPoint, 0.4)});
    EXPECT_EQ(Backend.BeginDragCalls, 3);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 0);
}

TEST_F(TitleBarTest, NonInteractiveChildAreaCanBeginHostWindowDrag)
{
    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("Project");
    titleText->SetHitTestVisible(true);
    TitleBar->AddLeadingItem(titleText);

    Advance();
    const FVector2 clickPoint = FindHitPointForWidget(titleText);
    ASSERT_NE(clickPoint, FVector2(0.0f, 0.0f));

    Advance({MouseEvent(EInputEventType::MouseButtonDown, clickPoint, 0.0)});

    EXPECT_EQ(Backend.BeginDragCalls, 1);
}

TEST_F(TitleBarTest, ChildItemsFillTitleBarHeightWithoutVerticalGap)
{
    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("Project");
    TitleBar->AddLeadingItem(titleText);

    Advance();
    std::vector<std::shared_ptr<ImWidget>> hitPath;
    ASSERT_TRUE(TitleBar->BuildHitTestPath(FVector2(2.0f, 2.0f), hitPath));

    const FGeometry& titleGeometry = TitleBar->GetGeometry();
    const FGeometry& itemGeometry = titleText->GetGeometry();
    EXPECT_FLOAT_EQ(itemGeometry.Position.Y, titleGeometry.Position.Y);
    EXPECT_FLOAT_EQ(itemGeometry.Size.Y, titleGeometry.Size.Y);
}

TEST_F(TitleBarTest, SystemButtonsInvokeBackendCapabilitiesWhenSupported)
{
    Advance();

    const FTitleBarStyle& style = TitleBar->GetStyle();
    const float closeX = 420.0f - style.Padding.Right - style.SystemButtonSize * 0.5f;
    const float maxX = closeX - style.SystemButtonSize;
    const float minX = maxX - style.SystemButtonSize;
    const float y = 18.0f;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(minX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(minX, y))
    });
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(maxX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(maxX, y))
    });
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(closeX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(closeX, y))
    });

    EXPECT_EQ(Backend.MinimizeCalls, 1);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 1);
    EXPECT_EQ(Backend.CloseCalls, 1);
}

TEST_F(TitleBarTest, DisabledHostWindowActionsKeepSystemButtonsInteractiveWithoutInvokingBackend)
{
    TitleBar->SetHostWindowActionsEnabled(false);
    Advance();

    const FTitleBarStyle& style = TitleBar->GetStyle();
    const float closeX = 420.0f - style.Padding.Right - style.SystemButtonSize * 0.5f;
    const float maxX = closeX - style.SystemButtonSize;
    const float minX = maxX - style.SystemButtonSize;
    const float y = 18.0f;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(minX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(minX, y))
    });
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(maxX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(maxX, y))
    });
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(closeX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(closeX, y))
    });

    EXPECT_EQ(Backend.MinimizeCalls, 0);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 0);
    EXPECT_EQ(Backend.CloseCalls, 0);
    EXPECT_EQ(Backend.BeginDragCalls, 0);
}

TEST_F(TitleBarTest, UnsupportedBackendHidesSystemButtonsFromHitTesting)
{
    Backend.bSupportsDrag = false;
    Backend.bSupportsMinimize = false;
    Backend.bSupportsMaximize = false;
    Backend.bSupportsClose = false;

    Advance();
    const FVector2 rightSidePoint(408.0f, 18.0f);
    std::vector<std::shared_ptr<ImWidget>> hitPath;
    ASSERT_TRUE(TitleBar->BuildHitTestPath(rightSidePoint, hitPath));
    ASSERT_FALSE(hitPath.empty());
    EXPECT_EQ(hitPath.back(), TitleBar);

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, rightSidePoint),
        MouseEvent(EInputEventType::MouseButtonUp, rightSidePoint)
    });

    EXPECT_EQ(Backend.MinimizeCalls, 0);
    EXPECT_EQ(Backend.ToggleMaximizeCalls, 0);
    EXPECT_EQ(Backend.CloseCalls, 0);
    EXPECT_EQ(Backend.BeginDragCalls, 0);
}

TEST_F(TitleBarTest, SystemButtonsRelayoutWhenViewportChanges)
{
    Advance();
    Advance({}, FVector2(860.0f, 44.0f));

    const FTitleBarStyle& style = TitleBar->GetStyle();
    const float closeX = 860.0f - style.Padding.Right - style.SystemButtonSize * 0.5f;
    const float y = 18.0f;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(closeX, y)),
        MouseEvent(EInputEventType::MouseButtonUp, FVector2(closeX, y))
    }, FVector2(860.0f, 44.0f));

    EXPECT_EQ(Backend.CloseCalls, 1);
}

TEST_F(TitleBarTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));
    const FTitleBarStyle expectedStyle = ResolveTitleBarStyle(App->GetStyleSet());

    const FTitleBarStyle& style = TitleBar->GetStyle();
    EXPECT_EQ(style.BackgroundColor.ToImU32(), expectedStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(style.BorderColor.ToImU32(), expectedStyle.BorderColor.ToImU32());
    EXPECT_FLOAT_EQ(style.BorderThickness, expectedStyle.BorderThickness);
    EXPECT_EQ(style.SystemButtonGlyphColor.ToImU32(), expectedStyle.SystemButtonGlyphColor.ToImU32());
}

TEST_F(TitleBarTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));

    FTitleBarStyle explicitStyle = TitleBar->GetStyle();
    explicitStyle.BackgroundColor = FColor::FromBytes(1, 2, 3);
    explicitStyle.BorderColor = FColor::FromBytes(4, 5, 6);
    explicitStyle.BorderThickness = 3.0f;
    explicitStyle.SystemButtonGlyphColor = FColor::FromBytes(7, 8, 9);
    TitleBar->SetStyle(explicitStyle);

    EXPECT_EQ(TitleBar->GetStyle().BackgroundColor.ToImU32(), explicitStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(TitleBar->GetStyle().BorderColor.ToImU32(), explicitStyle.BorderColor.ToImU32());
    EXPECT_FLOAT_EQ(TitleBar->GetStyle().BorderThickness, explicitStyle.BorderThickness);
    EXPECT_EQ(TitleBar->GetStyle().SystemButtonGlyphColor.ToImU32(), explicitStyle.SystemButtonGlyphColor.ToImU32());
}
