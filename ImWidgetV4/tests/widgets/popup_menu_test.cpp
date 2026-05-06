#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

namespace {

class FixedHostWidget : public ImWidget {
public:
    FixedHostWidget()
    {
        SetHitTestVisible(true);
    }

    void SetChild(const std::shared_ptr<ImWidget>& child)
    {
        ClearChildren();
        Child_ = child;
        if (Child_) {
            AddChild(Child_);
            Child_->SetGeometry(ChildGeometry_);
        }
    }

    void SetChildGeometry(const FGeometry& geometry)
    {
        ChildGeometry_ = geometry;
        if (Child_) {
            Child_->SetGeometry(geometry);
        }
    }

    void Paint(const FPaintContext& paintContext) override
    {
        if (Child_) {
            Child_->SetGeometry(ChildGeometry_);
            Child_->Paint(paintContext);
        }
    }

private:
    std::shared_ptr<ImWidget> Child_;
    FGeometry ChildGeometry_;
};

} // namespace

class PopupMenuTest : public ::testing::Test {
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

        App_ = std::make_shared<ImApplication>();
        Host_ = std::make_shared<FixedHostWidget>();
        Menu_ = std::make_shared<ImPopupMenu>();
        Host_->SetChildGeometry(FGeometry(FVector2(20.0f, 20.0f), FVector2(220.0f, 160.0f)));
        Host_->SetChild(Menu_);
        App_->SetRootWidget(Host_);
    }

    void TearDown() override
    {
        Menu_.reset();
        Host_.reset();
        App_.reset();

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

    void Advance(const std::vector<FInputEvent>& events)
    {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(320.0f, 220.0f);
        App_->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImWindow> GetTopmostPopupWindow() const
    {
        std::shared_ptr<ImWindow> topmostPopup;
        for (const auto& window : App_->GetWindowManager().GetOpenWindows()) {
            if (window && window->GetKind() == EWindowKind::Popup) {
                topmostPopup = window;
            }
        }

        return topmostPopup;
    }

    FVector2 ResolveRowCenter(int rowIndex) const
    {
        const FPopupMenuStyle& style = Menu_->GetStyle();
        return FVector2(
            44.0f,
            20.0f + style.OuterPaddingY + style.RowHeight * (static_cast<float>(rowIndex) + 0.5f));
    }

    std::shared_ptr<ImApplication> App_;
    std::shared_ptr<FixedHostWidget> Host_;
    std::shared_ptr<ImPopupMenu> Menu_;
};

TEST_F(PopupMenuTest, EmptyMenuHasStableMinimumSize)
{
    const FVector2 minSize = Menu_->GetMinSize();
    EXPECT_FLOAT_EQ(minSize.X, 180.0f);
    EXPECT_FLOAT_EQ(minSize.Y, 36.0f);
}

TEST_F(PopupMenuTest, ClickingEnabledItemInvokesCallbackAndBroadcastsDelegate)
{
    int callbackCount = 0;
    int delegateIndex = -1;
    ImPopupMenu* delegateSender = nullptr;

    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {"Open", FImageBrush(), {}, true, false, [&]() { ++callbackCount; }});
    items.push_back(FPopupMenuItem {"Disabled", FImageBrush(), {}, false, false, [&]() { ++callbackCount; }});
    items.push_back(FPopupMenuItem {"", FImageBrush(), {}, true, true, nullptr});
    Menu_->SetItems(items);
    Menu_->OnItemInvoked.AddLambda([&](ImPopupMenu& menu, int index) {
        delegateSender = &menu;
        delegateIndex = index;
    });

    const FVector2 clickPoint = ResolveRowCenter(0);
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, clickPoint),
        MouseEvent(EInputEventType::MouseButtonUp, clickPoint)
    });

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(delegateIndex, 0);
    EXPECT_EQ(delegateSender, Menu_.get());
}

TEST_F(PopupMenuTest, DisabledItemAndSeparatorDoNotInvoke)
{
    int callbackCount = 0;
    int delegateCount = 0;

    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {"Disabled", FImageBrush(), {}, false, false, [&]() { ++callbackCount; }});
    items.push_back(FPopupMenuItem {"", FImageBrush(), {}, true, true, [&]() { ++callbackCount; }});
    items.push_back(FPopupMenuItem {"Enabled", FImageBrush(), {}, true, false, [&]() { ++callbackCount; }});
    Menu_->SetItems(items);
    Menu_->OnItemInvoked.AddLambda([&](ImPopupMenu&, int) {
        ++delegateCount;
    });

    const FVector2 disabledPoint = ResolveRowCenter(0);
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, disabledPoint),
        MouseEvent(EInputEventType::MouseButtonUp, disabledPoint)
    });

    const FVector2 separatorPoint = ResolveRowCenter(1);
    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, separatorPoint),
        MouseEvent(EInputEventType::MouseButtonUp, separatorPoint)
    });

    EXPECT_EQ(callbackCount, 0);
    EXPECT_EQ(delegateCount, 0);
}

TEST_F(PopupMenuTest, HoveringSubmenuItemOpensChildPopupAndLeafInvocationBubblesUp)
{
    int callbackCount = 0;
    int bubbledIndex = -1;
    ImPopupMenu* bubbledSender = nullptr;

    std::vector<FPopupMenuItem> subItems;
    subItems.push_back(FPopupMenuItem {"Child A", FImageBrush(), {}, true, false, [&]() { ++callbackCount; }});
    subItems.push_back(FPopupMenuItem {"Child B", FImageBrush(), {}, true, false, nullptr});

    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {"Parent", FImageBrush(), subItems, true, false, nullptr});
    items.push_back(FPopupMenuItem {"Leaf", FImageBrush(), {}, true, false, nullptr});
    Menu_->SetItems(items);
    Menu_->OnItemInvoked.AddLambda([&](ImPopupMenu& sender, int index) {
        bubbledSender = &sender;
        bubbledIndex = index;
    });

    const FVector2 parentPoint = ResolveRowCenter(0);
    Advance({MouseEvent(EInputEventType::MouseMove, parentPoint)});

    const std::shared_ptr<ImWindow> popupWindow = GetTopmostPopupWindow();
    ASSERT_NE(popupWindow, nullptr);
    EXPECT_EQ(popupWindow->GetKind(), EWindowKind::Popup);

    const FGeometry popupGeometry = popupWindow->GetContentGeometry();
    const FVector2 childPoint(
        popupGeometry.Position.X + 32.0f,
        popupGeometry.Position.Y + Menu_->GetStyle().OuterPaddingY + Menu_->GetStyle().RowHeight * 0.5f);

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, childPoint),
        MouseEvent(EInputEventType::MouseButtonUp, childPoint)
    });

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(bubbledIndex, 0);
    ASSERT_NE(bubbledSender, nullptr);
    EXPECT_NE(bubbledSender, Menu_.get());
}

TEST_F(PopupMenuTest, MovingToNonSubmenuItemClosesExistingChildPopup)
{
    std::vector<FPopupMenuItem> subItems;
    subItems.push_back(FPopupMenuItem {"Child", FImageBrush(), {}, true, false, nullptr});

    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {"Parent", FImageBrush(), subItems, true, false, nullptr});
    items.push_back(FPopupMenuItem {"Leaf", FImageBrush(), {}, true, false, nullptr});
    Menu_->SetItems(items);

    Advance({MouseEvent(EInputEventType::MouseMove, ResolveRowCenter(0))});
    ASSERT_NE(GetTopmostPopupWindow(), nullptr);

    Advance({MouseEvent(EInputEventType::MouseMove, ResolveRowCenter(1))});
    EXPECT_EQ(GetTopmostPopupWindow(), nullptr);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
