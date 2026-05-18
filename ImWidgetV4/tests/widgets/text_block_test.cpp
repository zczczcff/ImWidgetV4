#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class TextBlockTest : public ::testing::Test {
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
        TextBlock = std::make_shared<ImTextBlock>();
        App->SetRootWidget(TextBlock);
    }

    void TearDown() override
    {
        TextBlock.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImTextBlock> TextBlock;
};

TEST_F(TextBlockTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FTextBlockStyle expectedStyle = ResolveTextBlockStyle(App->GetStyleSet());

    const FTextBlockStyle& style = TextBlock->GetStyle();
    EXPECT_EQ(style.TextColor.ToImU32(), expectedStyle.TextColor.ToImU32());
    EXPECT_FLOAT_EQ(style.FontSize, expectedStyle.FontSize);
}

TEST_F(TextBlockTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FTextBlockStyle style;
    style.TextColor = FColor::FromBytes(12, 34, 56);
    style.FontSize = 23.0f;
    TextBlock->SetStyle(style);

    EXPECT_EQ(TextBlock->GetTextColor().ToImU32(), style.TextColor.ToImU32());
    EXPECT_FLOAT_EQ(TextBlock->GetFontSize(), style.FontSize);
}

TEST_F(TextBlockTest, PropertySetterKeepsThemeValuesForUnmodifiedFields)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FTextBlockStyle themeStyle = ResolveTextBlockStyle(App->GetStyleSet());

    const FColor explicitColor = FColor::FromBytes(200, 150, 100);
    TextBlock->SetTextColor(explicitColor);

    EXPECT_EQ(TextBlock->GetTextColor().ToImU32(), explicitColor.ToImU32());
    EXPECT_FLOAT_EQ(TextBlock->GetFontSize(), themeStyle.FontSize);
}
