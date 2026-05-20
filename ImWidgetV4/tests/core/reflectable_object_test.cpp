#include <gtest/gtest.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/PropertyType.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imgui.h>

using namespace ImWidgetV4;
using namespace ImWidgetV4::Reflection;

// 测试基类：包含基本属性
class TestBaseObject : public ReflectableObject {
    DECLARE_OBJECT(TestBaseObject)
    registrar
        .RegisterProperty(PropertyType::Int, "baseIntValue", &TestBaseObject::m_baseIntValue, "基类整数值")
        .RegisterProperty(PropertyType::Float, "baseFloatValue", &TestBaseObject::m_baseFloatValue, "基类浮点值")
        .RegisterProperty(PropertyType::String, "baseStringValue", &TestBaseObject::m_baseStringValue, "基类字符串值");
    END_DECLARE_OBJECT()

public:
    TestBaseObject() : m_baseIntValue(0), m_baseFloatValue(0.0f), m_baseStringValue("") {}

    std::string GetTypeName() const override { return "TestBaseObject"; }

    int m_baseIntValue;
    float m_baseFloatValue;
    std::string m_baseStringValue;
};

namespace {

const FTypeDesc& GetTestBaseObjectTypeDesc()
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

} // namespace

// 测试派生类：包含更多属性
class TestDerivedObject : public TestBaseObject {
    DECLARE_OBJECT_WITH_PARENT(TestDerivedObject, TestBaseObject)
    registrar
        .RegisterProperty(PropertyType::Int, "intValue", &TestDerivedObject::m_intValue, "整数值")
        .RegisterProperty(PropertyType::Float, "floatValue", &TestDerivedObject::m_floatValue, "浮点值")
        .RegisterProperty(PropertyType::String, "stringValue", &TestDerivedObject::m_stringValue, "字符串值")
        .RegisterProperty(PropertyType::Bool, "boolValue", &TestDerivedObject::m_boolValue, "布尔值");
    END_DECLARE_OBJECT()

public:
    TestDerivedObject()
        : m_intValue(0), m_floatValue(0.0f), m_stringValue(""), m_boolValue(false) {}

    std::string GetTypeName() const override { return "TestDerivedObject"; }

    int m_intValue;
    float m_floatValue;
    std::string m_stringValue;
    bool m_boolValue;
};

// 测试复杂对象：包含所有类型的属性
class TestComplexObject : public ReflectableObject {
    DECLARE_OBJECT(TestComplexObject)
    registrar
        .RegisterProperty(PropertyType::Int, "intValue", &TestComplexObject::m_intValue, "整数值")
        .RegisterProperty(PropertyType::Float, "floatValue", &TestComplexObject::m_floatValue, "浮点值")
        .RegisterProperty(PropertyType::Bool, "boolValue", &TestComplexObject::m_boolValue, "布尔值")
        .RegisterProperty(PropertyType::String, "stringValue", &TestComplexObject::m_stringValue, "字符串值")
        .RegisterProperty(PropertyType::Color, "colorValue", &TestComplexObject::m_colorValue, "颜色值")
        .RegisterProperty(PropertyType::Vec2, "vec2Value", &TestComplexObject::m_vec2Value, "二维向量值")
        .RegisterProperty(PropertyType::StringArray, "stringArrayValue", &TestComplexObject::m_stringArrayValue, "字符串数组")
        .RegisterOptionalProperty(PropertyType::Enum, "enumValue", &TestComplexObject::m_enumValue,
            {"Option1", "Option2", "Option3"}, "枚举值");
    END_DECLARE_OBJECT()

public:
    TestComplexObject()
        : m_intValue(0), m_floatValue(0.0f), m_boolValue(false), m_stringValue(""),
          m_colorValue(IM_COL32(255, 255, 255, 255)), m_vec2Value(0.0f, 0.0f), m_enumValue(0) {}

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

// 测试嵌套对象
class TestNestedObject : public ReflectableObject {
    DECLARE_OBJECT(TestNestedObject)
    registrar
        .RegisterProperty(PropertyType::String, "name", &TestNestedObject::m_name, "名称")
        .RegisterProperty(PropertyType::Struct, "nested", &TestNestedObject::m_nested, "嵌套对象");
    END_DECLARE_OBJECT()

public:
    TestNestedObject() : m_name(""), m_nested(nullptr) {}

    std::string GetTypeName() const override { return "TestNestedObject"; }

    std::string m_name;
    TestComplexObject* m_nested;
};

// 测试基本类型序列化
TEST(ReflectableObjectTest, BasicTypesSerialization) {
    TestDerivedObject obj;
    obj.m_intValue = 42;
    obj.m_floatValue = 3.14f;
    obj.m_stringValue = "Hello";
    obj.m_boolValue = true;

    json j = obj.ToJson();

    EXPECT_EQ(j["Type"], "TestDerivedObject");
    EXPECT_TRUE(j.contains("Properties"));

    // 检查属性值
    EXPECT_EQ(j["Properties"]["TestDerivedObject::intValue"], 42);
    EXPECT_FLOAT_EQ(j["Properties"]["TestDerivedObject::floatValue"], 3.14f);
    EXPECT_EQ(j["Properties"]["TestDerivedObject::stringValue"], "Hello");
    EXPECT_EQ(j["Properties"]["TestDerivedObject::boolValue"], true);
}

TEST(ReflectableObjectTest, NewReflectionRegistryAdapterFindsMigratedType) {
    GetTestBaseObjectTypeDesc();

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

// 测试图形类型序列化
TEST(ReflectableObjectTest, GraphicsTypesSerialization) {
    TestComplexObject obj;
    obj.m_colorValue = IM_COL32(255, 128, 64, 32);
    obj.m_vec2Value = ImVec2(10.5f, 20.5f);

    json j = obj.ToJson();

    // 检查颜色值（RGBA 数组）
    EXPECT_TRUE(j["Properties"]["TestComplexObject::colorValue"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"].size(), 4);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][0], 255);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][1], 128);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][2], 64);
    EXPECT_EQ(j["Properties"]["TestComplexObject::colorValue"][3], 32);

    // 检查向量值（XY 数组）
    EXPECT_TRUE(j["Properties"]["TestComplexObject::vec2Value"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::vec2Value"].size(), 2);
    EXPECT_FLOAT_EQ(j["Properties"]["TestComplexObject::vec2Value"][0], 10.5f);
    EXPECT_FLOAT_EQ(j["Properties"]["TestComplexObject::vec2Value"][1], 20.5f);
}

// 测试数组类型序列化
TEST(ReflectableObjectTest, ArrayTypeSerialization) {
    TestComplexObject obj;
    obj.m_stringArrayValue = {"apple", "banana", "cherry"};

    json j = obj.ToJson();

    EXPECT_TRUE(j["Properties"]["TestComplexObject::stringArrayValue"].is_array());
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"].size(), 3);
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][0], "apple");
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][1], "banana");
    EXPECT_EQ(j["Properties"]["TestComplexObject::stringArrayValue"][2], "cherry");
}

// 测试枚举类型序列化
TEST(ReflectableObjectTest, EnumTypeSerialization) {
    TestComplexObject obj;
    obj.m_enumValue = 1; // Option2

    json j = obj.ToJson();

    EXPECT_EQ(j["Properties"]["TestComplexObject::enumValue"], "Option2");
}

// 测试继承链序列化
TEST(ReflectableObjectTest, InheritanceChainSerialization) {
    TestDerivedObject obj;
    obj.m_baseIntValue = 100;
    obj.m_baseFloatValue = 1.5f;
    obj.m_baseStringValue = "Base";
    obj.m_intValue = 200;
    obj.m_floatValue = 2.5f;
    obj.m_stringValue = "Derived";

    json j = obj.ToJson();

    // 检查基类属性
    EXPECT_EQ(j["Properties"]["TestBaseObject::baseIntValue"], 100);
    EXPECT_FLOAT_EQ(j["Properties"]["TestBaseObject::baseFloatValue"], 1.5f);
    EXPECT_EQ(j["Properties"]["TestBaseObject::baseStringValue"], "Base");

    // 检查派生类属性
    EXPECT_EQ(j["Properties"]["TestDerivedObject::intValue"], 200);
    EXPECT_FLOAT_EQ(j["Properties"]["TestDerivedObject::floatValue"], 2.5f);
    EXPECT_EQ(j["Properties"]["TestDerivedObject::stringValue"], "Derived");
}

// 测试嵌套对象序列化
TEST(ReflectableObjectTest, NestedObjectSerialization) {
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

// 测试基本类型反序列化
TEST(ReflectableObjectTest, BasicTypesDeserialization) {
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

// 测试图形类型反序列化
TEST(ReflectableObjectTest, GraphicsTypesDeserialization) {
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

// 测试枚举类型反序列化
TEST(ReflectableObjectTest, EnumTypeDeserialization) {
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

// 测试往返序列化（序列化后再反序列化）
TEST(ReflectableObjectTest, RoundTripSerialization) {
    TestComplexObject original;
    original.m_intValue = 42;
    original.m_floatValue = 3.14f;
    original.m_boolValue = true;
    original.m_stringValue = "Test";
    original.m_colorValue = IM_COL32(255, 128, 64, 32);
    original.m_vec2Value = ImVec2(10.5f, 20.5f);
    original.m_stringArrayValue = {"a", "b", "c"};
    original.m_enumValue = 2;

    // 序列化
    json j = original.ToJson();

    // 反序列化
    TestComplexObject restored;
    restored.FromJson(j);

    // 验证所有值
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

// 测试 JSON 格式验证
TEST(ReflectableObjectTest, InvalidJsonFormat) {
    TestDerivedObject obj;

    // 缺少 Type 字段
    json j1 = {{"Properties", {}}};
    EXPECT_THROW(obj.FromJson(j1), std::runtime_error);

    // 缺少 Properties 字段
    json j2 = {{"Type", "TestDerivedObject"}};
    EXPECT_THROW(obj.FromJson(j2), std::runtime_error);

    // 类型不匹配
    json j3 = {
        {"Type", "WrongType"},
        {"Properties", {}}
    };
    EXPECT_THROW(obj.FromJson(j3), std::runtime_error);
}

// 测试无效枚举值
TEST(ReflectableObjectTest, InvalidEnumValue) {
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::enumValue", "InvalidOption"}
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

// 测试无效颜色格式
TEST(ReflectableObjectTest, InvalidColorFormat) {
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::colorValue", {255, 128}} // 只有 2 个元素
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

// 测试无效向量格式
TEST(ReflectableObjectTest, InvalidVec2Format) {
    json j = {
        {"Type", "TestComplexObject"},
        {"Properties", {
            {"TestComplexObject::vec2Value", {10.5}} // 只有 1 个元素
        }}
    };

    TestComplexObject obj;
    EXPECT_THROW(obj.FromJson(j), std::runtime_error);
}

// 测试额外字段处理（应该被忽略）
TEST(ReflectableObjectTest, ExtraFieldsIgnored) {
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
