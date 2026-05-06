#include <gtest/gtest.h>
#include <uectti/detail/const_string.hpp>

using namespace uectti::detail;

// --- compile-time static_asserts ---
static_assert(const_string().empty(),   "default const_string should be empty");
static_assert(const_string().size() == 0, "default const_string size == 0");

constexpr const char kHello[] = "hello";
constexpr const_string cs_hello(kHello, kHello + 5);

static_assert(!cs_hello.empty(),     "hello should not be empty");
static_assert(cs_hello.size() == 5,  "hello.size() == 5");
static_assert(cs_hello[0] == 'h',    "hello[0] == h");

static_assert(cs_hello.hash() != 0,  "non-empty string hash != 0");

constexpr auto cs_empty_hash = const_string().hash();
constexpr auto fnv_offset = 14695981039346656037ULL;
static_assert(cs_empty_hash == fnv_offset, "empty string hash == FNV offset basis");

// same strings => equal
constexpr const_string cs_hello2(kHello, kHello + 5);
constexpr const_string cs_world("world", "world" + 5);

static_assert(cs_hello == cs_hello2, "identical strings should be equal");
static_assert(cs_hello != cs_world,  "different strings should not be equal");

static_assert(cs_hello.compare(cs_hello2) == 0, "identical compare == 0");
static_assert(cs_hello < cs_world,  "\"hello\" < \"world\"");

// --- runtime tests ---
TEST(const_string, default_empty) {
    const_string s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0u, s.size());
}

TEST(const_string, from_pointers) {
    static constexpr char data[] = "test";
    constexpr const_string s(data, data + 4);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(4u, s.size());
    EXPECT_EQ('t', s[0]);
    EXPECT_EQ('t', s.front());
    EXPECT_EQ('t', s.back());
}

TEST(const_string, hash) {
    static constexpr char data[] = "hello";
    constexpr const_string s(data, data + 5);
    // FNV-1a of "hello" should be this specific value
    EXPECT_EQ(11831194018420276491ULL, s.hash());
}

TEST(const_string, compare_equal) {
    static constexpr char a[] = "abc";
    static constexpr char b[] = "abc";
    constexpr const_string sa(a, a + 3);
    constexpr const_string sb(b, b + 3);
    EXPECT_EQ(0, sa.compare(sb));
    EXPECT_TRUE(sa == sb);
    EXPECT_FALSE(sa != sb);
}

TEST(const_string, compare_less) {
    static constexpr char a[] = "abc";
    static constexpr char b[] = "abd";
    constexpr const_string sa(a, a + 3);
    constexpr const_string sb(b, b + 3);
    EXPECT_EQ(-1, sa.compare(sb));
    EXPECT_TRUE(sa < sb);
    EXPECT_FALSE(sa > sb);
}

TEST(const_string, compare_greater) {
    static constexpr char a[] = "abd";
    static constexpr char b[] = "abc";
    constexpr const_string sa(a, a + 3);
    constexpr const_string sb(b, b + 3);
    EXPECT_EQ(1, sa.compare(sb));
    EXPECT_TRUE(sa > sb);
    EXPECT_FALSE(sa < sb);
}

TEST(const_string, same_string_repeatable_hash) {
    static constexpr char data[] = "some_longer_string_12345";
    constexpr const_string s(data, data + 25);
    auto h1 = s.hash();
    auto h2 = s.hash();
    EXPECT_EQ(h1, h2);
}

TEST(const_string, different_strings_different_hash) {
    static constexpr char a[] = "abcdef";
    static constexpr char b[] = "abcdee";
    constexpr const_string sa(a, a + 6);
    constexpr const_string sb(b, b + 6);
    EXPECT_NE(sa.hash(), sb.hash());
}
