#include <gtest/gtest.h>

#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/VerticalBox.h>

using namespace ImWidgetV4;

namespace {

class FixedSizeWidget : public ImWidget {
public:
    explicit FixedSizeWidget(const FVector2& minSize)
        : MinSize(minSize)
    {
    }

    FVector2 GetMinSize() const override
    {
        return MinSize;
    }

    FVector2 MinSize;
};

} // namespace

TEST(BoxPanelTest, EmptyHorizontalBoxHasNonZeroDefaultMinSize)
{
    ImHorizontalBox box;
    const FVector2 minSize = box.GetMinSize();

    EXPECT_GT(minSize.X, 0.0f);
    EXPECT_GT(minSize.Y, 0.0f);
}

TEST(BoxPanelTest, EmptyVerticalBoxHasNonZeroDefaultMinSize)
{
    ImVerticalBox box;
    const FVector2 minSize = box.GetMinSize();

    EXPECT_GT(minSize.X, 0.0f);
    EXPECT_GT(minSize.Y, 0.0f);
}

TEST(BoxPanelTest, HorizontalBoxStillComputesChildMinSize)
{
    ImHorizontalBox box;
    box.SetSpacing(5.0f);
    box.AddChild(std::make_shared<FixedSizeWidget>(FVector2(30.0f, 20.0f)));
    box.AddChild(std::make_shared<FixedSizeWidget>(FVector2(40.0f, 25.0f)));

    EXPECT_EQ(box.GetMinSize(), FVector2(75.0f, 25.0f));
}

TEST(BoxPanelTest, VerticalBoxStillComputesChildMinSize)
{
    ImVerticalBox box;
    box.SetSpacing(5.0f);
    box.AddChild(std::make_shared<FixedSizeWidget>(FVector2(30.0f, 20.0f)));
    box.AddChild(std::make_shared<FixedSizeWidget>(FVector2(40.0f, 25.0f)));

    EXPECT_EQ(box.GetMinSize(), FVector2(40.0f, 50.0f));
}
