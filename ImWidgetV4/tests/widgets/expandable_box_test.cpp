#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <memory>

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

FInputEvent CreateKeyEvent(EInputEventType type, EKey key)
{
    FInputEvent event;
    event.Type = type;
    event.Key = key;
    return event;
}

class ExpandableBoxTest : public ::testing::Test {
protected:
    void SetUp() override
    {
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
        ExpandableBox = std::make_shared<ImExpandableBox>();

        FExpandableBoxStyle style;
        style.HeaderPadding = FMargin(4.0f, 4.0f, 4.0f, 4.0f);
        style.BodyPadding = FMargin(6.0f, 6.0f, 6.0f, 6.0f);
        style.IndicatorSize = 12.0f;
        style.IndicatorSpacing = 4.0f;
        style.MinDesiredSize = FVector2(120.0f, 20.0f);
        ExpandableBox->SetStyle(style);

        HeaderText = std::make_shared<ImTextBlock>();
        HeaderText->SetText("Expandable Header");
        ExpandableBox->SetHeader(HeaderText);

        BodyButton = std::make_shared<ImButton>();
        BodyButton->SetText("Body Action");
        ExpandableBox->SetBody(BodyButton);

        ExpandableBox->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(320.0f, 180.0f)));
        App->SetRootWidget(ExpandableBox);
    }

    void TearDown() override
    {
        BodyButton.reset();
        HeaderText.reset();
        ExpandableBox.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    void AdvanceWithEvents(const std::vector<FInputEvent>& events)
    {
        DrawContext drawContext(ImGui::GetBackgroundDrawList());
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(320.0f, 180.0f);
        frameContext.DrawContext_ = &drawContext;
        App->AdvanceFrame(frameContext);
    }

    FVector2 GetIndicatorClickPoint() const
    {
        const FExpandableBoxStyle& style = ExpandableBox->GetStyle();
        return FVector2(
            ExpandableBox->GetGeometry().Position.X + style.HeaderPadding.Left + style.IndicatorSize * 0.5f,
            ExpandableBox->GetGeometry().Position.Y + 14.0f);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImExpandableBox> ExpandableBox;
    std::shared_ptr<ImTextBlock> HeaderText;
    std::shared_ptr<ImButton> BodyButton;
};

} // namespace

TEST_F(ExpandableBoxTest, ConstructionAndExpandedStateBroadcast)
{
    EXPECT_FALSE(ExpandableBox->IsExpanded());
    EXPECT_FALSE(ExpandableBox->IsHovered());

    int changeCount = 0;
    bool lastExpandedState = false;
    ImExpandableBox* sender = nullptr;
    ExpandableBox->OnExpandedStateChanged.AddLambda([&](ImExpandableBox& box, bool expanded) {
        ++changeCount;
        lastExpandedState = expanded;
        sender = &box;
    });

    ExpandableBox->SetExpanded(true);
    EXPECT_TRUE(ExpandableBox->IsExpanded());
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastExpandedState);
    EXPECT_EQ(sender, ExpandableBox.get());

    ExpandableBox->SetExpanded(true);
    EXPECT_EQ(changeCount, 1);

    ExpandableBox->ToggleExpanded();
    EXPECT_FALSE(ExpandableBox->IsExpanded());
    EXPECT_EQ(changeCount, 2);
    EXPECT_FALSE(lastExpandedState);
}

TEST_F(ExpandableBoxTest, MinSizeIncludesBodyOnlyWhenExpanded)
{
    const FVector2 collapsedMin = ExpandableBox->GetMinSize();
    ExpandableBox->SetExpanded(true);
    const FVector2 expandedMin = ExpandableBox->GetMinSize();

    EXPECT_GT(expandedMin.Y, collapsedMin.Y);
    EXPECT_GE(expandedMin.X, collapsedMin.X);
}

TEST_F(ExpandableBoxTest, IndicatorClickTogglesStateAndRequestsFocus)
{
    AdvanceWithEvents({});

    const FVector2 indicatorPoint = GetIndicatorClickPoint();
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, indicatorPoint)});
    EXPECT_EQ(App->GetMouseCapture(), ExpandableBox);
    EXPECT_EQ(App->GetKeyboardFocus(), ExpandableBox);

    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonUp, indicatorPoint)});
    EXPECT_TRUE(ExpandableBox->IsExpanded());
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(ExpandableBoxTest, HeaderChildClickDoesNotToggleExpandedState)
{
    auto headerButton = std::make_shared<ImButton>();
    headerButton->SetText("Header Action");
    ExpandableBox->SetHeader(headerButton);

    bool headerClicked = false;
    headerButton->OnClicked.AddLambda([&](ImButton&) {
        headerClicked = true;
    });

    AdvanceWithEvents({});
    const FGeometry headerGeometry = headerButton->GetGeometry();
    const FVector2 clickPoint = headerGeometry.GetCenter();

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, clickPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, clickPoint)
    });

    EXPECT_TRUE(headerClicked);
    EXPECT_FALSE(ExpandableBox->IsExpanded());
}

TEST_F(ExpandableBoxTest, BodyParticipatesInHitTestingOnlyWhenExpanded)
{
    bool bodyClicked = false;
    BodyButton->OnClicked.AddLambda([&](ImButton&) {
        bodyClicked = true;
    });

    ExpandableBox->SetExpanded(true);
    AdvanceWithEvents({});
    const FVector2 bodyClickPoint = BodyButton->GetGeometry().GetCenter();

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, bodyClickPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, bodyClickPoint)
    });
    EXPECT_TRUE(bodyClicked);

    bodyClicked = false;
    ExpandableBox->SetExpanded(false);
    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseButtonDown, bodyClickPoint),
        CreateMouseEvent(EInputEventType::MouseButtonUp, bodyClickPoint)
    });
    EXPECT_FALSE(bodyClicked);
}

TEST_F(ExpandableBoxTest, KeyboardToggleRequiresFocus)
{
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_FALSE(ExpandableBox->IsExpanded());

    App->SetKeyboardFocus(ExpandableBox);
    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Enter)});
    EXPECT_TRUE(ExpandableBox->IsExpanded());

    AdvanceWithEvents({CreateKeyEvent(EInputEventType::KeyDown, EKey::Space)});
    EXPECT_FALSE(ExpandableBox->IsExpanded());
}

TEST_F(ExpandableBoxTest, HoverOnlyTracksHeaderRegionChanges)
{
    ExpandableBox->SetExpanded(true);

    int hoverBeginCount = 0;
    int hoverEndCount = 0;
    ExpandableBox->OnHoverBegin.AddLambda([&](ImExpandableBox&) {
        ++hoverBeginCount;
    });
    ExpandableBox->OnHoverEnd.AddLambda([&](ImExpandableBox&) {
        ++hoverEndCount;
    });

    AdvanceWithEvents({});
    const FVector2 headerPoint = HeaderText->GetGeometry().GetCenter();
    const FVector2 bodyPoint = BodyButton->GetGeometry().GetCenter();

    AdvanceWithEvents({
        CreateMouseEvent(EInputEventType::MouseMove, headerPoint),
        CreateMouseEvent(EInputEventType::MouseMove, headerPoint),
        CreateMouseEvent(EInputEventType::MouseMove, bodyPoint),
        CreateMouseEvent(EInputEventType::MouseMove, bodyPoint)
    });

    EXPECT_EQ(hoverBeginCount, 1);
    EXPECT_EQ(hoverEndCount, 1);
    EXPECT_FALSE(ExpandableBox->IsHovered());
}

TEST_F(ExpandableBoxTest, CollapsingClearsBodyFocusAndCapture)
{
    ExpandableBox->SetExpanded(true);
    AdvanceWithEvents({});

    const FVector2 bodyPoint = BodyButton->GetGeometry().GetCenter();
    AdvanceWithEvents({CreateMouseEvent(EInputEventType::MouseButtonDown, bodyPoint)});
    EXPECT_EQ(App->GetKeyboardFocus(), BodyButton);
    EXPECT_EQ(App->GetMouseCapture(), BodyButton);

    ExpandableBox->SetExpanded(false);
    EXPECT_EQ(App->GetKeyboardFocus(), nullptr);
    EXPECT_EQ(App->GetMouseCapture(), nullptr);
}

TEST_F(ExpandableBoxTest, UsesThemeResolvedStyleByDefault)
{
    auto themedBox = std::make_shared<ImExpandableBox>();
    App->SetRootWidget(themedBox);
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FExpandableBoxStyle expectedStyle = ResolveExpandableBoxStyle(App->GetStyleSet());

    const FExpandableBoxStyle& style = themedBox->GetStyle();
    EXPECT_EQ(style.HeaderBackgroundColor.ToImU32(), expectedStyle.HeaderBackgroundColor.ToImU32());
    EXPECT_EQ(style.BorderColor.ToImU32(), expectedStyle.BorderColor.ToImU32());
}

TEST_F(ExpandableBoxTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FExpandableBoxStyle explicitStyle;
    explicitStyle.HeaderBackgroundColor = FColor::FromBytes(10, 20, 30);
    explicitStyle.BorderColor = FColor::FromBytes(200, 210, 220);
    ExpandableBox->SetStyle(explicitStyle);

    EXPECT_EQ(ExpandableBox->GetStyle().HeaderBackgroundColor.ToImU32(), explicitStyle.HeaderBackgroundColor.ToImU32());
    EXPECT_EQ(ExpandableBox->GetStyle().BorderColor.ToImU32(), explicitStyle.BorderColor.ToImU32());
}

TEST_F(ExpandableBoxTest, DeserializedStyleOverridesTheme)
{
    auto deserializedBox = std::make_shared<ImExpandableBox>();
    deserializedBox->FromJson(nlohmann::ordered_json::parse(R"JSON({
        "Type": "ImExpandableBox",
        "Properties": {
            "ImExpandableBox::Expanded": true,
            "ImExpandableBox::Style": {
                "Type": "FExpandableBoxStyle",
                "Properties": {
                    "FExpandableBoxStyle::CornerRadius": 0,
                    "FExpandableBoxStyle::HeaderBackgroundColor": [1, 2, 3, 255]
                }
            }
        }
    })JSON"));

    App->SetRootWidget(deserializedBox);
    ASSERT_TRUE(App->SetActiveTheme("Dark"));

    const FExpandableBoxStyle& style = deserializedBox->GetStyle();
    EXPECT_FLOAT_EQ(style.CornerRadius, 0.0f);
    EXPECT_EQ(style.HeaderBackgroundColor.ToImU32(), FColor::FromBytes(1, 2, 3).ToImU32());
}
