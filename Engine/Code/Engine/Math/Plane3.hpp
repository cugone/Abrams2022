#pragma once

#include "Engine/Math/Vector3.hpp"

class Plane3 {
public:
    Vector3 normal = Vector3::Z_Axis;
    float dist = 0.0f;

    Plane3() = default;
    Plane3(const Plane3& rhs) = default;
    Plane3(Plane3&& rhs) = default;
    Plane3& operator=(const Plane3& rhs) = default;
    Plane3& operator=(Plane3&& rhs) = default;
    explicit Plane3(const Vector3& normal, float distance_from_origin) noexcept;
    ~Plane3() = default;

    float Normalize() noexcept;
    [[nodiscard]] Plane3 GetNormalize() const noexcept;

protected:
private:
};