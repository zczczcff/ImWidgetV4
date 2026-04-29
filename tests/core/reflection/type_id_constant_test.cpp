#include <gtest/gtest.h>
#include <uectti/type_id_constant.hpp>

using namespace uectti;

struct MyType {};

// ─── type_id_constant<T> — strips CVR ─────────

TEST(type_id_constant, value_matches_type_id) {
    type_id_constant<int> ci;
    type_id_constant<double> cd;
    EXPECT_EQ(ci.value(), type_id<int>());
    EXPECT_EQ(cd.value(), type_id<double>());
}

TEST(type_id_constant, different_types_differ) {
    EXPECT_NE(type_id_constant<int>().value(), type_id_constant<double>().value());
}

TEST(type_id_constant, implicit_conversion) {
    type_id_t id = type_id_constant<int>{};
    EXPECT_EQ(id, type_id<int>());
}

TEST(type_id_constant, function_call_operator) {
    auto c = type_id_constant<int>{};
    EXPECT_EQ(c(), type_id<int>());
}

TEST(type_id_constant, custom_type) {
    EXPECT_NE(type_id_constant<MyType>().value(), type_id_constant<int>().value());
    EXPECT_EQ(type_id_constant<MyType>().value(), type_id<MyType>());
}

TEST(type_id_constant, strips_cvr) {
    EXPECT_EQ(type_id_constant<int>().value(),
              type_id_constant<const int&>().value());
}

TEST(type_id_constant, type_aliases) {
    bool same_type = std::is_same<type_id_constant<int>::type, int>::value;
    EXPECT_TRUE(same_type);
    bool same_value = std::is_same<type_id_constant<int>::value_type, type_id_t>::value;
    EXPECT_TRUE(same_value);
}

// ─── type_id_constant_with_cvr<T> — preserves ─

TEST(type_id_constant_with_cvr, preserves_cvr) {
    EXPECT_NE(type_id_constant_with_cvr<int>().value(),
              type_id_constant_with_cvr<const int>().value());
}

TEST(type_id_constant_with_cvr, matches_type_id_with_cvr) {
    EXPECT_EQ(type_id_constant_with_cvr<int>().value(),
              type_id_with_cvr<int>());
    EXPECT_EQ(type_id_constant_with_cvr<const int>().value(),
              type_id_with_cvr<const int>());
}
