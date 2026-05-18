#include <gtest/gtest.h>
#include <imwidgetv4/style/StyleSet.h>

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
