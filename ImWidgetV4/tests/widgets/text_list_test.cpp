#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

namespace {

FTextListStyle MakeCompactListStyle()
{
    FTextListStyle style;
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.MinDesiredSize = FVector2(120.0f, 60.0f);
    style.FontSize = 16.0f;
    style.LineSpacing = 1.0f;
    style.ScrollbarThickness = 10.0f;
    style.ScrollbarPadding = 2.0f;
    style.ThumbMinLength = 20.0f;
    style.WheelScrollStep = 12.0f;
    style.AutoScrollEdgePadding = 20.0f;
    style.AutoScrollSpeed = 8.0f;
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

FInputEvent CreateModifiedKeyEvent(EKey key, bool bCtrl)
{
    FInputEvent event;
    event.Type = EInputEventType::KeyDown;
    event.Key = key;
    event.Modifiers.bCtrl = bCtrl;
    return event;
}

class TextListTest : public ::testing::Test {
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
        io.SetClipboardTextFn = [](void*, const char* text) {
            ImGui::GetStateStorage()->SetVoidPtr(ImGui::GetID("TextListClipboard"), IM_NEW(std::string)(text != nullptr ? text : ""));
        };
        io.GetClipboardTextFn = [](void*) -> const char* {
            void* value = ImGui::GetStateStorage()->GetVoidPtr(ImGui::GetID("TextListClipboard"));
            if (value == nullptr) {
                return "";
            }
            return static_cast<std::string*>(value)->c_str();
        };
        ImGui::NewFrame();
    }

    void TearDown() override
    {
        if (ImGui::GetCurrentContext()) {
            void* value = ImGui::GetStateStorage()->GetVoidPtr(ImGui::GetID("TextListClipboard"));
            if (value != nullptr) {
                delete static_cast<std::string*>(value);
                ImGui::GetStateStorage()->SetVoidPtr(ImGui::GetID("TextListClipboard"), nullptr);
            }
            ImGui::EndFrame();
        }
    }

    void PaintList(ImTextList& list)
    {
        ImDrawList drawList(ImGui::GetDrawListSharedData());
        drawList._ResetForNewFrame();
        DrawContext drawContext(&drawList);
        FPaintContext paintContext(
            drawContext,
            list.GetGeometry(),
            nullptr,
            FVector2(0.0f, 0.0f),
            false,
            1.0f / 60.0f);
        list.Paint(paintContext);
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

TEST_F(TextListTest, TextManagementAndColorsAreConfigurable)
{
    ImTextList list;
    list.SetStyle(MakeCompactListStyle());
    list.SetItems({"Alpha", "Beta"});
    EXPECT_EQ(list.GetItems().size(), 2u);

    list.AddItem("Gamma");
    EXPECT_EQ(list.GetItems().size(), 3u);

    list.ModifyItem(1, "Beta Updated");
    EXPECT_EQ(list.GetItems()[1], "Beta Updated");

    list.SetItemColor(1, FColor::FromBytes(255, 214, 102));
    EXPECT_EQ(list.GetItemColor(1).ToImU32(), FColor::FromBytes(255, 214, 102).ToImU32());

    list.RemoveItem(0);
    ASSERT_EQ(list.GetItems().size(), 2u);
    EXPECT_EQ(list.GetItems()[0], "Beta Updated");

    list.ClearItems();
    EXPECT_TRUE(list.GetItems().empty());
}

TEST_F(TextListTest, NarrowWidthProducesMoreScrollForWrappedContent)
{
    ImTextList list;
    list.SetStyle(MakeCompactListStyle());
    list.SetItems({
        "This is a deliberately long paragraph that should wrap across several lines when the list becomes narrow enough."
    });

    list.SetGeometry(FGeometry(0.0f, 0.0f, 120.0f, 70.0f));
    PaintList(list);
    const float narrowScrollRange = list.GetMaxScrollOffset();

    list.SetGeometry(FGeometry(0.0f, 0.0f, 320.0f, 70.0f));
    PaintList(list);
    const float wideScrollRange = list.GetMaxScrollOffset();

    EXPECT_GT(narrowScrollRange, wideScrollRange);
}

TEST_F(TextListTest, FullContentWidthIsUsedBeforeReservingScrollbarSpace)
{
    ImTextList list;
    list.SetStyle(MakeCompactListStyle());

    const std::string singleLine =
        "This line should stay on a single row when the viewport is generously wide instead of collapsing into a narrow wrapped column.";
    list.SetItems({singleLine});

    list.SetGeometry(FGeometry(0.0f, 0.0f, 1200.0f, 32.0f));
    PaintList(list);

    EXPECT_FLOAT_EQ(list.GetMaxScrollOffset(), 0.0f);
}

TEST_F(TextListTest, MouseDragCreatesSelectionAndUsesMouseCapture)
{
    auto app = std::make_shared<ImApplication>();
    auto list = std::make_shared<ImTextList>();
    list->SetStyle(MakeCompactListStyle());
    list->SetItems({
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "second line of content for cross-line selection"
    });
    app->SetRootWidget(list);

    AdvanceWithDraw(*app, {}, FVector2(220.0f, 120.0f));

    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))}, FVector2(220.0f, 120.0f));
    EXPECT_EQ(app->GetMouseCapture(), list);

    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseMove, FVector2(180.0f, 52.0f))}, FVector2(220.0f, 120.0f));
    EXPECT_TRUE(list->HasSelection());
    EXPECT_FALSE(list->GetSelectedText().empty());

    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(180.0f, 52.0f))}, FVector2(220.0f, 120.0f));
    EXPECT_EQ(app->GetMouseCapture(), nullptr);
}

TEST_F(TextListTest, CtrlCAndCtrlAOperateOnSelection)
{
    auto app = std::make_shared<ImApplication>();
    auto list = std::make_shared<ImTextList>();
    list->SetStyle(MakeCompactListStyle());
    list->SetItems({
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "second line of content for cross-line selection"
    });
    app->SetRootWidget(list);

    AdvanceWithDraw(*app, {}, FVector2(220.0f, 120.0f));
    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f))}, FVector2(220.0f, 120.0f));
    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseMove, FVector2(180.0f, 52.0f))}, FVector2(220.0f, 120.0f));
    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(180.0f, 52.0f))}, FVector2(220.0f, 120.0f));
    ASSERT_TRUE(list->HasSelection());
    ASSERT_FALSE(list->GetSelectedText().empty());

    app->SetKeyboardFocus(list);
    AdvanceWithDraw(*app, {CreateModifiedKeyEvent(EKey::C, true)}, FVector2(220.0f, 120.0f));
    ImGuiIO& io = ImGui::GetIO();
    ASSERT_NE(io.GetClipboardTextFn, nullptr);
    EXPECT_FALSE(std::string(io.GetClipboardTextFn(io.ClipboardUserData)).empty());

    list->ClearSelection();
    EXPECT_FALSE(list->HasSelection());

    AdvanceWithDraw(*app, {CreateModifiedKeyEvent(EKey::A, true)}, FVector2(220.0f, 120.0f));
    EXPECT_TRUE(list->HasSelection());
    EXPECT_FALSE(list->GetSelectedText().empty());
}

TEST_F(TextListTest, WheelScrollAndScrollbarDragUpdateOffset)
{
    auto app = std::make_shared<ImApplication>();
    auto list = std::make_shared<ImTextList>();
    list->SetStyle(MakeCompactListStyle());
    list->SetItems({
        "0: a long entry that wraps and grows the content height enough to require scrolling through the retained viewport.",
        "1: another long entry that keeps the scroll range alive for testing.",
        "2: more content",
        "3: more content",
        "4: more content",
        "5: more content"
    });
    app->SetRootWidget(list);

    AdvanceWithDraw(*app, {}, FVector2(120.0f, 80.0f));
    AdvanceWithDraw(*app, {CreateWheelEvent(FVector2(40.0f, 40.0f), -2.0f)}, FVector2(120.0f, 80.0f));
    EXPECT_GT(list->GetScrollOffset(), 0.0f);

    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(115.0f, 10.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_EQ(app->GetMouseCapture(), list);

    const float beforeDragOffset = list->GetScrollOffset();
    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseMove, FVector2(115.0f, 60.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_GT(list->GetScrollOffset(), beforeDragOffset);

    AdvanceWithDraw(*app, {CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(115.0f, 60.0f))}, FVector2(120.0f, 80.0f));
    EXPECT_EQ(app->GetMouseCapture(), nullptr);
}

TEST_F(TextListTest, ScrollToItemClampsAndSelectionCanBeCleared)
{
    ImTextList list;
    list.SetStyle(MakeCompactListStyle());
    list.SetItems({
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta"
    });
    list.SetGeometry(FGeometry(0.0f, 0.0f, 120.0f, 60.0f));
    PaintList(list);

    EXPECT_FALSE(list.ScrollToItem(-1));
    EXPECT_FALSE(list.ScrollToItem(99));
    EXPECT_TRUE(list.ScrollToItem(5));
    EXPECT_GE(list.GetScrollOffset(), 0.0f);

    list.SetScrollOffset(1000.0f);
    EXPECT_FLOAT_EQ(list.GetScrollOffset(), list.GetMaxScrollOffset());

    list.ClearSelection();
    EXPECT_FALSE(list.HasSelection());
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
