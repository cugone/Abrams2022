#include "Engine/Math/Rotator.hpp"

#include <algorithm>

const Rotator Rotator::Zero = Rotator();

Rotator::Rotator(float scalar) noexcept
: pitch(scalar)
, yaw(scalar)
, roll(scalar) {
    /* DO NOTHING */
}

Rotator::Rotator(float pitch, float yaw, float roll) noexcept
: pitch(pitch)
, yaw(yaw)
, roll(roll) {
    /* DO NOTHING */
}

Rotator::Rotator(const Quaternion& q) noexcept {
    const auto&& [x, y, z] = q.CalcEulerAnglesDegrees();
    pitch = x;
    yaw = y;
    roll = z;
}

bool Rotator::operator==(const Rotator& rhs) noexcept {
    return pitch == rhs.pitch && roll == rhs.roll && yaw == rhs.yaw;
}

bool Rotator::operator!=(const Rotator& rhs) noexcept {
    return !(*this == rhs);
}

Rotator Rotator::operator*(float scalar) noexcept {
    return Rotator{pitch * scalar, yaw * scalar, roll * scalar};
}

Rotator& Rotator::operator*=(float scalar) noexcept {
    pitch *= scalar;
    yaw *= scalar;
    roll *= scalar;
    return *this;
}

Rotator Rotator::operator+(const Rotator& rhs) noexcept {
    return Rotator{pitch + rhs.pitch, yaw + rhs.yaw, roll + rhs.roll};
}

Rotator& Rotator::operator+=(const Rotator& rhs) noexcept {
    pitch += rhs.pitch;
    yaw += rhs.yaw;
    roll += rhs.roll;
    return *this;
}

void Rotator::Clamp() noexcept {
    pitch = std::clamp(pitch, 0.0f, 360.0f);
    yaw = std::clamp(yaw, 0.0f, 360.0f);
    roll = std::clamp(roll, 0.0f, 360.0f);
}

void Rotator::Clamp(Rotator& rotator) {
    rotator.Clamp();
}

Rotator Rotator::GetClamped() const noexcept {
    Rotator result{*this};
    result.Clamp();
    return result;
}

Rotator Rotator::GetClamped(const Rotator& rotator) noexcept {
    return rotator.GetClamped();
}

float Rotator::ClampAxis(float value) noexcept {
    return std::clamp(value, 0.0f, 360.0f);
}

Vector3 Rotator::GetEulerAngles() const noexcept {
    return Vector3{ClampAxis(roll), ClampAxis(pitch), ClampAxis(yaw)};
}

