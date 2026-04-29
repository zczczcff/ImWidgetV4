#ifndef UECTTI_TYPE_ID_HPP
#define UECTTI_TYPE_ID_HPP

#include "detail/parser.hpp"
#include <cstdint>
#include <type_traits>

namespace uectti {

using type_id_t = std::uint64_t;

namespace detail {

template <typename T>
using type_clean_t = typename std::remove_cv<
    typename std::remove_reference<T>::type
>::type;

} // namespace detail

// strips const/volatile/reference qualifiers
template <typename T>
type_id_t type_id() noexcept {
    static const type_id_t cached = detail::type_name_hash<detail::type_clean_t<T>>();
    return cached;
}

// preserves const/volatile/reference qualifiers
template <typename T>
type_id_t type_id_with_cvr() noexcept {
    static const type_id_t cached = detail::type_name_hash<T>();
    return cached;
}

struct type_id_equal {
    constexpr bool operator()(type_id_t a, type_id_t b) const noexcept {
        return a == b;
    }
};

struct type_id_less {
    constexpr bool operator()(type_id_t a, type_id_t b) const noexcept {
        return a < b;
    }
};

} // namespace uectti

#endif // UECTTI_TYPE_ID_HPP
