#ifndef UECTTI_TYPE_ID_CONSTANT_HPP
#define UECTTI_TYPE_ID_CONSTANT_HPP

#include "type_id.hpp"

namespace uectti {

// strips const/volatile/reference qualifiers
template <typename T>
struct type_id_constant {
    using type         = T;
    using value_type   = type_id_t;

    type_id_t value() const noexcept { return type_id<T>(); }
    operator value_type() const noexcept { return value(); }
    value_type operator()() const noexcept { return value(); }
};

// preserves const/volatile/reference qualifiers
template <typename T>
struct type_id_constant_with_cvr {
    using type         = T;
    using value_type   = type_id_t;

    type_id_t value() const noexcept { return type_id_with_cvr<T>(); }
    operator value_type() const noexcept { return value(); }
    value_type operator()() const noexcept { return value(); }
};

} // namespace uectti

#endif // UECTTI_TYPE_ID_CONSTANT_HPP
