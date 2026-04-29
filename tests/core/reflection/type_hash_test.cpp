#include <gtest/gtest.h>
#include <uectti/type_hash.hpp>
#include <unordered_set>

using namespace uectti;

struct TypeA {};
struct TypeB {};

// ─── type_hash<T> — strips CVR ───────────────

TEST(type_hash, returns_type_id_as_size_t) {
    type_hash<int> h;
    EXPECT_EQ(static_cast<std::size_t>(type_id<int>()), h(static_cast<int>(0)));
    EXPECT_EQ(static_cast<std::size_t>(type_id<int>()), h(42));
}

TEST(type_hash, different_types_differ) {
    type_hash<int> hi;
    type_hash<double> hd;
    EXPECT_NE(hi(static_cast<int>(0)), hd(0.0));
}

TEST(type_hash, strips_cvr) {
    type_hash<int>       h_int;
    type_hash<const int> h_const_int;
    EXPECT_EQ(h_int(0), h_const_int(0));
}

TEST(type_hash, type_id_overload) {
    type_hash<int> h;
    EXPECT_EQ(static_cast<std::size_t>(type_id<int>()), h(type_id<int>()));
}

TEST(type_hash, works_with_unordered_set) {
    std::unordered_set<type_id_t, type_hash<void>> ids;
    ids.insert(type_id<int>());
    ids.insert(type_id<double>());
    EXPECT_EQ(2u, ids.size());
    EXPECT_TRUE(ids.find(type_id<int>()) != ids.end());
    EXPECT_TRUE(ids.find(type_id<double>()) != ids.end());
}

TEST(type_hash, argument_type_and_result_type) {
    bool same = std::is_same<type_hash<int>::argument_type, int>::value;
    EXPECT_TRUE(same);
    bool res  = std::is_same<type_hash<int>::result_type, std::size_t>::value;
    EXPECT_TRUE(res);
}

TEST(type_hash, custom_types) {
    type_hash<TypeA> ha;
    type_hash<TypeB> hb;
    EXPECT_NE(ha(TypeA{}), hb(TypeB{}));
}

// ─── type_hash_with_cvr<T> — preserves CVR ───

TEST(type_hash_with_cvr, preserves_cvr) {
    type_hash_with_cvr<int>       h_int;
    type_hash_with_cvr<const int> h_const_int;
    EXPECT_NE(h_int(0), h_const_int(0));
}

TEST(type_hash_with_cvr, different_types_differ) {
    type_hash_with_cvr<int> hi;
    type_hash_with_cvr<double> hd;
    EXPECT_NE(hi(0), hd(0.0));
}

TEST(type_hash_with_cvr, type_id_overload) {
    type_hash_with_cvr<int> h;
    EXPECT_EQ(static_cast<std::size_t>(type_id_with_cvr<int>()),
              h(type_id_with_cvr<int>()));
}
