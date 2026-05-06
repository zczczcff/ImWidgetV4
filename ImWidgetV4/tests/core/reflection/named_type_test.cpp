#include <gtest/gtest.h>
#include <uectti/named_type.hpp>
#include <uectti/type_id.hpp>

using namespace uectti;

// distinct tags
struct MeterTag {};
struct SecondTag {};

using Meter   = named_type<MeterTag, double>;
using Second  = named_type<SecondTag, double>;

TEST(named_type, construction_and_get) {
    Meter m{42.0};
    EXPECT_DOUBLE_EQ(42.0, m.get());
}

TEST(named_type, default_construction) {
    Meter m{};
    EXPECT_DOUBLE_EQ(0.0, m.get());
}

TEST(named_type, explicit_conversion) {
    Meter m{3.14};
    double val = static_cast<double>(m);
    EXPECT_DOUBLE_EQ(3.14, val);
}

TEST(named_type, different_tags_are_distinct_types) {
    // This test verifies the types are distinct via type_id
    auto meter_id  = type_id<Meter>();
    auto second_id = type_id<Second>();
    EXPECT_NE(meter_id, second_id);
}

TEST(named_type, same_tag_and_value_equal) {
    Meter a{10.0};
    Meter b{10.0};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(named_type, different_values_not_equal) {
    Meter a{10.0};
    Meter b{20.0};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(named_type, less_than) {
    Meter a{5.0};
    Meter b{10.0};
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(named_type, greater_than) {
    Meter a{10.0};
    Meter b{5.0};
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(named_type, less_equal) {
    Meter a{5.0};
    Meter b{5.0};
    Meter c{10.0};
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(c <= a);
}

TEST(named_type, greater_equal) {
    Meter a{10.0};
    Meter b{10.0};
    Meter c{5.0};
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(c >= a);
}

TEST(named_type, swap) {
    Meter a{1.0};
    Meter b{2.0};
    a.swap(b);
    EXPECT_DOUBLE_EQ(2.0, a.get());
    EXPECT_DOUBLE_EQ(1.0, b.get());
}

TEST(named_type, hash) {
    Meter::hash hasher;
    Meter m{42.0};
    // same value should hash the same
    EXPECT_EQ(hasher(m), hasher(Meter{42.0}));
    // different values may hash differently (std::hash<double> may have collisions)
    // we just verify it compiles and returns a size_t
    auto h = hasher(m);
    EXPECT_NE(0u, h); // very likely non-zero for non-zero values
}

TEST(named_type, string_type) {
    using Name = named_type<MeterTag, std::string>;
    Name n{"hello"};
    EXPECT_EQ("hello", n.get());
}

TEST(named_type, move_construction) {
    using Name = named_type<MeterTag, std::string>;
    Name n1{std::string("move_me")};
    Name n2{std::move(n1)};
    EXPECT_EQ("move_me", n2.get());
}
