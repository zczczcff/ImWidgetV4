#ifndef UECTTI_DETAIL_CONST_STRING_HPP
#define UECTTI_DETAIL_CONST_STRING_HPP

#include <cstddef>
#include <cstdint>

namespace uectti {
namespace detail {

class const_string {
    const char* m_begin;
    const char* m_end;

public:
    constexpr const_string() noexcept : m_begin(nullptr), m_end(nullptr) {}

    constexpr const_string(const char* b, const char* e) noexcept
        : m_begin(b), m_end(e) {}

    constexpr const char* begin() const noexcept { return m_begin; }
    constexpr const char* end()   const noexcept { return m_end; }

    constexpr bool empty() const noexcept { return m_begin == m_end; }

    constexpr std::size_t size() const noexcept {
        return static_cast<std::size_t>(m_end - m_begin);
    }

    constexpr char operator[](std::size_t i) const noexcept {
        return m_begin[i];
    }

    constexpr char front() const noexcept { return *m_begin; }
    constexpr char back()  const noexcept { return *(m_end - 1); }

    // FNV-1a 64-bit hash
    constexpr std::uint64_t hash() const noexcept {
        std::uint64_t h = 14695981039346656037ULL;
        for (auto p = m_begin; p != m_end; ++p) {
            h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(*p));
            h *= 1099511628211ULL;
        }
        return h;
    }

    // Lexicographic compare: -1, 0, 1
    constexpr int compare(const const_string& other) const noexcept {
        auto a = m_begin;
        auto b = other.m_begin;
        auto a_end = m_end;
        auto b_end = other.m_end;

        while (a != a_end && b != b_end) {
            if (*a < *b) return -1;
            if (*a > *b) return 1;
            ++a;
            ++b;
        }

        if (a == a_end && b == b_end) return 0;
        if (a == a_end) return -1;
        return 1;
    }
};

// --- Free functions ---

constexpr bool operator==(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) == 0;
}

constexpr bool operator!=(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) != 0;
}

constexpr bool operator<(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) < 0;
}

constexpr bool operator>(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) > 0;
}

constexpr bool operator<=(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) <= 0;
}

constexpr bool operator>=(const const_string& a, const const_string& b) noexcept {
    return a.compare(b) >= 0;
}

// Compile-time strlen
constexpr std::size_t const_strlen(const char* s) noexcept {
    std::size_t n = 0;
    while (*s) { ++n; ++s; }
    return n;
}

} // namespace detail
} // namespace uectti

#endif // UECTTI_DETAIL_CONST_STRING_HPP
