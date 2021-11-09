#pragma once

#include "Engine/Math/Quaternion.hpp"

class Rotator {
public:
    float pitch{0.0f};
    float roll{0.0f};
    float yaw{0.0f};

    Rotator() noexcept = default;
    Rotator(const Rotator& other) noexcept = default;
    Rotator(Rotator&& other) noexcept = default;
    explicit Rotator(float scalar) noexcept;
    explicit Rotator(const Quaternion& q) noexcept;
    Rotator(float pitch, float yaw, float roll) noexcept;
    Rotator& operator=(const Rotator& rhs) = default;
    Rotator& operator=(Rotator&& rhs) = default;

    bool operator==(const Rotator& rhs) noexcept;
    bool operator!=(const Rotator& rhs) noexcept;

    Rotator operator*(float scalar) noexcept;
    Rotator& operator*=(float scalar) noexcept;

    Rotator operator+(const Rotator& rhs) noexcept;
    Rotator& operator+=(const Rotator& rhs) noexcept;

    static const Rotator Zero;

    static void Clamp(Rotator& rotator);
    static Rotator GetClamped(const Rotator& rotator) noexcept;
    void Clamp() noexcept;
    Rotator GetClamped() const noexcept;
    static float ClampAxis(float value) noexcept;

    Vector3 GetEulerAngles() const noexcept;

protected:
private:
};
