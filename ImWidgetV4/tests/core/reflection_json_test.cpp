#include <gtest/gtest.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionJson.h>
#include <string>
#include <vector>

using namespace ImWidgetV4::Reflection;

namespace {

enum class EJsonMode : int {
    Basic,
    Advanced
};

struct FJsonNested final : public IReflectable {
    int Count = 0;
    std::string Label;

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
};

struct FJsonObject final : public IReflectable {
    std::string Name;
    bool bEnabled = false;
    float Opacity = 0.0f;
    std::vector<std::string> Tags;
    int Mode = static_cast<int>(EJsonMode::Basic);
    FJsonNested Nested;

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
};

const FTypeDesc& FJsonNested::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<FJsonNested, int, &FJsonNested::Count>(
            "FJsonNested",
            "Count",
            EPropertyKind::Int,
            "int"),
        MakeMemberProperty<FJsonNested, std::string, &FJsonNested::Label>(
            "FJsonNested",
            "Label",
            EPropertyKind::String,
            "std::string")
    };

    static const FTypeDesc typeDesc {
        "FJsonNested",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    return typeDesc;
}

const FTypeDesc& FJsonObject::StaticTypeDesc()
{
    static constexpr const char* modeOptions[] = {"Basic", "Advanced"};

    static const FPropertyDesc properties[] = {
        MakeMemberProperty<FJsonObject, std::string, &FJsonObject::Name>(
            "FJsonObject",
            "Name",
            EPropertyKind::String,
            "std::string"),
        MakeMemberProperty<FJsonObject, bool, &FJsonObject::bEnabled>(
            "FJsonObject",
            "Enabled",
            EPropertyKind::Bool,
            "bool"),
        MakeMemberProperty<FJsonObject, float, &FJsonObject::Opacity>(
            "FJsonObject",
            "Opacity",
            EPropertyKind::Float,
            "float"),
        MakeMemberProperty<FJsonObject, std::vector<std::string>, &FJsonObject::Tags>(
            "FJsonObject",
            "Tags",
            EPropertyKind::StringArray,
            "std::vector<std::string>"),
        MakeMemberProperty<FJsonObject, int, &FJsonObject::Mode>(
            "FJsonObject",
            "Mode",
            EPropertyKind::Enum,
            "EJsonMode",
            "",
            nullptr,
            {modeOptions, sizeof(modeOptions) / sizeof(modeOptions[0])}),
        MakeMemberProperty<FJsonObject, FJsonNested, &FJsonObject::Nested>(
            "FJsonObject",
            "Nested",
            EPropertyKind::Struct,
            "FJsonNested",
            "",
            &FJsonNested::StaticTypeDesc())
    };

    static const FTypeDesc typeDesc {
        "FJsonObject",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    return typeDesc;
}

} // namespace

TEST(ReflectionJsonTest, SerializesReflectedValues)
{
    FJsonObject object;
    object.Name = "Panel";
    object.bEnabled = true;
    object.Opacity = 0.75f;
    object.Tags = {"layout", "debug"};
    object.Mode = static_cast<int>(EJsonMode::Advanced);
    object.Nested.Count = 3;
    object.Nested.Label = "Child";

    const FReflectionJson json = ToJson(object);

    EXPECT_EQ(json["Type"], "FJsonObject");
    EXPECT_EQ(json["Properties"]["FJsonObject::Name"], "Panel");
    EXPECT_EQ(json["Properties"]["FJsonObject::Enabled"], true);
    EXPECT_FLOAT_EQ(json["Properties"]["FJsonObject::Opacity"].get<float>(), 0.75f);
    EXPECT_EQ(json["Properties"]["FJsonObject::Tags"][0], "layout");
    EXPECT_EQ(json["Properties"]["FJsonObject::Mode"], "Advanced");
    EXPECT_EQ(json["Properties"]["FJsonObject::Nested"]["Type"], "FJsonNested");
    EXPECT_EQ(json["Properties"]["FJsonObject::Nested"]["Properties"]["FJsonNested::Count"], 3);
    EXPECT_EQ(json["Properties"]["FJsonObject::Nested"]["Properties"]["FJsonNested::Label"], "Child");
}

TEST(ReflectionJsonTest, DeserializesReflectedValues)
{
    FJsonObject object;
    FReflectionJson json;
    json["Type"] = "FJsonObject";
    json["Properties"] = FReflectionJson::object();
    json["Properties"]["FJsonObject::Name"] = "Editor";
    json["Properties"]["FJsonObject::Enabled"] = true;
    json["Properties"]["FJsonObject::Opacity"] = 0.5f;
    json["Properties"]["FJsonObject::Tags"] = {"details", "runtime"};
    json["Properties"]["FJsonObject::Mode"] = "Advanced";
    json["Properties"]["FJsonObject::Nested"]["Type"] = "FJsonNested";
    json["Properties"]["FJsonObject::Nested"]["Properties"] = FReflectionJson::object();
    json["Properties"]["FJsonObject::Nested"]["Properties"]["FJsonNested::Count"] = 9;
    json["Properties"]["FJsonObject::Nested"]["Properties"]["FJsonNested::Label"] = "Nested";

    std::string error;
    ASSERT_TRUE(FromJson(object, json, &error)) << error;

    EXPECT_EQ(object.Name, "Editor");
    EXPECT_TRUE(object.bEnabled);
    EXPECT_FLOAT_EQ(object.Opacity, 0.5f);
    ASSERT_EQ(object.Tags.size(), 2);
    EXPECT_EQ(object.Tags[0], "details");
    EXPECT_EQ(object.Tags[1], "runtime");
    EXPECT_EQ(object.Mode, static_cast<int>(EJsonMode::Advanced));
    EXPECT_EQ(object.Nested.Count, 9);
    EXPECT_EQ(object.Nested.Label, "Nested");
}

TEST(ReflectionJsonTest, RejectsMismatchedType)
{
    FJsonObject object;
    FReflectionJson json;
    json["Type"] = "OtherType";
    json["Properties"] = FReflectionJson::object();

    std::string error;
    EXPECT_FALSE(FromJson(object, json, &error));
    EXPECT_NE(error.find("Type mismatch"), std::string::npos);
}
