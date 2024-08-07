#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace TypeUtils {

template<class T, class... Types>
inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

template<class T, class...Types>
concept is_any_of = is_any_of_v<T, Types...>;

template<class T, class...Types>
concept is_not_any_of = !is_any_of_v<T, Types...>;

template<typename E>
struct is_bitflag_enum_type : std::false_type {};

template<typename E>
struct is_incrementable_enum_type : std::false_type {};

template<typename E>
struct is_decrementable_enum_type : std::false_type {};

template<typename E>
struct is_dereferenceable_enum_type : std::false_type {};

template<typename E>
using is_scoped_enum = std::integral_constant<bool, std::is_enum_v<E> && !std::is_convertible_v<E, int>>;

template<typename E>
constexpr bool is_scoped_enum_v = is_scoped_enum<E>::value;

template<typename E>
concept scoped_enum = is_scoped_enum_v<E>;

template<typename E>
requires scoped_enum<E>
[[nodiscard]] const auto GetUnderlyingValue(E a) {
    using underlying = std::underlying_type_t<E>;
    return static_cast<underlying>(a);
}

namespace detail {

template<typename E>
constexpr bool is_bitflag_enum_type_v = is_bitflag_enum_type<E>::value;

template<typename E>
using is_bitflag_enum = std::integral_constant<bool, is_scoped_enum_v<E> && is_bitflag_enum_type_v<E>>;

template<typename E>
constexpr bool is_bitflag_enum_v = is_bitflag_enum<E>::value;

template<typename E>
concept bitflag_enum = is_bitflag_enum_v<E>;

template<typename E>
constexpr bool is_incrementable_enum_type_v = is_incrementable_enum_type<E>::value;

template<typename E>
using is_incrementable_enum = std::integral_constant<bool, is_scoped_enum_v<E> && is_incrementable_enum_type_v<E>>;

template<typename E>
constexpr bool is_incrementable_enum_v = is_incrementable_enum<E>::value;

template<typename E>
concept incrementable_enum = is_incrementable_enum_v<E>;

template<typename E>
constexpr bool is_decrementable_enum_type_v = is_decrementable_enum_type<E>::value;

template<typename E>
using is_decrementable_enum = std::integral_constant<bool, is_scoped_enum_v<E> && is_decrementable_enum_type_v<E>>;

template<typename E>
constexpr bool is_decrementable_enum_v = is_decrementable_enum<E>::value;

template<typename E>
concept decrementable_enum = is_decrementable_enum_v<E>;

template<typename E>
constexpr bool is_dereferenceable_enum_type_v = is_dereferenceable_enum_type<E>::value;

template<typename E>
using is_dereferenceable_enum = std::integral_constant<bool, is_scoped_enum_v<E> && is_dereferenceable_enum_type_v<E>>;

template<typename E>
constexpr bool is_dereferenceable_enum_v = is_dereferenceable_enum<E>::value;

template<typename E>
concept dereferenceable_enum = is_dereferenceable_enum_v<E>;

template<typename E>
requires scoped_enum<E>
[[nodiscard]] const auto GetUnderlyingValues(E a, E b) {
    using underlying = std::underlying_type_t<E>;
    return std::make_pair(static_cast<underlying>(a), static_cast<underlying>(b));
}
} // namespace detail

} // namespace TypeUtils

/************************************************************************/
/* BITFLAGS                                                             */
/************************************************************************/

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
bool operator!(E a) noexcept {
    using underlying = std::underlying_type_t<E>;
    return TypeUtils::GetUnderlyingValue(a) == 0;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E& operator|=(E& a, const E& b) noexcept {
    const auto [underlying_a, underlying_b] = TypeUtils::detail::GetUnderlyingValues(a, b);
    a = static_cast<E>(underlying_a | underlying_b);
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E operator|(E a, const E& b) noexcept {
    a |= b;
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E& operator&=(E& a, const E& b) noexcept {
    const auto [underlying_a, underlying_b] = TypeUtils::detail::GetUnderlyingValues(a, b);
    a = static_cast<E>(underlying_a & underlying_b);
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E operator&(E a, const E& b) noexcept {
    a &= b;
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E& operator^=(E& a, const E& b) noexcept {
    const auto [underlying_a, underlying_b] = TypeUtils::detail::GetUnderlyingValues(a, b);
    a = static_cast<E>(underlying_a ^ underlying_b);
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E operator^(E a, const E& b) noexcept {
    a ^= b;
    return a;
}

template<typename E>
requires TypeUtils::detail::bitflag_enum<E>
E operator~(E a) noexcept {
    const auto underlying_a = TypeUtils::GetUnderlyingValue(a);
    a = static_cast<E>(~underlying_a);
    return a;
}

/************************************************************************/
/* INCREMENTABLE                                                        */
/************************************************************************/

template<typename E>
requires TypeUtils::detail::incrementable_enum<E>
E& operator++(E& e) noexcept {
    using underlying = std::underlying_type_t<E>;
    e = static_cast<E>(static_cast<underlying>(e) + 1);
    return e;
}

template<typename E>
requires TypeUtils::detail::incrementable_enum<E>
E operator++(E& e, int) noexcept {
    E result = e;
    ++e;
    return result;
}

/************************************************************************/
/* DECREMENTABLE                                                        */
/************************************************************************/

template<typename E>
requires TypeUtils::detail::decrementable_enum<E>
E& operator--(E& e) noexcept {
    using underlying = std::underlying_type_t<E>;
    e = static_cast<E>(static_cast<underlying>(e) - 1);
    return e;
}

template<typename E>
requires TypeUtils::detail::decrementable_enum<E>
E operator--(E& e, int) noexcept {
    E result = e;
    --e;
    return result;
}

/************************************************************************/
/* DEREFERENCEABLE                                                      */
/************************************************************************/

template<typename E>
requires TypeUtils::detail::dereferenceable_enum<E>
auto operator*(E e) noexcept -> std::underlying_type_t<E>& {
    return TypeUtils::GetUnderlyingValue(e);
}
