#ifndef UECTTI_TYPE_HASH_HPP
#define UECTTI_TYPE_HASH_HPP

#include "type_id.hpp"
#include <cstddef>

namespace uectti {

// strips const/volatile/reference qualifiers
template <typename T>
struct type_hash {
    using argument_type = T;
    using result_type   = std::size_t;

    result_type operator()(const T&) const noexcept {
        return static_cast<result_type>(type_id<T>());
    }

    result_type operator()(type_id_t id) const noexcept {
        return static_cast<result_type>(id);
    }
};

// preserves const/volatile/reference qualifiers
template <typename T>
struct type_hash_with_cvr {
    using argument_type = T;
    using result_type   = std::size_t;

    result_type operator()(const T&) const noexcept {
        return static_cast<result_type>(type_id_with_cvr<T>());
    }

    result_type operator()(type_id_t id) const noexcept {
        return static_cast<result_type>(id);
    }
};

// void specialization: void cannot form const void&
template <>
struct type_hash<void> {
    using argument_type = void;
    using result_type   = std::size_t;

    result_type operator()(type_id_t id) const noexcept {
        return static_cast<result_type>(id);
    }
};

template <>
struct type_hash_with_cvr<void> {
    using argument_type = void;
    using result_type   = std::size_t;

    result_type operator()(type_id_t id) const noexcept {
        return static_cast<result_type>(id);
    }
};

} // namespace uectti

#endif // UECTTI_TYPE_HASH_HPP
