#pragma once

#include <string>
#include <tuple>

class IntVector2;
class Vector2;
class Vector3;

class IntVector3 {
public:
    static const IntVector3 Zero;
    static const IntVector3 One;
    static const IntVector3 X_Axis;
    static const IntVector3 Y_Axis;
    static const IntVector3 Z_Axis;
    static const IntVector3 XY_Axis;
    static const IntVector3 XZ_Axis;
    static const IntVector3 YX_Axis;
    static const IntVector3 YZ_Axis;
    static const IntVector3 ZX_Axis;
    static const IntVector3 ZY_Axis;
    static const IntVector3 XYZ_Axis;

    IntVector3() = default;
    ~IntVector3() = default;

    IntVector3(const IntVector3& rhs) = default;
    IntVector3(IntVector3&& rhs) = default;

    explicit IntVector3(const IntVector2& iv2, int initialZ) noexcept;
    explicit IntVector3(const Vector2& v2, int initialZ) noexcept;
    explicit IntVector3(int initialX, int initialY, int initialZ) noexcept;
    explicit IntVector3(const Vector3& v3) noexcept;
    explicit IntVector3(const std::string& value) noexcept;

    IntVector3& operator=(const IntVector3& rhs) = default;
    IntVector3& operator=(IntVector3&& rhs) = default;

    [[nodiscard]] IntVector3 operator+(const IntVector3& rhs) const noexcept;
    IntVector3& operator+=(const IntVector3& rhs) noexcept;

    [[nodiscard]] IntVector3 operator-() const noexcept;
    [[nodiscard]] IntVector3 operator-(const IntVector3& rhs) const noexcept;
    IntVector3& operator-=(const IntVector3& rhs) noexcept;

    friend IntVector3 operator*(int lhs, const IntVector3& rhs) noexcept;
    [[nodiscard]] IntVector3 operator*(const IntVector3& rhs) const noexcept;
    IntVector3& operator*=(const IntVector3& rhs) noexcept;
    [[nodiscard]] IntVector3 operator*(int scalar) const noexcept;
    IntVector3& operator*=(int scalar) noexcept;
    [[nodiscard]] IntVector3 operator*(float scalar) const noexcept;
    IntVector3& operator*=(float scalar) noexcept;

    [[nodiscard]] IntVector3 operator/(const IntVector3& rhs) const noexcept;
    IntVector3& operator/=(const IntVector3& rhs) noexcept;
    [[nodiscard]] IntVector3 operator/(int scalar) const noexcept;
    IntVector3& operator/=(int scalar) noexcept;
    [[nodiscard]] IntVector3 operator/(float scalar) const noexcept;
    IntVector3& operator/=(float scalar) noexcept;

    [[nodiscard]] bool operator==(const IntVector3& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const IntVector3& rhs) const noexcept;
    [[nodiscard]] bool operator<(const IntVector3& rhs) const noexcept;
    [[nodiscard]] bool operator>=(const IntVector3& rhs) const noexcept;
    [[nodiscard]] bool operator>(const IntVector3& rhs) const noexcept;
    [[nodiscard]] bool operator<=(const IntVector3& rhs) const noexcept;

    friend std::ostream& operator<<(std::ostream& out_stream, const IntVector3& v) noexcept;
    friend std::istream& operator>>(std::istream& in_stream, IntVector3& v) noexcept;

    void SetXYZ(int newX, int newY, int newZ) noexcept;
    [[nodiscard]] std::tuple<int, int, int> GetXYZ() const noexcept;

    int x = 0;
    int y = 0;
    int z = 0;

protected:
private:
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const IntVector3& v) noexcept;
}
