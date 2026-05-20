#include <gtest/gtest.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/reflection/Reflectable.h>
#include <string>

using namespace ImWidgetV4::Reflection;

namespace {

struct FLightBase : public IReflectable {
    std::string Name;
    bool bVisible = true;

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
};

struct FLightButton : public FLightBase {
    int ClickCount = 0;
    float Opacity = 1.0f;

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
};

const FTypeDesc& FLightBase::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<FLightBase, std::string, &FLightBase::Name>(
            "FLightBase",
            "Name",
            EPropertyKind::String,
            "std::string",
            "Display name"),
        MakeMemberProperty<FLightBase, bool, &FLightBase::bVisible>(
            "FLightBase",
            "Visible",
            EPropertyKind::Bool,
            "bool",
            "Visibility")
    };

    static const FTypeDesc typeDesc {
        "FLightBase",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const FTypeDesc& FLightButton::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<FLightButton, int, &FLightButton::ClickCount>(
            "FLightButton",
            "ClickCount",
            EPropertyKind::Int,
            "int",
            "Click count"),
        MakeMemberProperty<FLightButton, float, &FLightButton::Opacity>(
            "FLightButton",
            "Opacity",
            EPropertyKind::Float,
            "float",
            "Opacity")
    };

    static const FTypeDesc typeDesc {
        "FLightButton",
        &FLightBase::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

} // namespace

TEST(LightReflectionTest, CollectsOwnThenParentProperties)
{
    const auto properties = CollectProperties(FLightButton::StaticTypeDesc());

    ASSERT_EQ(properties.size(), 4);
    EXPECT_STREQ(properties[0]->Name, "ClickCount");
    EXPECT_STREQ(properties[1]->Name, "Opacity");
    EXPECT_STREQ(properties[2]->Name, "Name");
    EXPECT_STREQ(properties[3]->Name, "Visible");
}

TEST(LightReflectionTest, ReadsAndWritesMemberProperties)
{
    FLightButton button;
    button.Name = "Apply";
    button.ClickCount = 2;

    const FPropertyDesc* nameProperty = FindProperty(button.GetTypeDesc(), "Name", "FLightBase");
    const FPropertyDesc* clickProperty = FindProperty(button.GetTypeDesc(), "ClickCount", "FLightButton");
    ASSERT_NE(nameProperty, nullptr);
    ASSERT_NE(clickProperty, nullptr);

    FPropertyHandle nameHandle(&button, nameProperty);
    FPropertyHandle clickHandle(&button, clickProperty);

    ASSERT_NE(nameHandle.GetConstAs<std::string>(), nullptr);
    EXPECT_EQ(*nameHandle.GetConstAs<std::string>(), "Apply");
    EXPECT_EQ(*clickHandle.GetConstAs<int>(), 2);

    const int newClickCount = 5;
    EXPECT_TRUE(clickHandle.CopyFrom(&newClickCount));
    EXPECT_EQ(button.ClickCount, 5);
}

TEST(LightReflectionTest, RegistersTypesByName)
{
    FLightButton::StaticTypeDesc();

    const FTypeDesc* found = FReflectionRegistry::Get().FindType("FLightButton");
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->Name, "FLightButton");
}
