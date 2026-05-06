#include <gtest/gtest.h>
#include <imwidgetv4/core/rop/RunTimeObjectProperty.h>
#include <string>
#include <vector>

// 定义测试用的属性枚举类型
enum class TestPropertyType
{
    INT,
    FLOAT,
    DOUBLE,
    STRING,
    BOOL,
    OPTIONAL
};

// 测试基类
class TestBaseObject : public ROP::PropertyObject<TestPropertyType>
{
    DECLARE_OBJECT(TestBaseObject)
    registrar
        .RegisterProperty(TestPropertyType::INT, "baseIntValue", &TestBaseObject::baseIntValue, "基类整数属性")
        .RegisterProperty(TestPropertyType::FLOAT, "baseFloatValue", &TestBaseObject::baseFloatValue, "基类浮点数属性")
        .RegisterProperty(TestPropertyType::STRING, "baseStringValue", &TestBaseObject::baseStringValue, "基类字符串属性");
    END_DECLARE_OBJECT()

public:
    TestBaseObject() : baseIntValue(0), baseFloatValue(0.0f), baseStringValue("") {}

    int baseIntValue;
    float baseFloatValue;
    std::string baseStringValue;
};

// 测试派生类
class TestDerivedObject : public TestBaseObject
{
    DECLARE_OBJECT_WITH_PARENT(TestDerivedObject, TestBaseObject)
    registrar
        .RegisterProperty(TestPropertyType::INT, "intValue", &TestDerivedObject::intValue, "派生类整数属性")
        .RegisterProperty(TestPropertyType::FLOAT, "floatValue", &TestDerivedObject::floatValue, "派生类浮点数属性")
        .RegisterProperty(TestPropertyType::STRING, "stringValue", &TestDerivedObject::stringValue, "派生类字符串属性")
        .RegisterProperty(TestPropertyType::BOOL, "boolValue", &TestDerivedObject::boolValue, "派生类布尔属性");
    END_DECLARE_OBJECT()

public:
    TestDerivedObject() : intValue(0), floatValue(0.0f), stringValue(""), boolValue(false) {}

    int intValue;
    float floatValue;
    std::string stringValue;
    bool boolValue;
};

// 测试可选属性类
class TestOptionalObject : public ROP::PropertyObject<TestPropertyType>
{
    DECLARE_OBJECT(TestOptionalObject)
    registrar
        .RegisterProperty(TestPropertyType::INT, "normalInt", &TestOptionalObject::normalInt, "普通整数属性")
        .RegisterOptionalProperty(
            TestPropertyType::OPTIONAL, "status", &TestOptionalObject::status,
            { "Offline", "Online", "Error", "Maintenance" },
            "设备状态")
        .RegisterOptionalProperty(
            TestPropertyType::OPTIONAL, "mode", &TestOptionalObject::mode,
            { "Auto", "Manual", "Debug" },
            "运行模式");
    END_DECLARE_OBJECT()

public:
    TestOptionalObject() : normalInt(0), status(0), mode(0) {}

    int normalInt;
    int status;
    int mode;
};

// 测试自定义访问器类
class TestCustomAccessorObject : public ROP::PropertyObject<TestPropertyType>
{
    DECLARE_OBJECT(TestCustomAccessorObject)
    registrar
        .RegisterProperty(
            TestPropertyType::INT, "customInt",
            static_cast<void (TestCustomAccessorObject::*)(int&)>(&TestCustomAccessorObject::SetCustomInt),
            static_cast<int& (TestCustomAccessorObject::*)()>(&TestCustomAccessorObject::GetCustomInt),
            "自定义整数属性")
        .RegisterProperty(TestPropertyType::INT, "directInt", &TestCustomAccessorObject::directInt, "直接整数属性");
    END_DECLARE_OBJECT()

public:
    TestCustomAccessorObject() : m_customInt(0), directInt(0) {}

    void SetCustomInt(int& value)
    {
        if (value < 0) value = 0;
        if (value > 1000) value = 1000;
        m_customInt = value;
        directInt = value * 2;
    }

    int& GetCustomInt()
    {
        return m_customInt;
    }

private:
    int m_customInt;

public:
    int directInt;
};

// 测试同名属性类（派生类与基类有同名属性）
class TestSameNameObject : public TestBaseObject
{
    DECLARE_OBJECT_WITH_PARENT(TestSameNameObject, TestBaseObject)
    registrar
        .RegisterProperty(TestPropertyType::INT, "baseIntValue", &TestSameNameObject::derivedIntValue, "派生类同名整数属性");
    END_DECLARE_OBJECT()

public:
    TestSameNameObject() : derivedIntValue(0) {}

    int derivedIntValue;
};

// ==================== 基本属性注册和访问测试 ====================

TEST(ROPTest, BasicPropertyRegistration)
{
    TestBaseObject obj;
    obj.baseIntValue = 42;
    obj.baseFloatValue = 3.14f;
    obj.baseStringValue = "test";

    // 测试属性是否存在
    EXPECT_TRUE(obj.HasProperty("baseIntValue"));
    EXPECT_TRUE(obj.HasProperty("baseFloatValue"));
    EXPECT_TRUE(obj.HasProperty("baseStringValue"));
    EXPECT_FALSE(obj.HasProperty("nonExistentProperty"));
}

TEST(ROPTest, BasicPropertyAccess)
{
    TestBaseObject obj;
    obj.baseIntValue = 42;
    obj.baseFloatValue = 3.14f;
    obj.baseStringValue = "test";

    // 通过 Property 包装器访问
    auto intProp = obj.GetProperty("baseIntValue");
    auto floatProp = obj.GetProperty("baseFloatValue");
    auto stringProp = obj.GetProperty("baseStringValue");

    EXPECT_TRUE(intProp.IsValid());
    EXPECT_TRUE(floatProp.IsValid());
    EXPECT_TRUE(stringProp.IsValid());

    EXPECT_EQ(intProp.GetValue<int>(), 42);
    EXPECT_FLOAT_EQ(floatProp.GetValue<float>(), 3.14f);
    EXPECT_EQ(stringProp.GetValue<std::string>(), "test");
}

TEST(ROPTest, BasicPropertyModification)
{
    TestBaseObject obj;

    auto intProp = obj.GetProperty("baseIntValue");
    auto floatProp = obj.GetProperty("baseFloatValue");
    auto stringProp = obj.GetProperty("baseStringValue");

    // 通过 Property 包装器修改值
    intProp.SetValue<int>(100);
    floatProp.SetValue<float>(2.71f);
    stringProp.SetValue<std::string>("modified");

    // 验证修改是否生效
    EXPECT_EQ(obj.baseIntValue, 100);
    EXPECT_FLOAT_EQ(obj.baseFloatValue, 2.71f);
    EXPECT_EQ(obj.baseStringValue, "modified");
}

// ==================== 属性继承测试 ====================

TEST(ROPTest, PropertyInheritance)
{
    TestDerivedObject obj;

    // 测试继承的属性
    EXPECT_TRUE(obj.HasProperty("baseIntValue"));
    EXPECT_TRUE(obj.HasProperty("baseFloatValue"));
    EXPECT_TRUE(obj.HasProperty("baseStringValue"));

    // 测试自身的属性
    EXPECT_TRUE(obj.HasProperty("intValue"));
    EXPECT_TRUE(obj.HasProperty("floatValue"));
    EXPECT_TRUE(obj.HasProperty("stringValue"));
    EXPECT_TRUE(obj.HasProperty("boolValue"));
}

TEST(ROPTest, InheritedPropertyAccess)
{
    TestDerivedObject obj;
    obj.baseIntValue = 10;
    obj.intValue = 20;

    auto baseProp = obj.GetProperty("baseIntValue");
    auto derivedProp = obj.GetProperty("intValue");

    EXPECT_TRUE(baseProp.IsValid());
    EXPECT_TRUE(derivedProp.IsValid());

    EXPECT_EQ(baseProp.GetValue<int>(), 10);
    EXPECT_EQ(derivedProp.GetValue<int>(), 20);

    // 修改继承的属性
    baseProp.SetValue<int>(30);
    EXPECT_EQ(obj.baseIntValue, 30);
}

TEST(ROPTest, GetAllPropertiesList)
{
    TestDerivedObject obj;

    auto allProps = obj.GetAllPropertiesList();

    // 应该包含基类的3个属性 + 派生类的4个属性 = 7个属性
    EXPECT_EQ(allProps.size(), 7);
}

// ==================== 可选属性测试 ====================

TEST(ROPTest, OptionalPropertyBasic)
{
    TestOptionalObject obj;
    obj.status = 1; // Online

    auto statusProp = obj.GetPropertyAsOptional("status");

    EXPECT_TRUE(statusProp.IsValid());
    EXPECT_TRUE(statusProp.IsOptional());

    // 测试获取选项字符串
    EXPECT_EQ(statusProp.GetOptionString(), "Online");

    // 测试获取选项列表
    auto options = statusProp.GetOptionList();
    EXPECT_EQ(options.size(), 4);
    EXPECT_EQ(options[0], "Offline");
    EXPECT_EQ(options[1], "Online");
    EXPECT_EQ(options[2], "Error");
    EXPECT_EQ(options[3], "Maintenance");
}

TEST(ROPTest, OptionalPropertySetByString)
{
    TestOptionalObject obj;

    auto statusProp = obj.GetPropertyAsOptional("status");

    // 通过字符串设置选项
    EXPECT_TRUE(statusProp.SetOptionByString("Error"));
    EXPECT_EQ(obj.status, 2);
    EXPECT_EQ(statusProp.GetOptionString(), "Error");

    // 设置无效选项应该失败
    EXPECT_FALSE(statusProp.SetOptionByString("InvalidOption"));
}

TEST(ROPTest, OptionalPropertySetByIndex)
{
    TestOptionalObject obj;

    auto modeProp = obj.GetPropertyAsOptional("mode");

    // 通过索引设置选项
    EXPECT_TRUE(modeProp.SetOptionByIndex(2)); // Debug
    EXPECT_EQ(obj.mode, 2);
    EXPECT_EQ(modeProp.GetOptionString(), "Debug");

    // 设置无效索引应该失败
    EXPECT_FALSE(modeProp.SetOptionByIndex(10));
}

// ==================== 自定义访问器测试 ====================

TEST(ROPTest, CustomAccessor)
{
    TestCustomAccessorObject obj;

    auto customProp = obj.GetProperty("customInt");

    // 测试自定义 setter 的验证逻辑
    customProp.SetValue<int>(500);
    EXPECT_EQ(obj.GetCustomInt(), 500);
    EXPECT_EQ(obj.directInt, 1000); // 副作用：directInt = customInt * 2

    // 测试边界验证（< 0）
    customProp.SetValue<int>(-10);
    EXPECT_EQ(obj.GetCustomInt(), 0);

    // 测试边界验证（> 1000）
    customProp.SetValue<int>(2000);
    EXPECT_EQ(obj.GetCustomInt(), 1000);
}

// ==================== 同名属性测试 ====================

TEST(ROPTest, SameNameProperties)
{
    TestSameNameObject obj;
    obj.baseIntValue = 10;
    obj.derivedIntValue = 20;

    // 获取所有同名属性
    auto props = obj.GetAllPropertiesByName("baseIntValue");

    // 应该有2个同名属性：基类的和派生类的
    EXPECT_EQ(props.size(), 2);

    // 默认 GetProperty 应该返回派生类的属性（最近的）
    auto prop = obj.GetProperty("baseIntValue");
    EXPECT_TRUE(prop.IsValid());
    EXPECT_EQ(prop.GetValue<int>(), 20); // 派生类的值

    // 通过类名获取特定类的属性
    auto baseProp = obj.GetProperty("baseIntValue", "TestBaseObject");
    auto derivedProp = obj.GetProperty("baseIntValue", "TestSameNameObject");

    EXPECT_TRUE(baseProp.IsValid());
    EXPECT_TRUE(derivedProp.IsValid());

    EXPECT_EQ(baseProp.GetValue<int>(), 10);
    EXPECT_EQ(derivedProp.GetValue<int>(), 20);
}

// ==================== 属性描述测试 ====================

TEST(ROPTest, PropertyDescription)
{
    TestBaseObject obj;

    auto intProp = obj.GetProperty("baseIntValue");
    EXPECT_EQ(intProp.GetDescription(), "基类整数属性");

    auto floatProp = obj.GetProperty("baseFloatValue");
    EXPECT_EQ(floatProp.GetDescription(), "基类浮点数属性");
}

// ==================== 属性路径访问测试 ====================

TEST(ROPTest, PropertyMetadata)
{
    TestDerivedObject obj;

    auto prop = obj.GetProperty("intValue");

    EXPECT_TRUE(prop.IsValid());
    EXPECT_EQ(prop.GetName(), "intValue");
    EXPECT_EQ(prop.GetClassName(), "TestDerivedObject");
    EXPECT_EQ(prop.GetType(), TestPropertyType::INT);
}

// ==================== 属性引用和指针访问测试 ====================

TEST(ROPTest, PropertyReferenceAccess)
{
    TestBaseObject obj;
    obj.baseIntValue = 42;

    auto prop = obj.GetProperty("baseIntValue");

    // 通过引用访问
    int& ref = prop.GetReference<int>();
    EXPECT_EQ(ref, 42);

    // 修改引用应该影响原始值
    ref = 100;
    EXPECT_EQ(obj.baseIntValue, 100);
}

TEST(ROPTest, PropertyPointerAccess)
{
    TestBaseObject obj;
    obj.baseIntValue = 42;

    auto prop = obj.GetProperty("baseIntValue");

    // 通过指针访问
    int* ptr = prop.GetPointer<int>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);

    // 修改指针指向的值应该影响原始值
    *ptr = 200;
    EXPECT_EQ(obj.baseIntValue, 200);
}

// ==================== 属性计数和唯一名称测试 ====================

TEST(ROPTest, PropertyCount)
{
    TestDerivedObject obj;

    // 基类3个属性 + 派生类4个属性 = 7个
    EXPECT_EQ(obj.GetPropertyCount(), 7);
}

TEST(ROPTest, UniquePropertyNames)
{
    TestDerivedObject obj;

    auto uniqueNames = obj.GetUniquePropertyNames();

    // 应该有7个唯一的属性名
    EXPECT_EQ(uniqueNames.size(), 7);
}

// ==================== 无效属性测试 ====================

TEST(ROPTest, InvalidProperty)
{
    TestBaseObject obj;

    auto invalidProp = obj.GetProperty("nonExistent");

    EXPECT_FALSE(invalidProp.IsValid());

    // 访问无效属性应该抛出异常
    EXPECT_THROW(invalidProp.GetValue<int>(), std::runtime_error);
    EXPECT_THROW(invalidProp.SetValue<int>(10), std::runtime_error);
}

// ==================== 主函数 ====================

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
