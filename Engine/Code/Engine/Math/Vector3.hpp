#pragma once

#include <string>

class Vector2;
class IntVector3;
class Vector4;
class Quaternion;

class Vector3 {
public:
    static const Vector3 One;
    static const Vector3 Zero;
    static const Vector3 X_Axis;
    static const Vector3 Y_Axis;
    static const Vector3 Z_Axis;
    static const Vector3 XY_Axis;
    static const Vector3 XZ_Axis;
    static const Vector3 YZ_Axis;

    Vector3() noexcept = default;
    Vector3(const Vector3& other) noexcept = default;
    Vector3(Vector3&& other) noexcept = default;

    Vector3& operator=(const Vector3& rhs) noexcept = default;
    Vector3& operator=(Vector3&& rhs) noexcept = default;

    ~Vector3() noexcept = default;

    explicit Vector3(const std::string& value) noexcept;
    explicit Vector3(float initialX, float initialY, float initialZ) noexcept;
    explicit Vector3(const Vector2& vec2) noexcept;
    explicit Vector3(const IntVector3& intvec3) noexcept;
    explicit Vector3(const Vector2& xy, float initialZ) noexcept;
    explicit Vector3(const Vector4& vec4) noexcept;
    explicit Vector3(const Quaternion& q) noexcept;

    [[nodiscard]] Vector3 operator+(const Vector3& rhs) const noexcept;
    Vector3& operator+=(const Vector3& rhs) noexcept;

    [[nodiscard]] Vector3 operator-() const noexcept;
    [[nodiscard]] Vector3 operator-(const Vector3& rhs) const noexcept;
    Vector3& operator-=(const Vector3& rhs) noexcept;

    friend Vector3 operator*(float lhs, const Vector3& rhs) noexcept;
    [[nodiscard]] Vector3 operator*(float scalar) const noexcept;
    Vector3& operator*=(float scalar) noexcept;
    [[nodiscard]] Vector3 operator*(const Vector3& rhs) const noexcept;
    Vector3& operator*=(const Vector3& rhs) noexcept;

    friend Vector3 operator/(float lhs, const Vector3& v) noexcept;
    [[nodiscard]] Vector3 operator/(float scalar) const noexcept;
    Vector3 operator/=(float scalar) noexcept;
    [[nodiscard]] Vector3 operator/(const Vector3& rhs) const noexcept;
    Vector3 operator/=(const Vector3& rhs) noexcept;

    [[nodiscard]] bool operator==(const Vector3& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Vector3& rhs) const noexcept;

    friend std::ostream& operator<<(std::ostream& out_stream, const Vector3& v) noexcept;
    friend std::istream& operator>>(std::istream& in_stream, Vector3& v) noexcept;

    [[nodiscard]] Vector2 GetXY() const noexcept;
    [[nodiscard]] Vector3 GetXYZ() const noexcept;
    [[nodiscard]] const float* GetAsFloatArray() const noexcept;
    [[nodiscard]] float* GetAsFloatArray() noexcept;

    [[nodiscard]] float CalcLength() const noexcept;
    [[nodiscard]] float CalcLengthSquared() const noexcept;

    float Normalize() noexcept;
    [[nodiscard]] Vector3 GetNormalize() const noexcept;

    void SetXYZ(float newX, float newY, float newZ) noexcept;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    friend void swap(Vector3& a, Vector3& b) noexcept;

protected:
private:
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const Vector3& v) noexcept;
}
