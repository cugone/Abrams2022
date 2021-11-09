#pragma once

#include "Engine/Math/Vector3.hpp"

#include <string>

class Matrix4;

class Quaternion {
public:
    static Quaternion I;
    float w = 1.0f;
    Vector3 axis = Vector3::Zero;

    [[nodiscard]] static Quaternion CreateRealQuaternion(float scalar) noexcept;
    [[nodiscard]] static Quaternion CreatePureQuaternion(const Vector3& v) noexcept;
    [[nodiscard]] static Quaternion CreateFromAxisAngle(const Vector3& axis, float degreesAngle) noexcept;
    [[nodiscard]] static Quaternion CreateFromEulerAnglesDegrees(float pitch, float yaw, float roll) noexcept;
    [[nodiscard]] static Quaternion CreateFromEulerAnglesRadians(float pitch, float yaw, float roll) noexcept;
    [[nodiscard]] static Quaternion CreateFromEulerAngles(float pitch, float yaw, float roll, bool degrees) noexcept;

    Quaternion() = default;
    explicit Quaternion(const std::string& value) noexcept;
    explicit Quaternion(const Matrix4& mat) noexcept;
    explicit Quaternion(const Vector3& rotations) noexcept;
    Quaternion(const Quaternion& other) = default;
    Quaternion& operator=(const Quaternion& rhs) = default;
    ~Quaternion() = default;

    explicit Quaternion(float initialScalar, const Vector3& initialAxis) noexcept;
    explicit Quaternion(float initialW, float initialX, float initialY, float initialZ) noexcept;

    [[nodiscard]] Quaternion operator+(const Quaternion& rhs) const noexcept;
    Quaternion& operator+=(const Quaternion& rhs) noexcept;

    [[nodiscard]] Quaternion operator-(const Quaternion& rhs) const noexcept;
    Quaternion& operator-=(const Quaternion& rhs) noexcept;

    [[nodiscard]] Quaternion operator*(const Quaternion& rhs) const noexcept;
    Quaternion& operator*=(const Quaternion& rhs) noexcept;

    [[nodiscard]] Quaternion operator*(const Vector3& rhs) const noexcept;
    Quaternion& operator*=(const Vector3& rhs) noexcept;

    [[nodiscard]] Quaternion operator*(float scalar) const noexcept;
    Quaternion& operator*=(float scalar) noexcept;

    [[nodiscard]] Quaternion operator-() noexcept;

    [[nodiscard]] bool operator==(const Quaternion& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Quaternion& rhs) const noexcept;

    [[nodiscard]] Vector4 CalcAxisAnglesDegrees() const noexcept;
    [[nodiscard]] Vector4 CalcAxisAnglesRadians() const noexcept;
    [[nodiscard]] Vector4 CalcAxisAngles(bool degrees) const noexcept;
    [[nodiscard]] Vector3 CalcEulerAnglesDegrees() const noexcept;
    [[nodiscard]] Vector3 CalcEulerAnglesRadians() const noexcept;
    [[nodiscard]] Vector3 CalcEulerAngles(bool degrees) const noexcept;

    [[nodiscard]] float CalcLength() const noexcept;
    [[nodiscard]] float CalcLengthSquared() const noexcept;
    [[nodiscard]] Quaternion CalcInverse() const noexcept;

    void Normalize() noexcept;
    [[nodiscard]] Quaternion GetNormalize() const noexcept;

    void Conjugate() noexcept;
    [[nodiscard]] Quaternion GetConjugate() const noexcept;

    void Inverse() noexcept;

protected:
private:
};

[[nodiscard]] Quaternion operator*(float scalar, const Quaternion& rhs) noexcept;
Quaternion& operator*=(float scalar, Quaternion& rhs) noexcept;

[[nodiscard]] Quaternion operator*(const Vector3& lhs, const Quaternion& rhs) noexcept;
Quaternion& operator*=(const Vector3& lhs, Quaternion& rhs) noexcept;

[[nodiscard]] Quaternion Conjugate(const Quaternion& q) noexcept;
[[nodiscard]] Quaternion Inverse(const Quaternion& q) noexcept;
