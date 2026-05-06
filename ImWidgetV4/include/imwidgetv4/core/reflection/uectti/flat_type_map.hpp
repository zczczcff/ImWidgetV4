#ifndef UECTTI_FLAT_TYPE_MAP_HPP
#define UECTTI_FLAT_TYPE_MAP_HPP

#include "type_id.hpp"
#include <cstddef>

namespace uectti {

template <typename TKey, typename TValue, std::size_t N>
class flat_type_map {
    struct entry {
        type_id_t type_id;
        TValue    value;
    };

    entry      m_entries[N];
    std::size_t m_size;

public:
    using key_type    = TKey;
    using mapped_type = TValue;
    using size_type   = std::size_t;

    flat_type_map() noexcept : m_entries{}, m_size(0) {}

    template <typename T>
    TValue& get() noexcept {
        const auto id = type_id<T>();
        for (size_type i = 0; i < m_size; ++i) {
            if (m_entries[i].type_id == id) return m_entries[i].value;
        }
        if (m_size < N) {
            m_entries[m_size].type_id = id;
            m_entries[m_size].value   = TValue{};
            ++m_size;
        }
        return m_entries[m_size > 0 ? m_size - 1 : 0].value;
    }

    template <typename T>
    const TValue& get() const noexcept {
        const auto id = type_id<T>();
        for (size_type i = 0; i < m_size; ++i) {
            if (m_entries[i].type_id == id) return m_entries[i].value;
        }
        return m_entries[0].value;
    }

    template <typename T>
    bool contains() const noexcept {
        const auto id = type_id<T>();
        for (size_type i = 0; i < m_size; ++i) {
            if (m_entries[i].type_id == id) return true;
        }
        return false;
    }

    std::size_t size()  const noexcept { return m_size; }
    bool        empty() const noexcept { return m_size == 0; }
    bool        full()  const noexcept { return m_size == N; }

    void clear() noexcept { m_size = 0; }
};

} // namespace uectti

#endif // UECTTI_FLAT_TYPE_MAP_HPP
