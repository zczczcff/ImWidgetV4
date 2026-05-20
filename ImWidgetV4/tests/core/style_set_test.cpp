#include <gtest/gtest.h>
#include <imwidgetv4/style/StyleSet.h>
#include <nlohmann/json.hpp>
#include <string>

using namespace ImWidgetV4;

TEST(StyleSetTest, StoresBoolAndMarginTokens)
{
    FStyleSet styleSet;

    EXPECT_FALSE(styleSet.HasBool("Bool.Test.Enabled"));
    EXPECT_FALSE(styleSet.GetBool("Bool.Test.Enabled"));
    EXPECT_TRUE(styleSet.GetBool("Bool.Test.Enabled", true));

    styleSet.SetBool("Bool.Test.Enabled", true);
    EXPECT_TRUE(styleSet.HasBool("Bool.Test.Enabled"));
    EXPECT_TRUE(styleSet.GetBool("Bool.Test.Enabled"));

    const FMargin margin(1.0f, 2.0f, 3.0f, 4.0f);
    styleSet.SetMargin("Margin.Test.Padding", margin);
    EXPECT_TRUE(styleSet.HasMargin("Margin.Test.Padding"));

    const FMargin resolved = styleSet.GetMargin("Margin.Test.Padding");
    EXPECT_FLOAT_EQ(resolved.Left, 1.0f);
    EXPECT_FLOAT_EQ(resolved.Right, 2.0f);
    EXPECT_FLOAT_EQ(resolved.Top, 3.0f);
    EXPECT_FLOAT_EQ(resolved.Bottom, 4.0f);

    const FMargin fallback(8.0f);
    const FMargin missing = styleSet.GetMargin("Margin.Test.Missing", fallback);
    EXPECT_FLOAT_EQ(missing.Left, 8.0f);
    EXPECT_FLOAT_EQ(missing.Right, 8.0f);
    EXPECT_FLOAT_EQ(missing.Top, 8.0f);
    EXPECT_FLOAT_EQ(missing.Bottom, 8.0f);
}

TEST(StyleSetTest, MergeAndClearHandleBoolAndMarginTokens)
{
    FStyleSet base;
    base.SetBool("Bool.Feature.Visible", false);
    base.SetMargin("Margin.Panel.Padding", FMargin(2.0f, 3.0f, 4.0f, 5.0f));

    FStyleSet override;
    override.SetBool("Bool.Feature.Visible", true);
    override.SetMargin("Margin.Panel.Padding", FMargin(6.0f));

    base.Merge(override);
    EXPECT_TRUE(base.GetBool("Bool.Feature.Visible"));

    const FMargin mergedMargin = base.GetMargin("Margin.Panel.Padding");
    EXPECT_FLOAT_EQ(mergedMargin.Left, 6.0f);
    EXPECT_FLOAT_EQ(mergedMargin.Right, 6.0f);
    EXPECT_FLOAT_EQ(mergedMargin.Top, 6.0f);
    EXPECT_FLOAT_EQ(mergedMargin.Bottom, 6.0f);

    ASSERT_EQ(base.GetBoolKeys().size(), 1u);
    ASSERT_EQ(base.GetMarginKeys().size(), 1u);

    base.Clear();
    EXPECT_FALSE(base.HasBool("Bool.Feature.Visible"));
    EXPECT_FALSE(base.HasMargin("Margin.Panel.Padding"));
    EXPECT_TRUE(base.GetBoolKeys().empty());
    EXPECT_TRUE(base.GetMarginKeys().empty());
}

TEST(StyleSetTest, SerializesAndRestoresThemePackJson)
{
    FThemePack themePack("Editor Blue Green");
    themePack.StyleSet.SetColor("Color.Editor.Accent", FColor::FromBytes(0, 132, 126));
    themePack.StyleSet.SetVector2("Vector2.Editor.MinSize", FVector2(12.0f, 34.0f));
    themePack.StyleSet.SetMargin("Margin.Editor.Padding", FMargin(1.0f, 2.0f, 3.0f, 4.0f));

    const nlohmann::ordered_json json = FStyleSetFactory::ThemePackToJson(themePack);
    std::string error;
    const FThemePack restored = FStyleSetFactory::CreateThemePackFromJson(json, &error);

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(restored.Name, themePack.Name);
    const FColor restoredColor = restored.StyleSet.GetColor("Color.Editor.Accent");
    const FColor expectedColor = themePack.StyleSet.GetColor("Color.Editor.Accent");
    EXPECT_FLOAT_EQ(restoredColor.R, expectedColor.R);
    EXPECT_FLOAT_EQ(restoredColor.G, expectedColor.G);
    EXPECT_FLOAT_EQ(restoredColor.B, expectedColor.B);
    EXPECT_FLOAT_EQ(restoredColor.A, expectedColor.A);

    const FVector2 restoredSize = restored.StyleSet.GetVector2("Vector2.Editor.MinSize");
    const FVector2 expectedSize = themePack.StyleSet.GetVector2("Vector2.Editor.MinSize");
    EXPECT_FLOAT_EQ(restoredSize.X, expectedSize.X);
    EXPECT_FLOAT_EQ(restoredSize.Y, expectedSize.Y);

    const FMargin restoredMargin = restored.StyleSet.GetMargin("Margin.Editor.Padding");
    const FMargin expectedMargin = themePack.StyleSet.GetMargin("Margin.Editor.Padding");
    EXPECT_FLOAT_EQ(restoredMargin.Left, expectedMargin.Left);
    EXPECT_FLOAT_EQ(restoredMargin.Right, expectedMargin.Right);
    EXPECT_FLOAT_EQ(restoredMargin.Top, expectedMargin.Top);
    EXPECT_FLOAT_EQ(restoredMargin.Bottom, expectedMargin.Bottom);
}

TEST(StyleSetTest, CreatesThemePackFromJsonStringWithBaseTheme)
{
    const std::string jsonText = R"json(
{
  "Name": "Custom Data Theme",
  "BaseTheme": "Light",
  "StyleSet": {
    "Colors": {
      "Color.Editor.Accent": {"Bytes": [0, 128, 64, 255]}
    },
    "Floats": {
      "Float.Button.CornerRadius": 3.0
    }
  }
}
)json";

    std::string error;
    const FThemePack themePack = FStyleSetFactory::CreateThemePackFromJsonString(jsonText, &error);

    EXPECT_TRUE(error.empty()) << error;
    EXPECT_EQ(themePack.Name, "Custom Data Theme");
    EXPECT_TRUE(themePack.StyleSet.HasColor("Color.Button.Normal.Background"));
    EXPECT_FLOAT_EQ(themePack.StyleSet.GetFloat("Float.Button.CornerRadius"), 3.0f);

    const FColor accent = themePack.StyleSet.GetColor("Color.Editor.Accent");
    EXPECT_FLOAT_EQ(accent.R, 0.0f);
    EXPECT_FLOAT_EQ(accent.G, 128.0f / 255.0f);
    EXPECT_FLOAT_EQ(accent.B, 64.0f / 255.0f);
    EXPECT_FLOAT_EQ(accent.A, 1.0f);
}

TEST(StyleSetTest, RejectsThemePackJsonStringWithUnknownBaseTheme)
{
    const std::string jsonText = R"json(
{
  "Name": "Broken Theme",
  "BaseTheme": "Missing",
  "StyleSet": {}
}
)json";

    std::string error;
    const FThemePack themePack = FStyleSetFactory::CreateThemePackFromJsonString(jsonText, &error);

    EXPECT_TRUE(themePack.Name.empty());
    EXPECT_NE(error.find("Unknown BaseTheme"), std::string::npos);
}
