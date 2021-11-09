#include "Engine/Math/Plane3.hpp"

Plane3::Plane3(const Vector3& normal, float distance_from_origin) noexcept
: normal(normal)
, dist(distance_from_origin) {
    /* DO NOTHING */
}

float Plane3::Normalize() noexcept {
    const auto length = normal.CalcLength();
    if(length > 0.0f) {
        const auto inv_length = 1.0f / length;
        normal.x *= inv_length;
        normal.y *= inv_length;
        normal.z *= inv_length;
        dist *= inv_length;
    }
    return length;
}

Plane3 Plane3::GetNormalize() const noexcept {
    Plane3 result(*this);
    result.Normalize();
    return result;
}
