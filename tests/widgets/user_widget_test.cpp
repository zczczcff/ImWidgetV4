#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;

namespace {

class FixedSizeWidget : public ImWidget {
public:
    explicit FixedSizeWidget(const FVector2& minSize, const std::string& name = "")
        : MinSize(minSize)
    {
        SetName(name);
    }

    FVector2 GetMinSize() const override
    {
        return MinSize;
    }

private:
    FVector2 MinSize;
};

class InteractiveLeafWidget : public ImWidget {
public:
    explicit InteractiveLeafWidget(const std::string& name, std::vector<std::string>* log = nullptr)
        : Log(log)
    {
        SetName(name);
        SetSupportsKeyboardFocus(true);
    }

    void SetRequestFocusAndCapture(bool enabled)
    {
        bRequestFocusAndCapture = enabled;
    }

    FReply OnPreviewInputEvent(const FInputEvent& event) override
    {
        if (Log) {
            Log->push_back("preview:" + GetName() + ":" + std::to_string(static_cast<int>(event.Type)));
        }
        return FReply::Unhandled();
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (Log) {
            Log->push_back("bubble:" + GetName() + ":" + std::to_string(static_cast<int>(event.Type)));
        }

        if (bRequestFocusAndCapture && event.Type == EInputEventType::MouseButtonDown) {
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .CaptureMouse(shared_from_this(), EMouseButton::Left);
        }

        return FReply::Unhandled();
    }

private:
    std::vector<std::string>* Log = nullptr;
    bool bRequestFocusAndCapture = false;
};

class DeferredBuildUserWidget : public ImUserWidget {
public:
    int BuildCount = 0;
    int RebuiltCount = 0;

protected:
    Ptr RebuildWidget() override
    {
        ++BuildCount;

        auto root = std::make_shared<ImVerticalBox>();
        root->SetName("RootBox");
        root->SetSpacing(4.0f);

        auto nestedBox = std::make_shared<ImVerticalBox>();
        nestedBox->SetName("NestedBox");
        nestedBox->AddChild(std::make_shared<FixedSizeWidget>(FVector2(30.0f, 12.0f), "InnerLeaf"));
        nestedBox->AddChild(std::make_shared<FixedSizeWidget>(FVector2(18.0f, 9.0f), "OtherLeaf"));

        root->AddChild(std::make_shared<FixedSizeWidget>(FVector2(20.0f, 10.0f), "TopLeaf"));
        root->AddChild(nestedBox);
        return root;
    }

    void OnRootWidgetRebuilt() override
    {
        ++RebuiltCount;
    }
};

class LoggingUserWidget : public ImUserWidget {
public:
    explicit LoggingUserWidget(std::vector<std::string>* log)
        : Log(log)
    {
        SetName("UserWidget");
    }

    FReply OnPreviewInputEvent(const FInputEvent& event) override
    {
        if (Log) {
            Log->push_back("preview:user:" + std::to_string(static_cast<int>(event.Type)));
        }
        return FReply::Unhandled();
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (Log) {
            Log->push_back("bubble:user:" + std::to_string(static_cast<int>(event.Type)));
        }
        return FReply::Unhandled();
    }

private:
    std::vector<std::string>* Log = nullptr;
};

FInputEvent MouseEvent(EInputEventType type, const FVector2& position)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = EMouseButton::Left;
    return event;
}

} // namespace

TEST(UserWidgetTest, DefaultWidgetHasZeroMinSizeAndExplicitRootProxiesMinSize)
{
    ImUserWidget userWidget;
    EXPECT_FALSE(userWidget.IsRootBuilt());
    EXPECT_EQ(userWidget.GetMinSize(), FVector2(0.0f, 0.0f));
    EXPECT_TRUE(userWidget.IsRootBuilt());

    userWidget.SetRootWidget(std::make_shared<FixedSizeWidget>(FVector2(64.0f, 28.0f), "ExplicitRoot"));
    EXPECT_TRUE(userWidget.IsRootBuilt());
    EXPECT_EQ(userWidget.GetMinSize(), FVector2(64.0f, 28.0f));
    ASSERT_NE(userWidget.GetRootWidget(), nullptr);
    EXPECT_EQ(userWidget.GetRootWidget()->GetName(), "ExplicitRoot");
}

TEST(UserWidgetTest, DeferredBuildOccursOnFirstAccessAndRebuildRefreshesNamedCache)
{
    auto userWidget = std::make_shared<DeferredBuildUserWidget>();
    EXPECT_FALSE(userWidget->IsRootBuilt());
    EXPECT_EQ(userWidget->BuildCount, 0);
    EXPECT_EQ(userWidget->RebuiltCount, 0);

    const FVector2 minSize = userWidget->GetMinSize();
    EXPECT_TRUE(userWidget->IsRootBuilt());
    EXPECT_EQ(userWidget->BuildCount, 1);
    EXPECT_EQ(userWidget->RebuiltCount, 1);
    EXPECT_GT(minSize.X, 0.0f);
    EXPECT_GT(minSize.Y, 0.0f);

    auto firstInnerLeaf = userWidget->FindWidgetByName("InnerLeaf");
    ASSERT_NE(firstInnerLeaf, nullptr);
    auto nestedBox = userWidget->FindWidgetAs<ImVerticalBox>("NestedBox");
    ASSERT_NE(nestedBox, nullptr);
    EXPECT_EQ(nestedBox->GetName(), "NestedBox");
    EXPECT_EQ(userWidget->FindWidgetAs<FixedSizeWidget>("NestedBox"), nullptr);

    userWidget->Rebuild();
    EXPECT_EQ(userWidget->BuildCount, 2);
    EXPECT_EQ(userWidget->RebuiltCount, 2);

    auto rebuiltInnerLeaf = userWidget->FindWidgetByName("InnerLeaf");
    ASSERT_NE(rebuiltInnerLeaf, nullptr);
    EXPECT_NE(firstInnerLeaf, rebuiltInnerLeaf);
}

TEST(UserWidgetTest, HitTestPathIncludesUserWidgetBeforeInternalRoot)
{
    auto leaf = std::make_shared<FixedSizeWidget>(FVector2(40.0f, 20.0f), "Leaf");
    auto userWidget = std::make_shared<ImUserWidget>();
    userWidget->SetName("Wrapper");
    userWidget->SetRootWidget(leaf);
    userWidget->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(100.0f, 60.0f)));

    std::vector<std::shared_ptr<ImWidget>> path;
    ASSERT_TRUE(userWidget->BuildHitTestPath(FVector2(20.0f, 20.0f), path));
    ASSERT_EQ(path.size(), 2u);
    EXPECT_EQ(path[0], userWidget);
    EXPECT_EQ(path[1], leaf);
}

TEST(UserWidgetTest, ApplicationRoutesEventsThroughUserWidgetAndInternalLeaf)
{
    auto application = std::make_shared<ImApplication>();
    std::vector<std::string> log;

    auto userWidget = std::make_shared<LoggingUserWidget>(&log);
    auto leaf = std::make_shared<InteractiveLeafWidget>("Leaf", &log);
    userWidget->SetRootWidget(leaf);
    application->SetRootWidget(userWidget);

    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportSize = FVector2(180.0f, 120.0f);
    std::vector<FInputEvent> events {
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(32.0f, 24.0f))
    };
    frameContext.InputEvents = &events;
    application->AdvanceFrame(frameContext);

    ASSERT_EQ(log.size(), 4u);
    EXPECT_EQ(log[0], "preview:user:4");
    EXPECT_EQ(log[1], "preview:Leaf:4");
    EXPECT_EQ(log[2], "bubble:Leaf:4");
    EXPECT_EQ(log[3], "bubble:user:4");
}

TEST(UserWidgetTest, ReplacingRootClearsFocusAndCaptureFromDetachedSubtree)
{
    auto application = std::make_shared<ImApplication>();
    auto userWidget = std::make_shared<ImUserWidget>();
    auto interactiveLeaf = std::make_shared<InteractiveLeafWidget>("FocusLeaf");
    interactiveLeaf->SetRequestFocusAndCapture(true);
    userWidget->SetRootWidget(interactiveLeaf);
    application->SetRootWidget(userWidget);

    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportSize = FVector2(180.0f, 120.0f);
    std::vector<FInputEvent> events {
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))
    };
    frameContext.InputEvents = &events;
    application->AdvanceFrame(frameContext);

    EXPECT_EQ(application->GetKeyboardFocus(), interactiveLeaf);
    EXPECT_EQ(application->GetMouseCapture(), interactiveLeaf);

    userWidget->SetRootWidget(std::make_shared<FixedSizeWidget>(FVector2(40.0f, 20.0f), "ReplacementRoot"));
    EXPECT_EQ(application->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(application->GetMouseCapture(), nullptr);
}

TEST(UserWidgetTest, VerticalBoxCanLayoutAUserWidgetLikeAnyOtherChild)
{
    auto box = std::make_shared<ImVerticalBox>();
    box->SetSpacing(6.0f);

    auto userWidget = std::make_shared<ImUserWidget>();
    userWidget->SetRootWidget(std::make_shared<FixedSizeWidget>(FVector2(70.0f, 22.0f), "InnerRoot"));
    box->AddChild(userWidget);

    box->SetGeometry(FGeometry(FVector2(10.0f, 15.0f), FVector2(200.0f, 120.0f)));
    box->Relayout();

    EXPECT_EQ(userWidget->GetGeometry().Position, FVector2(10.0f, 15.0f));
    EXPECT_GE(userWidget->GetGeometry().Size.X, 70.0f);
    EXPECT_GE(userWidget->GetGeometry().Size.Y, 22.0f);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
