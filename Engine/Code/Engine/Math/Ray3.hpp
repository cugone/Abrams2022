#pragma once

#include "Engine/Math/Vector3.hpp"

class Ray3 {
public:
    Vector3 position;
    Vector3 direction;

    void SetDirection(const Vector3& newDirection) noexcept;
    [[nodiscard]] Vector3 Interpolate(float t);

protected:
private:
};
