#ifndef UECTTI_DETAIL_PARSER_HPP
#define UECTTI_DETAIL_PARSER_HPP

#include "const_string.hpp"

namespace uectti {
namespace detail {

// Find substring in haystack, return start index or ~size_t(0)
constexpr std::size_t find_substring(
    const char* haystack, std::size_t haystack_len,
    const char* needle,   std::size_t needle_len,
    std::size_t pos = 0) noexcept
{
    if (needle_len == 0) return pos;
    if (haystack_len < needle_len || pos > haystack_len - needle_len)
        return ~std::size_t(0);

    for (std::size_t i = pos; i <= haystack_len - needle_len; ++i) {
        std::size_t j = 0;
        while (j < needle_len && haystack[i + j] == needle[j]) {
            ++j;
        }
        if (j == needle_len) return i;
    }
    return ~std::size_t(0);
}

// Extract content between func_name< and matching >
// Handles nested angle brackets by tracking depth
constexpr const_string extract_template_arg(
    const char* raw, const char* func_name) noexcept
{
    std::size_t raw_len = const_strlen(raw);
    std::size_t fn_len  = const_strlen(func_name);

    std::size_t pos = find_substring(raw, raw_len, func_name, fn_len, 0);
    if (pos == ~std::size_t(0)) {
        return const_string{};
    }

    std::size_t start = pos + fn_len;
    if (start >= raw_len) {
        return const_string{};
    }

    // Find matching '>', tracking nesting depth
    std::size_t depth = 1;
    std::size_t end = start;
    while (end < raw_len && depth > 0) {
        if (raw[end] == '<') {
            ++depth;
        } else if (raw[end] == '>') {
            --depth;
        }
        if (depth > 0) ++end;
    }

    if (end <= start) {
        return const_string{};
    }

    return const_string(raw + start, raw + end);
}

constexpr const_string extract_between_markers(
    const char* raw,
    const char* begin_marker,
    const char* end_marker) noexcept
{
    const std::size_t raw_len = const_strlen(raw);
    const std::size_t begin_len = const_strlen(begin_marker);
    const std::size_t end_len = const_strlen(end_marker);

    const std::size_t begin_pos = find_substring(raw, raw_len, begin_marker, begin_len, 0);
    if (begin_pos == ~std::size_t(0)) {
        return const_string{};
    }

    const std::size_t start = begin_pos + begin_len;
    if (start >= raw_len) {
        return const_string{};
    }

    const std::size_t end = find_substring(raw, raw_len, end_marker, end_len, start);
    if (end == ~std::size_t(0) || end <= start) {
        return const_string{};
    }

    return const_string(raw + start, raw + end);
}

// Remove MSVC decoration prefixes from a single name component
// "struct " -> "", "class " -> "", "enum " -> "", "union " -> ""
constexpr const_string strip_decoration(const_string name) noexcept {
    const_string result = name;

    // Repeat to handle cases like "const struct Foo"
    bool changed = true;
    while (changed && !result.empty()) {
        changed = false;

        // struct / class / union (6 chars each)
        if (result.size() >= 7) {
            const char prefixes[][8] = {"struct ", "class ", "union "};
            for (int i = 0; i < 3; ++i) {
                std::size_t len = const_strlen(prefixes[i]);
                bool match = true;
                for (std::size_t j = 0; j < len; ++j) {
                    if (result[j] != prefixes[i][j]) { match = false; break; }
                }
                if (match) {
                    result = const_string(result.begin() + len, result.end());
                    changed = true;
                    break;
                }
            }

            // "enum " (5 chars)
            if (!changed && result.size() >= 5) {
                const char enum_str[] = "enum ";
                bool match = true;
                for (std::size_t j = 0; j < 5; ++j) {
                    if (result[j] != enum_str[j]) { match = false; break; }
                }
                if (match) {
                    result = const_string(result.begin() + 5, result.end());
                    changed = true;
                }
            }
        }
    }

    return result;
}

// Extract type name from __FUNCSIG__ at runtime.
//
// MSVC __FUNCSIG__ format (example for type_name_impl<int>()):
//   "class uectti::detail::const_string __cdecl uectti::detail::type_name_impl<int>(void)"
//
// We search for "type_name_impl<" and extract the text between < and matching >,
// then strip MSVC decoration prefixes like "struct ", "class ", etc.
//
// NOTE: __FUNCSIG__ is not reliably evaluable in constexpr context on MSVC C++14.
template <typename T>
const_string type_name_impl() noexcept {
#if defined(_MSC_VER)
    return strip_decoration(
        extract_template_arg(__FUNCSIG__, "type_name_impl<")
    );
#elif defined(__clang__) || defined(__GNUC__)
    return strip_decoration(
        extract_between_markers(__PRETTY_FUNCTION__, "T = ", "]")
    );
#else
    return const_string{};
#endif
}

// One-shot hash shortcut
template <typename T>
std::uint64_t type_name_hash() noexcept {
    return type_name_impl<T>().hash();
}

} // namespace detail
} // namespace uectti

#endif // UECTTI_DETAIL_PARSER_HPP
