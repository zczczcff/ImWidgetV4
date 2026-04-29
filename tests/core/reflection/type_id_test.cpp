#include <gtest/gtest.h>
#include <uectti/type_id.hpp>

using namespace uectti;

struct CustomType {};
struct OtherType {};
namespace ns { struct Nested {}; }

// ─── type_id<T>() — strips CVR ───────────────────

TEST(type_id, basic_types) {
    EXPECT_NE(0u, type_id<int>());
    EXPECT_NE(0u, type_id<double>());
    EXPECT_NE(0u, type_id<float>());
    EXPECT_NE(0u, type_id<char>());
    EXPECT_NE(0u, type_id<bool>());
    EXPECT_NE(0u, type_id<void>());
}

TEST(type_id, stability) {
    EXPECT_EQ(type_id<int>(),   type_id<int>());
    EXPECT_EQ(type_id<double>(), type_id<double>());
}

TEST(type_id, different_types_differ) {
    EXPECT_NE(type_id<int>(),   type_id<double>());
    EXPECT_NE(type_id<float>(), type_id<char>());
    EXPECT_NE(type_id<bool>(),  type_id<void>());
}

TEST(type_id, custom_types) {
    EXPECT_NE(type_id<CustomType>(), type_id<OtherType>());
    EXPECT_EQ(type_id<CustomType>(), type_id<CustomType>());
}

TEST(type_id, nested_type) {
    EXPECT_NE(0u, type_id<ns::Nested>());
    EXPECT_NE(type_id<ns::Nested>(), type_id<CustomType>());
}

TEST(type_id, strips_cvr) {
    // type_id strips const/volatile/reference — same base type → same id
    EXPECT_EQ(type_id<int>(),       type_id<const int>());
    EXPECT_EQ(type_id<int>(),       type_id<int&>());
    EXPECT_EQ(type_id<int>(),       type_id<const int&>());
    EXPECT_EQ(type_id<int>(),       type_id<int&&>());
    EXPECT_EQ(type_id<CustomType>(), type_id<const CustomType&>());

    // pointer is NOT stripped
    EXPECT_NE(type_id<int>(),       type_id<int*>());
}

TEST(type_id, type_id_t_is_uint64) {
    bool same = std::is_same<type_id_t, std::uint64_t>::value;
    EXPECT_TRUE(same);
}

TEST(type_id, type_id_equal) {
    type_id_equal eq;
    EXPECT_TRUE( eq(type_id<int>(), type_id<int>()));
    EXPECT_FALSE(eq(type_id<int>(), type_id<double>()));
}

TEST(type_id, type_id_less) {
    type_id_less less;
    bool result = less(type_id<int>(), type_id<double>());
    EXPECT_EQ(result, type_id<int>() < type_id<double>());
}

// ─── type_id_with_cvr<T>() — preserves CVR ───────

TEST(type_id_with_cvr, preserves_cvr) {
    // type_id_with_cvr preserves const/volatile/reference
    EXPECT_NE(type_id_with_cvr<int>(),       type_id_with_cvr<const int>());
    EXPECT_NE(type_id_with_cvr<int>(),       type_id_with_cvr<int&>());
    EXPECT_NE(type_id_with_cvr<int>(),       type_id_with_cvr<const int&>());
    EXPECT_NE(type_id_with_cvr<int>(),       type_id_with_cvr<int&&>());
    EXPECT_NE(type_id_with_cvr<CustomType>(), type_id_with_cvr<const CustomType&>());
}

TEST(type_id_with_cvr, stability) {
    EXPECT_EQ(type_id_with_cvr<int>(),       type_id_with_cvr<int>());
    EXPECT_EQ(type_id_with_cvr<const int>(), type_id_with_cvr<const int>());
}

TEST(type_id_with_cvr, different_types_differ) {
    EXPECT_NE(type_id_with_cvr<int>(),   type_id_with_cvr<double>());
    EXPECT_NE(type_id_with_cvr<float>(), type_id_with_cvr<char>());
}
