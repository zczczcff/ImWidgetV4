#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
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

class TextOutlineViewTest : public ::testing::Test {
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
        View = std::make_shared<ImTextOutlineView>();

        FTextOutlineViewStyle style;
        style.Padding = FMargin(0.0f);
        style.RowPadding = FMargin(4.0f, 4.0f, 2.0f, 2.0f);
        style.BorderThickness = 0.0f;
        style.RowHeight = 24.0f;
        style.ScrollbarThickness = 10.0f;
        style.ScrollbarPadding = 2.0f;
        style.WheelScrollStep = 18.0f;
        style.MinDesiredSize = FVector2(120.0f, 80.0f);
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

    void Advance(const std::vector<FInputEvent>& events, const FVector2& viewportSize = FVector2(120.0f, 80.0f))
    {
        DrawContext drawContext(ImGui::GetBackgroundDrawList());
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = viewportSize;
        frameContext.DrawContext_ = &drawContext;
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImTextOutlineView> View;
};

} // namespace

TEST_F(TextOutlineViewTest, SelectionExpandsAncestorsAndScrollsTargetIntoView)
{
    ImTextOutlineItem* root = View->AddRootItem("Root");
    ImTextOutlineItem* child = View->AddChildItem(root, "Child");
    ImTextOutlineItem* grandChild = View->AddChildItem(child, "GrandChild");
    View->AddRootItem("Sibling");

    Advance({});
    EXPECT_EQ(View->GetMaxScrollOffset(), 0.0f);

    View->SetSelectedItem(grandChild);

    EXPECT_EQ(View->GetSelectedItem(), grandChild);
    EXPECT_TRUE(root->Expanded);
    EXPECT_TRUE(child->Expanded);
    EXPECT_TRUE(View->ScrollToItem(grandChild));
}

TEST_F(TextOutlineViewTest, MouseAndKeyboardInteractionFollowOutlineSemantics)
{
    ImTextOutlineItem* root = View->AddRootItem("Root");
    ImTextOutlineItem* childA = View->AddChildItem(root, "Child A");
    ImTextOutlineItem* childB = View->AddChildItem(root, "Child B");
    ImTextOutlineItem* other = View->AddRootItem("Other");

    int expandedChangedCount = 0;
    ImTextOutlineItem* expandedSender = nullptr;
    bool lastExpandedState = false;
    View->OnItemExpandedChanged.AddLambda([&](ImTextOutlineView&, ImTextOutlineItem& item, bool expanded) {
        ++expandedChangedCount;
        expandedSender = &item;
        lastExpandedState = expanded;
    });

    ImTextOutlineItem* contextItem = nullptr;
    FVector2 contextPosition;
    View->OnItemContextMenuRequested.AddLambda([&](ImTextOutlineView&, ImTextOutlineItem& item, FVector2 position) {
        contextItem = &item;
        contextPosition = position;
    });

    Advance({});

    const FVector2 rootArrowPoint(8.0f, 12.0f);
    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, rootArrowPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, rootArrowPoint)
    });

    EXPECT_TRUE(root->Expanded);
    EXPECT_EQ(View->GetSelectedItem(), root);
    EXPECT_EQ(expandedChangedCount, 1);
    EXPECT_EQ(expandedSender, root);
    EXPECT_TRUE(lastExpandedState);

    Advance({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(34.0f, 36.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(34.0f, 36.0f))
    });
    EXPECT_EQ(View->GetSelectedItem(), childA);

    App->SetKeyboardFocus(View);
    Advance({CreateKeyEvent(EKey::Down)});
    EXPECT_EQ(View->GetSelectedItem(), childB);

    Advance({CreateKeyEvent(EKey::Right)});
    EXPECT_EQ(View->GetSelectedItem(), childB);

    Advance({CreateKeyEvent(EKey::End)});
    EXPECT_EQ(View->GetSelectedItem(), other);

    View->SetScrollOffset(0.0f);
    Advance({});
    Advance({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(34.0f, 36.0f), EMouseButton::Right)});
    ASSERT_NE(contextItem, nullptr);
    EXPECT_EQ(View->GetSelectedItem(), contextItem);
    EXPECT_TRUE(contextItem == root || contextItem == childA || contextItem == childB || contextItem == other);
    EXPECT_EQ(contextPosition, FVector2(34.0f, 36.0f));
}

TEST_F(TextOutlineViewTest, LongTextDoesNotWrapAndScrollbarInputClamps)
{
    for (int index = 0; index < 10; ++index) {
        View->AddRootItem(
            "A deliberately long single-line outline entry that should remain one row wide even when the view gets narrow.");
    }

    Advance({}, FVector2(120.0f, 80.0f));
    const float narrowScroll = View->GetMaxScrollOffset();
    ASSERT_GT(narrowScroll, 0.0f);

    Advance({}, FVector2(320.0f, 80.0f));
    const float wideScroll = View->GetMaxScrollOffset();
    EXPECT_FLOAT_EQ(narrowScroll, wideScroll);

    Advance({CreateWheelEvent(FVector2(40.0f, 40.0f), -2.0f)}, FVector2(120.0f, 80.0f));
    EXPECT_GT(View->GetScrollOffset(), 0.0f);

    bool startedDrag = false;
    for (int y = 0; y < 80; ++y) {
        const FReply reply = View->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(115.0f, static_cast<float>(y))));
        if (reply.IsHandled() && reply.MouseCaptureTarget == View) {
            startedDrag = true;
            break;
        }
    }
    ASSERT_TRUE(startedDrag);

    const float beforeDrag = View->GetScrollOffset();
    EXPECT_TRUE(View->OnInputEvent(CreateMouseEvent(EInputEventType::MouseMove, FVector2(115.0f, 60.0f))).IsHandled());
    EXPECT_GT(View->GetScrollOffset(), beforeDrag);

    EXPECT_TRUE(View->OnInputEvent(CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(115.0f, 60.0f))).IsHandled());
    EXPECT_LE(View->GetScrollOffset(), View->GetMaxScrollOffset());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
