#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <memory>

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
        Child = child;
        if (Child) {
            AddChild(Child);
            Child->SetGeometry(ChildGeometry);
        }
    }

    void SetChildGeometry(const FGeometry& geometry)
    {
        ChildGeometry = geometry;
        if (Child) {
            Child->SetGeometry(geometry);
        }
    }

    void Paint(const FPaintContext& paintContext) override
    {
        if (Child) {
            Child->SetGeometry(ChildGeometry);
            Child->Paint(paintContext);
        }
    }

private:
    std::shared_ptr<ImWidget> Child;
    FGeometry ChildGeometry;
};

} // namespace

class ComboBoxTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
            io.DisplaySize = ImVec2(1920.0f, 1080.0f);
            io.DeltaTime = 1.0f / 60.0f;
        }

        ImGui::NewFrame();

        App = std::make_shared<ImApplication>();
        ComboBox = std::make_shared<ImComboBox>();
        Host = std::make_shared<FixedHostWidget>();
        Host->SetChildGeometry(FGeometry(FVector2(20.0f, 20.0f), FVector2(180.0f, 38.0f)));
        Host->SetChild(ComboBox);
        ComboBox->SetItems({"Alpha", "Beta", "Gamma"});
        App->SetRootWidget(Host);
    }

    void TearDown() override {
        Host.reset();
        ComboBox.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    FInputEvent CreateMouseEvent(
        EInputEventType type,
        const FVector2& position,
        EMouseButton button = EMouseButton::Left) {
        FInputEvent event;
        event.Type = type;
        event.MousePosition = position;
        event.MouseButton = button;
        return event;
    }

    FInputEvent CreateKeyEvent(EInputEventType type, EKey key) {
        FInputEvent event;
        event.Type = type;
        event.Key = key;
        return event;
    }

    FInputEvent CreateWheelEvent(float x, float y, float deltaY) {
        FInputEvent event;
        event.Type = EInputEventType::MouseWheel;
        event.MousePosition = FVector2(x, y);
        event.ScrollDelta = FVector2(0.0f, deltaY);
        return event;
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events) {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(320.0f, 220.0f);
        App->AdvanceFrame(frameContext);
    }

    FGeometry GetPopupContentGeometry() const {
        for (const auto& window : App->GetWindowManager().GetOpenWindows()) {
            if (window && window->GetKind() == EWindowKind::Popup) {
                return window->GetContentGeometry();
            }
        }

        return FGeometry();
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<FixedHostWidget> Host;
    std::shared_ptr<ImComboBox> ComboBox;
};

TEST_F(ComboBoxTest, ConstructionAndSelectionProperties) {
    EXPECT_TRUE(ComboBox->SupportsKeyboardFocus());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), -1);
    EXPECT_FALSE(ComboBox->HasSelection());
    EXPECT_EQ(ComboBox->GetSelectedText(), "");

    ComboBox->SetSelectedIndex(1);
    EXPECT_TRUE(ComboBox->HasSelection());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), 1);
    EXPECT_EQ(ComboBox->GetSelectedText(), "Beta");

    ComboBox->ClearSelection();
    EXPECT_FALSE(ComboBox->HasSelection());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), -1);
}

TEST_F(ComboBoxTest, ClickOpensPopupAndSelectingItemCommitsValue) {
    int selectedIndex = -1;
    ImComboBox* sender = nullptr;
    ComboBox->OnSelectionChanged.AddLambda([&](ImComboBox& comboBox, int index) {
        sender = &comboBox;
        selectedIndex = index;
    });

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f))
    });
    EXPECT_TRUE(ComboBox->IsPopupOpen());
    EXPECT_EQ(App->GetKeyboardFocus(), ComboBox);
    const FGeometry popupGeometry = GetPopupContentGeometry();
    ASSERT_GT(popupGeometry.Size.Y, 0.0f);

    AdvanceWithEvents({
        CreateMouseEvent(
            EInputEventType::MouseButtonDown,
            FVector2(popupGeometry.Position.X + 20.0f, popupGeometry.Position.Y + 45.0f)),
        CreateMouseEvent(
            EInputEventType::MouseButtonUp,
            FVector2(popupGeometry.Position.X + 20.0f, popupGeometry.Position.Y + 45.0f))
    });

    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), 1);
    EXPECT_EQ(ComboBox->GetSelectedText(), "Beta");
    EXPECT_EQ(selectedIndex, 1);
    EXPECT_EQ(sender, ComboBox.get());
}

TEST_F(ComboBoxTest, KeyboardNavigationOpensPopupAndCommitsHighlightedItem) {
    App->SetKeyboardFocus(ComboBox);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_TRUE(ComboBox->IsPopupOpen());

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Down)});
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});

    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), 1);
    EXPECT_EQ(ComboBox->GetSelectedText(), "Beta");
}

TEST_F(ComboBoxTest, EscapeAndOutsideClickClosePopupWithoutChangingSelection) {
    ComboBox->SetSelectedIndex(0);
    App->SetKeyboardFocus(ComboBox);

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_TRUE(ComboBox->IsPopupOpen());

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Escape)});
    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), 0);

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f))
    });
    EXPECT_TRUE(ComboBox->IsPopupOpen());

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(260.0f, 180.0f))});
    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), 0);
}

TEST_F(ComboBoxTest, DisabledComboBoxIgnoresInput) {
    ComboBox->SetDisabled(true);

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f)),
        CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)
    });

    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_EQ(ComboBox->GetSelectedIndex(), -1);
}

TEST_F(ComboBoxTest, MouseWheelCanScrollPopupListAndCommitLaterItem) {
    ComboBox->SetItems({"A", "B", "C", "D", "E", "F", "G", "H"});
    ComboBox->SetMaxVisibleItems(4);

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f))
    });
    EXPECT_TRUE(ComboBox->IsPopupOpen());
    const FGeometry popupGeometry = GetPopupContentGeometry();
    ASSERT_GT(popupGeometry.Size.Y, 0.0f);

    AdvanceWithEvents({CreateWheelEvent(60.0f, 90.0f, -3.0f)});
    AdvanceWithEvents({
        CreateMouseEvent(
            EInputEventType::MouseButtonDown,
            FVector2(
                popupGeometry.Position.X + 20.0f,
                popupGeometry.Position.Y + popupGeometry.Size.Y - 15.0f)),
        CreateMouseEvent(
            EInputEventType::MouseButtonUp,
            FVector2(
                popupGeometry.Position.X + 20.0f,
                popupGeometry.Position.Y + popupGeometry.Size.Y - 15.0f))
    });

    EXPECT_FALSE(ComboBox->IsPopupOpen());
    EXPECT_GE(ComboBox->GetSelectedIndex(), 3);
}

TEST_F(ComboBoxTest, SelectionCallbackCanReplaceOwningWidgetTreeSafely) {
    std::weak_ptr<ImComboBox> oldCombo = ComboBox;
    int callbackCount = 0;

    ComboBox->OnSelectionChanged.AddLambda([&](ImComboBox&, int) {
        ++callbackCount;

        auto replacement = std::make_shared<ImTextBlock>();
        replacement->SetText("Rebuilt");
        Host->SetChild(replacement);
        ComboBox.reset();
    });

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f))
    });
    const FGeometry popupGeometry = GetPopupContentGeometry();
    ASSERT_GT(popupGeometry.Size.Y, 0.0f);

    AdvanceWithEvents({
        CreateMouseEvent(
            EInputEventType::MouseButtonDown,
            FVector2(popupGeometry.Position.X + 20.0f, popupGeometry.Position.Y + 45.0f)),
        CreateMouseEvent(
            EInputEventType::MouseButtonUp,
            FVector2(popupGeometry.Position.X + 20.0f, popupGeometry.Position.Y + 45.0f))
    });

    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(oldCombo.expired());
    EXPECT_TRUE(App->GetWindowManager().GetOpenWindows().size() >= 1);
}

TEST_F(ComboBoxTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));
    const FComboBoxStyle expectedStyle = ResolveComboBoxStyle(App->GetStyleSet());

    const FComboBoxStyle& style = ComboBox->GetStyle();
    EXPECT_EQ(style.BackgroundColor.ToImU32(), expectedStyle.BackgroundColor.ToImU32());
    EXPECT_EQ(style.PopupOutlineColor.ToImU32(), expectedStyle.PopupOutlineColor.ToImU32());
}

TEST_F(ComboBoxTest, ExplicitStyleOverridesThemeAndPopupWindowStyle)
{
    FComboBoxStyle explicitStyle;
    explicitStyle.BackgroundColor = FColor::FromBytes(14, 24, 34);
    explicitStyle.PopupOutlineColor = FColor::FromBytes(90, 100, 110);
    ComboBox->SetStyle(explicitStyle);

    EXPECT_EQ(ComboBox->GetStyle().BackgroundColor.ToImU32(), explicitStyle.BackgroundColor.ToImU32());

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(60.0f, 40.0f)),
        CreateMouseEvent(EInputEventType::MouseButtonUp, FVector2(60.0f, 40.0f))
    });
    ASSERT_TRUE(ComboBox->IsPopupOpen());

    for (const auto& window : App->GetWindowManager().GetOpenWindows()) {
        if (window && window->GetKind() == EWindowKind::Popup) {
            EXPECT_EQ(window->GetStyle().BackgroundColor.ToImU32(), explicitStyle.BackgroundColor.ToImU32());
            EXPECT_EQ(window->GetStyle().BorderColor.ToImU32(), explicitStyle.PopupOutlineColor.ToImU32());
            break;
        }
    }

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(260.0f, 180.0f))});
    EXPECT_FALSE(ComboBox->IsPopupOpen());
}

TEST_F(ComboBoxTest, DeserializedStyleOverridesTheme)
{
    auto comboBox = std::make_shared<ImComboBox>();
    comboBox->FromJson(nlohmann::ordered_json::parse(R"JSON({
        "Type": "ImComboBox",
        "Properties": {
            "ImComboBox::Items": ["One", "Two"],
            "ImComboBox::SelectedIndex": 0,
            "ImComboBox::PlaceholderText": "Pick",
            "ImComboBox::MaxVisibleItems": 4,
            "ImComboBox::Disabled": false,
            "ImComboBox::Style": {
                "Type": "FComboBoxStyle",
                "Properties": {
                    "FComboBoxStyle::BackgroundColor": [1, 2, 3, 255],
                    "FComboBoxStyle::PopupOutlineColor": [4, 5, 6, 255],
                    "FComboBoxStyle::CornerRadius": 0
                }
            }
        }
    })JSON"));
    Host->SetChild(comboBox);
    ComboBox = comboBox;
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    const FComboBoxStyle& style = ComboBox->GetStyle();
    EXPECT_EQ(style.BackgroundColor.ToImU32(), FColor::FromBytes(1, 2, 3).ToImU32());
    EXPECT_EQ(style.PopupOutlineColor.ToImU32(), FColor::FromBytes(4, 5, 6).ToImU32());
    EXPECT_FLOAT_EQ(style.CornerRadius, 0.0f);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
