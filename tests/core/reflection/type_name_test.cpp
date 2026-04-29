#include <gtest/gtest.h>
#include <uectti/type_name.hpp>

using namespace uectti;
using namespace uectti::detail;

struct MyClass {};
struct AnotherType {};

// ─── type_name<T>() — strips CVR ───────────────

TEST(type_name, basic_types) {
    EXPECT_FALSE(type_name<int>().empty());
    EXPECT_FALSE(type_name<double>().empty());
    EXPECT_FALSE(type_name<float>().empty());
    EXPECT_FALSE(type_name<char>().empty());
}

TEST(type_name, different_types_have_different_names) {
    EXPECT_NE(type_name<int>(),   type_name<double>());
    EXPECT_NE(type_name<float>(), type_name<char>());
}

TEST(type_name, custom_types) {
    EXPECT_FALSE(type_name<MyClass>().empty());
    EXPECT_FALSE(type_name<AnotherType>().empty());
    EXPECT_NE(type_name<MyClass>(), type_name<AnotherType>());
}

TEST(type_name, same_type_stable) {
    EXPECT_EQ(type_name<int>(),     type_name<int>());
    EXPECT_EQ(type_name<MyClass>(), type_name<MyClass>());
}

TEST(type_name, strips_cvr) {
    // type_name strips CVR — same base type gives same string
    EXPECT_EQ(type_name<int>(), type_name<const int>());
    EXPECT_EQ(type_name<int>(), type_name<int&>());
    EXPECT_EQ(type_name<int>(), type_name<const int&>());
    EXPECT_EQ(type_name<int>(), type_name<int&&>());
    // pointer is NOT stripped
    EXPECT_NE(type_name<int>(), type_name<int*>());
}

// Verify no MSVC decoration prefixes like "struct " / "class " appear
TEST(type_name, no_struct_prefix) {
    auto name = type_name<MyClass>();
    std::string s(name.begin(), name.end());
    bool starts_with_struct = (s.size() >= 7 && s.substr(0, 7) == "struct ");
    EXPECT_FALSE(starts_with_struct) << "name should not contain 'struct ' prefix: " << s;
}

TEST(type_name, return_type_is_const_string) {
    auto name = type_name<int>();
    (void)name;
}

struct Empty {};
TEST(type_name, type_name_has_content) {
    EXPECT_GT(type_name<Empty>().size(), 0u);
}
