#ifndef UECTTI_TYPE_INDEX_HPP
#define UECTTI_TYPE_INDEX_HPP

#include <cstddef>
#include <atomic>

namespace uectti {
namespace detail {

inline std::size_t next_type_index() noexcept {
    static std::atomic<std::size_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace detail

template <typename T>
std::size_t type_index() noexcept {
    static const std::size_t idx = detail::next_type_index();
    return idx;
}

} // namespace uectti

#endif // UECTTI_TYPE_INDEX_HPP
