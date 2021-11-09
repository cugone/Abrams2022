#pragma once

#include <string>
#include <tuple>

class Vector2;
class Vector3;
class IntVector4;

class Vector4 {
public:
    static const Vector4 Zero;
    static const Vector4 One;
    static const Vector4 Zero_XYZ_One_W;
    static const Vector4 One_XYZ_Zero_W;
    static const Vector4 X_Axis;
    static const Vector4 XY_Axis;
    static const Vector4 XZ_Axis;
    static const Vector4 XW_Axis;
    static const Vector4 Y_Axis;
    static const Vector4 YX_Axis;
    static const Vector4 YZ_Axis;
    static const Vector4 YW_Axis;
    static const Vector4 Z_Axis;
    static const Vector4 ZX_Axis;
    static const Vector4 ZY_Axis;
    static const Vector4 ZW_Axis;
    static const Vector4 W_Axis;
    static const Vector4 WX_Axis;
    static const Vector4 WY_Axis;
    static const Vector4 WZ_Axis;
    static const Vector4 XYZ_Axis;
    static const Vector4 YZW_Axis;
    static const Vector4 XZW_Axis;
    static const Vector4 XYW_Axis;

    [[nodiscard]] static Vector4 CalcHomogeneous(const Vector4& v) noexcept;

    Vector4() noexcept = default;
    Vector4(const Vector4& other) noexcept = default;
    Vector4(Vector4&& other) noexcept = default;

    Vector4& operator=(const Vector4& rhs) noexcept = default;
    Vector4& operator=(Vector4&& rhs) noexcept = default;

    ~Vector4() noexcept = default;

    explicit Vector4(const std::string& value) noexcept;
    explicit Vector4(const IntVector4& intvec4) noexcept;
    explicit Vector4(const Vector3& xyz, float initialW) noexcept;
    explicit Vector4(const Vector2& xy, float initialZ, float initialW) noexcept;
    explicit Vector4(const Vector2& xy, const Vector2& zw) noexcept;
    explicit Vector4(float initialX, float initialY, float initialZ, float initialW) noexcept;

    [[nodiscard]] bool operator==(const Vector4& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Vector4& rhs) const noexcept;

    [[nodiscard]] Vector4 operator+(const Vector4& rhs) const noexcept;
    [[nodiscard]] Vector4 operator-(const Vector4& rhs) const noexcept;
    [[nodiscard]] Vector4 operator*(const Vector4& rhs) const noexcept;
    [[nodiscard]] Vector4 operator*(float scale) const noexcept;
    [[nodiscard]] Vector4 operator/(const Vector4 rhs) const noexcept;
    [[nodiscard]] Vector4 operator/(float inv_scale) const noexcept;

    friend Vector4 operator*(float lhs, const Vector4& rhs) noexcept;
    Vector4& operator*=(float scale) noexcept;
    Vector4& operator*=(const Vector4& rhs) noexcept;
    Vector4& operator/=(const Vector4& rhs) noexcept;
    Vector4& operator+=(const Vector4& rhs) noexcept;
    Vector4& operator-=(const Vector4& rhs) noexcept;

    [[nodiscard]] Vector4 operator-() const noexcept;

    friend std::ostream& operator<<(std::ostream& out_stream, const Vector4& v) noexcept;
    friend std::istream& operator>>(std::istream& in_stream, Vector4& v) noexcept;

    [[nodiscard]] Vector2 GetXY() const noexcept;
    [[nodiscard]] Vector2 GetZW() const noexcept;

    std::tuple<float, float, float> GetXYZ() const noexcept;
    std::tuple<float, float, float, float> GetXYZW() const noexcept;

    void SetXYZ(float newX, float newY, float newZ) noexcept;
    void SetXYZW(float newX, float newY, float newZ, float newW) noexcept;

    [[nodiscard]] const float* GetAsFloatArray() const noexcept;
    [[nodiscard]] float* GetAsFloatArray() noexcept;

    [[nodiscard]] float CalcLength3D() const noexcept;
    [[nodiscard]] float CalcLength3DSquared() const noexcept;
    [[nodiscard]] float CalcLength4D() const noexcept;
    [[nodiscard]] float CalcLength4DSquared() const noexcept;
    void CalcHomogeneous() noexcept;

    float Normalize4D() noexcept;
    float Normalize3D() noexcept;

    [[nodiscard]] Vector4 GetNormalize4D() const noexcept;
    [[nodiscard]] Vector4 GetNormalize3D() const noexcept;

    friend void swap(Vector4& a, Vector4& b) noexcept;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

protected:
private:
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const Vector4& v) noexcept;
}
