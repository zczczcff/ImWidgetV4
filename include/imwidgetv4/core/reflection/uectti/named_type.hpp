#ifndef UECTTI_NAMED_TYPE_HPP
#define UECTTI_NAMED_TYPE_HPP

#include <type_traits>
#include <utility>
#include <functional>

namespace uectti {

template <typename Tag, typename T>
class named_type {
public:
    using tag_type   = Tag;
    using value_type = T;

    constexpr explicit named_type()
        noexcept(std::is_nothrow_default_constructible<T>::value)
        : m_value{} {}

    constexpr explicit named_type(const T& val)
        noexcept(std::is_nothrow_copy_constructible<T>::value)
        : m_value(val) {}

    constexpr explicit named_type(T&& val)
        noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_value(std::move(val)) {}

    constexpr T& get() noexcept { return m_value; }
    constexpr const T& get() const noexcept { return m_value; }

    explicit constexpr operator T() const noexcept { return m_value; }

    void swap(named_type& other)
        noexcept(std::is_nothrow_move_constructible<T>::value &&
                 std::is_nothrow_move_assignable<T>::value)
    {
        using std::swap;
        swap(m_value, other.m_value);
    }

    friend constexpr bool operator==(const named_type& a, const named_type& b) noexcept
        { return a.m_value == b.m_value; }
    friend constexpr bool operator!=(const named_type& a, const named_type& b) noexcept
        { return !(a == b); }
    friend constexpr bool operator<(const named_type& a, const named_type& b) noexcept
        { return a.m_value < b.m_value; }
    friend constexpr bool operator>(const named_type& a, const named_type& b) noexcept
        { return b < a; }
    friend constexpr bool operator<=(const named_type& a, const named_type& b) noexcept
        { return !(b < a); }
    friend constexpr bool operator>=(const named_type& a, const named_type& b) noexcept
        { return !(a < b); }

    struct hash {
        std::size_t operator()(const named_type& nt) const
            noexcept(noexcept(std::hash<T>()(std::declval<const T&>())))
        {
            return std::hash<T>()(nt.get());
        }
    };

private:
    T m_value;
};

} // namespace uectti

#endif // UECTTI_NAMED_TYPE_HPP
