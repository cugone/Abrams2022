#include "Engine/Math/Ray3.hpp"

void Ray3::SetDirection(const Vector3& newDirection) noexcept {
    direction = newDirection.GetNormalize();
}

Vector3 Ray3::Interpolate(float t) const noexcept {
    return position + t * direction;
}
