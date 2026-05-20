#include <gtest/gtest.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

using namespace ImWidgetV4;
using namespace ImWidgetV4::Reflection;

class TestBaseObject : public ReflectableObject {
public:
    TestBaseObject()
        : m_baseIntValue(0)
        , m_baseFloatValue(0.0f)
        , m_baseStringValue("")
    {
        StaticTypeDesc();
    }

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
    std::string GetTypeName() const override { return "TestBaseObject"; }

    int m_baseIntValue;
    float m_baseFloatValue;
    std::string m_baseStringValue;
};

class TestDerivedObject : public TestBaseObject {
public:
    TestDerivedObject()
        : m_intValue(0)
        , m_floatValue(0.0f)
        , m_stringValue("")
        , m_boolValue(false)
    {
        StaticTypeDesc();
    }

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
    std::string GetTypeName() const override { return "TestDerivedObject"; }

    int m_intValue;
    float m_floatValue;
    std::string m_stringValue;
    bool m_boolValue;
};

class TestComplexObject : public ReflectableObject {
public:
    TestComplexObject()
        : m_intValue(0)
        , m_floatValue(0.0f)
        , m_boolValue(false)
        , m_stringValue("")
        , m_colorValue(IM_COL32(255, 255, 255, 255))
        , m_vec2Value(0.0f, 0.0f)
        , m_enumValue(0)
    {
        StaticTypeDesc();
    }

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
    std::string GetTypeName() const override { return "TestComplexObject"; }

    int m_intValue;
    float m_floatValue;
    bool m_boolValue;
    std::string m_stringValue;
    ImU32 m_colorValue;
    ImVec2 m_vec2Value;
    std::vector<std::string> m_stringArrayValue;
    int m_enumValue;
};

class TestNestedObject : public ReflectableObject {
public:
    TestNestedObject()
        : m_name("")
        , m_nested(nullptr)
    {
        StaticTypeDesc();
    }

    static const FTypeDesc& StaticTypeDesc();
    const FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }
    std::string GetTypeName() const override { return "TestNestedObject"; }

    std::string m_name;
    TestComplexObject* m_nested;
};

const FTypeDesc& TestBaseObject::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<TestBaseObject, int, &TestBaseObject::m_baseIntValue>(
            "TestBaseObject",
            "baseIntValue",
            EPropertyKind::Int,
            "int",
            "Base integer value"),
        MakeMemberProperty<TestBaseObject, float, &TestBaseObject::m_baseFloatValue>(
            "TestBaseObject",
            "baseFloatValue",
            EPropertyKind::Float,
            "float",
            "Base float value"),
        MakeMemberProperty<TestBaseObject, std::string, &TestBaseObject::m_baseStringValue>(
            "TestBaseObject",
            "baseStringValue",
            EPropertyKind::String,
            "std::string",
            "Base string value")
    };

    static const FTypeDesc typeDesc {
        "TestBaseObject",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const FTypeDesc& TestDerivedObject::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<TestDerivedObject, int, &TestDerivedObject::m_intValue>(
            "TestDerivedObject",
            "intValue",
            EPropertyKind::Int,
            "int",
            "Integer value"),
        MakeMemberProperty<TestDerivedObject, float, &TestDerivedObject::m_floatValue>(
            "TestDerivedObject",
            "floatValue",
            EPropertyKind::Float,
            "float",
            "Float value"),
        MakeMemberProperty<TestDerivedObject, std::string, &TestDerivedObject::m_stringValue>(
            "TestDerivedObject",
            "stringValue",
            EPropertyKind::String,
            "std::string",
            "String value"),
        MakeMemberProperty<TestDerivedObject, bool, &TestDerivedObject::m_boolValue>(
            "TestDerivedObject",
            "boolValue",
            EPropertyKind::Bool,
            "bool",
            "Boolean value")
    };

    static const FTypeDesc typeDesc {
        "TestDerivedObject",
        &TestBaseObject::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const FTypeDesc& TestComplexObject::StaticTypeDesc()
{
    static constexpr const char* enumOptions[] = {"Option1", "Option2", "Option3"};
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<TestComplexObject, int, &TestComplexObject::m_intValue>(
            "TestComplexObject",
            "intValue",
            EPropertyKind::Int,
            "int",
            "Integer value"),
        MakeMemberProperty<TestComplexObject, float, &TestComplexObject::m_floatValue>(
            "TestComplexObject",
            "floatValue",
            EPropertyKind::Float,
            "float",
            "Float value"),
        MakeMemberProperty<TestComplexObject, bool, &TestComplexObject::m_boolValue>(
            "TestComplexObject",
            "boolValue",
            EPropertyKind::Bool,
            "bool",
            "Boolean value"),
        MakeMemberProperty<TestComplexObject, std::string, &TestComplexObject::m_stringValue>(
            "TestComplexObject",
            "stringValue",
            EPropertyKind::String,
            "std::string",
            "String value"),
        MakeMemberProperty<TestComplexObject, ImU32, &TestComplexObject::m_colorValue>(
            "TestComplexObject",
            "colorValue",
            EPropertyKind::Color,
            "ImU32",
            "Color value"),
        MakeMemberProperty<TestComplexObject, ImVec2, &TestComplexObject::m_vec2Value>(
            "TestComplexObject",
            "vec2Value",
            EPropertyKind::Vec2,
            "ImVec2",
            "Vector value"),
        MakeMemberProperty<TestComplexObject, std::vector<std::string>, &TestComplexObject::m_stringArrayValue>(
            "TestComplexObject",
            "stringArrayValue",
            EPropertyKind::StringArray,
            "std::vector<std::string>",
            "String array"),
        MakeMemberProperty<TestComplexObject, int, &TestComplexObject::m_enumValue>(
            "TestComplexObject",
            "enumValue",
            EPropertyKind::Enum,
            "int",
            "Enum value",
            nullptr,
            FEnumOptions {enumOptions, sizeof(enumOptions) / sizeof(enumOptions[0])})
    };

    static const FTypeDesc typeDesc {
        "TestComplexObject",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const FTypeDesc& TestNestedObject::StaticTypeDesc()
{
    static const FPropertyDesc properties[] = {
        MakeMemberProperty<TestNestedObject, std::string, &TestNestedObject::m_name>(
            "TestNestedObject",
            "name",
            EPropertyKind::String,
            "std::string",
            "Name"),
        MakeMemberProperty<TestNestedObject, TestComplexObject*, &TestNestedObject::m_nested>(
            "TestNestedObject",
            "nested",
            EPropertyKind::Struct,
            "TestComplexObject*",
            "Nested object",
            &TestComplexObject::StaticTypeDesc())
    };

    static const FTypeDesc typeDesc {
        "TestNestedObject",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

TEST(ReflectableObjectTest, BasicTypesSerialization)
{
    TestDerivedObject obj;
    obj.m_intValue = 42;
    obj.m_floatValue = 3.14f;
    obj.m_stringValue = "Hello";
    obj.m_boolValue = true;

    json j = obj.ToJson();

    EXPECT_EQ(j["Type"], "TestDerivedObject");
    EXPECT_TRUE(j.contains("Properties"));
    EXPECT_EQ(j["Properties"]["TestDerivedObject::intValue"], 42);
    EXPECT_FLOAT_EQ(j["Properties"]["TestDerivedObject::floatValue"], 3.14f);
    EXPECT_EQ(j["Properties"]["TestDerivedObject::stringValue"], "Hello");
    EXPECT_EQ(j["Properties"]["TestDerivedObject::boolValue"], true);
}

TEST(ReflectableObjectTest, NewReflectionRegistryAdapterFindsMigratedType)
{
    TestBaseObject obj;
    obj.m_baseIntValue = 7;

    const FTypeDesc& typeDesc = obj.GetTypeDesc();
    EXPECT_STREQ(typeDesc.Name, "TestBaseObject");

    const FPropertyDesc* propertyDesc = FindProperty(typeDesc, "baseIntValue", "TestBaseObject");
    ASSERT_NE(propertyDesc, nullptr);

    FPropertyHandle property(&obj, propertyDesc);
    ASSERT_NE(property.GetConstAs<int>(), nullptr);
    EXPECT_EQ(*property.GetConstAs<int>(), 7);
}

TEST(ReflectableObjectTest, GraphicsTypesSerialization)
{
    TestComplexObject obj;
    obj.m_colorValue = IM_COL32(255, 128, 64, 32);
    obj.m_vec2Value = ImVec2(10.5f, 20.5f);

    json j = obj.ToJson();

    EXPECT_TRUE(j["Properties"]["TestComplexObject::colorValue"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"].size(), 4);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][0], 255);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][1], 128);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][2], 64);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][3], 32);

    EXPECT_TRUE(j["Properties"]["TestComplexObject::vec2Value"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::vec2Value"].size(), 2);
    EXPECT_FLOAT_EQ(j["Properties"]["TestComplexObject::vec2Value"][0], 10.5f);
    EXPECT_FLOAT_EQ(j["Properties"]["TestComplexObject::vec2Value"][1], 20.5f);
}

TEST(ReflectableObjectTest, ArrayTypeSerialization)
{
    TestComplexObject obj;
    obj.m_stringArrayValue = {"apple", "banana", "cherry"};

    json j = obj.ToJson();

    EXPECT_TRUE(j["Properties"]["TestComplexObject::stringArrayValue"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"].size(), 3);
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][0], "apple");
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][1], "banana");
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][2], "cherry");
}

TEST(ReflectableObjectTest, EnumTypeSerialization)
{
    TestComplexObject obj;
    obj.m_enumValue = 1;

    json j = obj.ToJson();

    EXPECT_EQ(j["Properties"]["TestComplexObject::enumValue"], "Option2");
}

TEST(ReflectableObjectTest, OptionalEnumAccess)
{
    TestComplexObject obj;
    FReflectedOptionalProperty enumProperty = obj.GetPropertyAsOptional("enumValue");

    ASSERT_TRUE(enumProperty.IsValid());
    EXPECT_EQ(enumProperty.GetOptionCount(), 3u);
    EXPECT_EQ(enumProperty.GetOptionString(), "Option1");
    EXPECT_TRUE(enumProperty.SetOptionByString("Option3"));
    EXPECT_EQ(obj.m_enumValue, 2);
}

TEST(ReflectableObjectTest, InheritanceChainSerialization)
{
    TestDerivedObject obj;
    obj.m_baseIntValue = 100;
    obj.m_baseFloatValue = 1.5f;
    obj.m_baseStringValue = "Base";
    obj.m_intValue = 200;
    obj.m_floatValue = 2.5f;
    obj.m_stringValue = "Derived";

    json j = obj.ToJson();

    EXPECT_EQ(j["Properties"]["TestBaseObject::baseIntValue"], 100);
    EXPECT_FLOAT_EQ(j["Properties"]["TestBaseObject::baseFloatValue"], 1.5f);
    EXPECT_EQ(j["Properties"]["TestBaseObject::baseStringValue"], "Base");
    EXPECT_EQ(j["Properties"]["TestDerivedObject::intValue"], 200);
    EXPECT_FLOAT_EQ(j["Properties"]["TestDerivedObject::floatValue"], 2.5f);
    EXPECT_EQ(j["Properties"]["TestDerivedObject::stringValue"], "Derived");
}

TEST(ReflectableObjectTest, NestedObjectSerialization)
{
    TestNestedObject obj;
    obj.m_name = "Parent";

    TestComplexObject nested;
    nested.m_intValue = 42;
    nested.m_stringValue = "Nested";
    obj.m_nested = &nested;

    json j = obj.ToJson();

    EXPECT_EQ(j["Properties"]["TestNestedObject::name"], "Parent");
    EXPECT_TRUE(j["Properties"]["TestNestedObject::nested"].is_object());
    EXPECT_EQ(j["Properties"]["TestNestedObject::nested"]["Type"], "TestComplexObject");
    EXPECT_EQ(j["Properties"]["TestNestedObject::nested"]["Properties"]["TestComplexObject::intValue"], 42);
}

TEST(ReflectableObjectTest, BasicTypesDeserialization)
{
    json j = {
        {"Type", "TestDerivedObject"},
        {"Properties", {
            {"TestDerivedObject::intValue", 42},
            {"TestDerivedObject::floatValue", 3.14},
            {"TestDerivedObject::stringValue", "Hello"},
            {"TestDerivedObject::boolValue", true}
        }}
    };

    TestDerivedObject obj;
    obj.FromJson(j);

    EXPECT_EQ(obj.m_intValue, 42);
    EXPECT_FLOAT_EQ(obj.m_floatValue, 3.14f);
    EXPECT_EQ(obj.m_stringValue, "Hello");
    EXPECT_EQ(obj.m_boolValue, true);
}

TEST(ReflectableObjectTest, GraphicsTypesDeserialization)
{
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::colorValue", {255, 128, 64, 32}},
            {"TestComplexObject::vec2Value", {10.5, 20.5}}
        }}
    };

    TestComplexObject obj;
    obj.FromJson(j);

    EXPECT_EQ(obj.m_colorValue, IM_COL32(255, 128, 64, 32));
    EXPECT_FLOAT_EQ(obj.m_vec2Value.x, 10.5f);
    EXPECT_FLOAT_EQ(obj.m_vec2Value.y, 20.5f);
}

TEST(ReflectableObjectTest, EnumTypeDeserialization)
{
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::enumValue", "Option2"}
        }}
    };

    TestComplexObject obj;
    obj.FromJson(j);

    EXPECT_EQ(obj.m_enumValue, 1);
}

TEST(ReflectableObjectTest, RoundTripSerialization)
{
    TestComplexObject original;
    original.m_intValue = 42;
    original.m_floatValue = 3.14f;
    original.m_boolValue = true;
    original.m_stringValue = "Test";
    original.m_colorValue = IM_COL32(255, 128, 64, 32);
    original.m_vec2Value = ImVec2(10.5f, 20.5f);
    original.m_stringArrayValue = {"a", "b", "c"};
    original.m_enumValue = 2;

    json j = original.ToJson();

    TestComplexObject restored;
    restored.FromJson(j);

    EXPECT_EQ(restored.m_intValue, original.m_intValue);
    EXPECT_FLOAT_EQ(restored.m_floatValue, original.m_floatValue);
    EXPECT_EQ(restored.m_boolValue, original.m_boolValue);
    EXPECT_EQ(restored.m_stringValue, original.m_stringValue);
    EXPECT_EQ(restored.m_colorValue, original.m_colorValue);
    EXPECT_FLOAT_EQ(restored.m_vec2Value.x, original.m_vec2Value.x);
    EXPECT_FLOAT_EQ(restored.m_vec2Value.y, original.m_vec2Value.y);
    EXPECT_EQ(restored.m_stringArrayValue, original.m_stringArrayValue);
    EXPECT_EQ(restored.m_enumValue, original.m_enumValue);
}

TEST(ReflectableObjectTest, InvalidJsonFormat)
{
    TestDerivedObject obj;

    json j1 = {{"Properties", {}}};
    EXPECT_THROW(obj.FromJson(j1), std::runtime_error);

    json j2 = {{"Type", "TestDerivedObject"}};
    EXPECT_THROW(obj.FromJson(j2), std::runtime_error);

    json j3 = {
        {"Type", "WrongType"},
        {"Properties", {}}
    };
    EXPECT_THROW(obj.FromJson(j3), std::runtime_error);
}

TEST(ReflectableObjectTest, InvalidEnumValue)
{
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::enumValue", "InvalidOption"}
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

TEST(ReflectableObjectTest, InvalidColorFormat)
{
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::colorValue", {255, 128}}
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

TEST(ReflectableObjectTest, InvalidVec2Format)
{
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::vec2Value", {10.5}}
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

TEST(ReflectableObjectTest, ExtraFieldsIgnored)
{
    json j = {
        {"Type", "TestDerivedObject"},
        {"Properties", {
            {"TestDerivedObject::intValue", 42},
            {"TestDerivedObject::unknownField", "ignored"}
        }}
    };

    TestDerivedObject obj;
    EXPECT_NO_THROW(obj.FromJson(j));
    EXPECT_EQ(obj.m_intValue, 42);
}
