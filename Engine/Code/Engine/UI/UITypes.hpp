#pragma once

#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vector2.hpp"


// clang-format off
enum class UIFillMode {
    Fill
    , Fit
    , Stretch
    , Tile
    , Center
    , Span
    , Crop = Fill
};

enum class UIPivotPosition {
    First_
    , TopLeft = First_
    , Top
    , TopRight
    , Left
    , Center
    , Right
    , BottomLeft
    , Bottom
    , BottomRight
    , Last_
};
// clang-format on

template<>
struct TypeUtils::is_incrementable_enum_type<UIPivotPosition> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<UIPivotPosition> : std::true_type {};

// clang-format off
enum class UIInvalidateElementReason : uint8_t {
    None = 0
    , Layout = 1 << 0
    , Order = 1 << 1
    , Any = Layout | Order
};
// clang-format on

template<>
struct TypeUtils::is_bitflag_enum_type<UIInvalidateElementReason> : std::true_type {};


