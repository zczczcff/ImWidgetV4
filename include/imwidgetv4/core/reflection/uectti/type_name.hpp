#ifndef UECTTI_TYPE_NAME_HPP
#define UECTTI_TYPE_NAME_HPP

#include "detail/parser.hpp"
#include <type_traits>

namespace uectti {

namespace detail {

template <typename T>
using type_clean_t = typename std::remove_cv<
    typename std::remove_reference<T>::type
>::type;

} // namespace detail

// strips const/volatile/reference qualifiers
template <typename T>
detail::const_string type_name() noexcept {
    return detail::type_name_impl<detail::type_clean_t<T>>();
}

// preserves const/volatile/reference qualifiers
template <typename T>
detail::const_string type_name_with_cvr() noexcept {
    return detail::type_name_impl<T>();
}

} // namespace uectti

#endif // UECTTI_TYPE_NAME_HPP
