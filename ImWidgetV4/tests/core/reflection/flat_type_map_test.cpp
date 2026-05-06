#include <gtest/gtest.h>
#include <uectti/flat_type_map.hpp>

using namespace uectti;

struct KeyA {};
struct KeyB {};
struct KeyC {};

TEST(flat_type_map, default_construction) {
    flat_type_map<void, int, 4> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(0u, map.size());
    EXPECT_FALSE(map.full());
}

TEST(flat_type_map, insert_and_retrieve) {
    flat_type_map<void, int, 4> map;
    map.get<int>() = 42;
    EXPECT_EQ(42, map.get<int>());
    EXPECT_EQ(1u, map.size());
    EXPECT_FALSE(map.empty());
}

TEST(flat_type_map, different_keys_independent) {
    flat_type_map<void, int, 4> map;
    map.get<int>()  = 10;
    map.get<double>() = 20;

    EXPECT_EQ(10, map.get<int>());
    EXPECT_EQ(20, map.get<double>());
    EXPECT_EQ(2u, map.size());
}

TEST(flat_type_map, contains) {
    flat_type_map<void, int, 4> map;
    EXPECT_FALSE(map.contains<int>());

    map.get<int>() = 42;
    EXPECT_TRUE(map.contains<int>());
    EXPECT_FALSE(map.contains<double>());
}

TEST(flat_type_map, clear) {
    flat_type_map<void, int, 4> map;
    map.get<int>() = 42;
    EXPECT_FALSE(map.empty());

    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(0u, map.size());
}

TEST(flat_type_map, full) {
    flat_type_map<void, int, 2> map;
    EXPECT_FALSE(map.full());

    map.get<int>()    = 1;
    EXPECT_FALSE(map.full());

    map.get<double>() = 2;
    EXPECT_TRUE(map.full());
}

TEST(flat_type_map, capacity_limit) {
    flat_type_map<void, int, 1> map;
    map.get<int>()    = 10;
    map.get<double>() = 20;  // should not crash, will overwrite last slot

    EXPECT_EQ(1u, map.size());
    EXPECT_TRUE(map.full());
}

TEST(flat_type_map, type_independence) {
    flat_type_map<void, int, 4> map;
    map.get<KeyA>() = 100;
    map.get<KeyB>() = 200;
    map.get<KeyC>() = 300;

    EXPECT_EQ(100, map.get<KeyA>());
    EXPECT_EQ(200, map.get<KeyB>());
    EXPECT_EQ(300, map.get<KeyC>());
    EXPECT_EQ(3u, map.size());
}

TEST(flat_type_map, const_get) {
    flat_type_map<void, int, 4> map;
    map.get<int>() = 42;

    const auto& cmap = map;
    EXPECT_EQ(42, cmap.get<int>());
}
