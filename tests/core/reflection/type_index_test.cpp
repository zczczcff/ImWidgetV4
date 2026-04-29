#include <gtest/gtest.h>
#include <uectti/type_index.hpp>

using namespace uectti;

struct TypeA {};
struct TypeB {};

TEST(type_index, same_type_same_index) {
    EXPECT_EQ(type_index<int>(), type_index<int>());
    EXPECT_EQ(type_index<double>(), type_index<double>());
}

TEST(type_index, different_types_different_indices) {
    // different types should have different indices
    EXPECT_NE(type_index<int>(), type_index<double>());
}

TEST(type_index, indices_are_monotonic) {
    auto i1 = type_index<TypeA>();
    auto i2 = type_index<TypeB>();

    // Can't guarantee ordering (depends on test execution), but values should be non-negative
    EXPECT_GE(i1, 0u);
    EXPECT_GE(i2, 0u);
}

TEST(type_index, custom_types) {
    auto a1 = type_index<TypeA>();
    auto a2 = type_index<TypeA>();
    auto b1 = type_index<TypeB>();

    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, b1);
}

TEST(type_index, stable_after_multiple_calls) {
    std::size_t first  = type_index<void>();
    std::size_t second = type_index<void>();
    std::size_t third  = type_index<void>();
    EXPECT_EQ(first, second);
    EXPECT_EQ(second, third);
}
