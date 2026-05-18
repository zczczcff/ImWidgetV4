#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imgui.h>
#include <memory>

using namespace ImWidgetV4;

class DesignerSurfaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
            io.DisplaySize = ImVec2(1280.0f, 720.0f);
            io.DeltaTime = 1.0f / 60.0f;
        }

        ImGui::NewFrame();

        App = std::make_shared<ImApplication>();
        DesignerSurface = std::make_shared<ImDesignerSurface>();
        App->SetRootWidget(DesignerSurface);
    }

    void TearDown() override
    {
        DesignerSurface.reset();
        App.reset();

        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<ImDesignerSurface> DesignerSurface;
};

TEST_F(DesignerSurfaceTest, UsesThemeResolvedStyleByDefault)
{
    ASSERT_TRUE(App->SetActiveTheme("Dark"));
    const FDesignerSurfaceStyle expectedStyle = ResolveDesignerSurfaceStyle(App->GetStyleSet());

    const FDesignerSurfaceStyle& style = DesignerSurface->GetStyle();
    EXPECT_EQ(style.SelectionBorderColor.ToImU32(), expectedStyle.SelectionBorderColor.ToImU32());
    EXPECT_EQ(style.DropPreviewFillColor.ToImU32(), expectedStyle.DropPreviewFillColor.ToImU32());
    EXPECT_FLOAT_EQ(style.TransformHandleSize, expectedStyle.TransformHandleSize);
    EXPECT_FLOAT_EQ(style.DropPreviewBorderThickness, expectedStyle.DropPreviewBorderThickness);
}

TEST_F(DesignerSurfaceTest, ExplicitStyleOverridesTheme)
{
    ASSERT_TRUE(App->SetActiveTheme("Light"));

    FDesignerSurfaceStyle style;
    style.SelectionBorderColor = FColor::FromBytes(1, 2, 3);
    style.SelectionFillColor = FColor::FromBytes(4, 5, 6, 7);
    style.SelectionBorderThickness = 3.0f;
    style.TransformHandleSize = 9.0f;
    style.TransformHandleColor = FColor::FromBytes(8, 9, 10);
    style.TransformHandleHoveredColor = FColor::FromBytes(11, 12, 13);
    style.TransformHandleActiveColor = FColor::FromBytes(14, 15, 16);
    style.TransformHandleBorderColor = FColor::FromBytes(17, 18, 19);
    style.TransformHandleBorderThickness = 2.0f;
    style.DropPreviewBorderColor = FColor::FromBytes(20, 21, 22);
    style.DropPreviewFillColor = FColor::FromBytes(23, 24, 25, 26);
    style.DropPreviewBorderThickness = 4.0f;
    DesignerSurface->SetStyle(style);

    const FDesignerSurfaceStyle& effectiveStyle = DesignerSurface->GetStyle();
    EXPECT_EQ(effectiveStyle.SelectionBorderColor.ToImU32(), style.SelectionBorderColor.ToImU32());
    EXPECT_EQ(effectiveStyle.DropPreviewBorderColor.ToImU32(), style.DropPreviewBorderColor.ToImU32());
    EXPECT_FLOAT_EQ(effectiveStyle.TransformHandleSize, style.TransformHandleSize);
    EXPECT_FLOAT_EQ(effectiveStyle.DropPreviewBorderThickness, style.DropPreviewBorderThickness);
}

TEST_F(DesignerSurfaceTest, FieldSetterOverridesExplicitStyleField)
{
    FDesignerSurfaceStyle style;
    style.SelectionBorderColor = FColor::FromBytes(10, 20, 30);
    style.SelectionFillColor = FColor::FromBytes(40, 50, 60, 70);
    DesignerSurface->SetStyle(style);

    const FColor overrideColor = FColor::FromBytes(90, 100, 110);
    DesignerSurface->SetSelectionBorderColor(overrideColor);

    EXPECT_EQ(DesignerSurface->GetSelectionBorderColor().ToImU32(), overrideColor.ToImU32());
    EXPECT_EQ(DesignerSurface->GetSelectionFillColor().ToImU32(), style.SelectionFillColor.ToImU32());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
