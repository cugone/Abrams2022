#pragma once

#include "Engine/Math/Vector2.hpp"

class Plane2 {
public:
    Vector2 normal = Vector2::X_Axis;
    float dist = 0.0f;

    Plane2() = default;
    Plane2(const Plane2& rhs) = default;
    Plane2(Plane2&& rhs) = default;
    Plane2& operator=(const Plane2& rhs) = default;
    Plane2& operator=(Plane2&& rhs) = default;
    explicit Plane2(const Vector2& normal, float distance_from_origin) noexcept;
    ~Plane2() = default;

    float Normalize() noexcept;
    [[nodiscard]] Plane2 GetNormalize() const noexcept;

protected:
private:
};