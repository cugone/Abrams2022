#pragma once

#include <string>
#include <tuple>

class IntVector2;
class IntVector3;
class Vector2;
class Vector3;
class Vector4;

class IntVector4 {
public:
    static const IntVector4 Zero;
    static const IntVector4 One;
    static const IntVector4 X_Axis;
    static const IntVector4 Y_Axis;
    static const IntVector4 Z_Axis;
    static const IntVector4 W_Axis;
    static const IntVector4 XY_Axis;
    static const IntVector4 XZ_Axis;
    static const IntVector4 XW_Axis;
    static const IntVector4 YX_Axis;
    static const IntVector4 YZ_Axis;
    static const IntVector4 YW_Axis;
    static const IntVector4 ZX_Axis;
    static const IntVector4 ZY_Axis;
    static const IntVector4 ZW_Axis;
    static const IntVector4 WX_Axis;
    static const IntVector4 WY_Axis;
    static const IntVector4 WZ_Axis;
    static const IntVector4 XYZ_Axis;
    static const IntVector4 XYW_Axis;
    static const IntVector4 YXZ_Axis;
    static const IntVector4 YZW_Axis;
    static const IntVector4 WXY_Axis;
    static const IntVector4 WXZ_Axis;
    static const IntVector4 WYZ_Axis;
    static const IntVector4 XYZW_Axis;

    IntVector4() = default;
    ~IntVector4() = default;

    IntVector4(const IntVector4& rhs) = default;
    IntVector4(IntVector4&& rhs) = default;

    explicit IntVector4(const IntVector2& iv2, int initialZ, int initialW) noexcept;
    explicit IntVector4(const Vector2& v2, int initialZ, int initialW) noexcept;
    explicit IntVector4(const Vector2& xy, const Vector2& zw) noexcept;
    explicit IntVector4(const IntVector2& xy, const IntVector2& zw) noexcept;
    explicit IntVector4(int initialX, int initialY, int initialZ, int initialW) noexcept;
    explicit IntVector4(const IntVector3& iv3, int initialW) noexcept;
    explicit IntVector4(const Vector3& v3, int initialW) noexcept;
    explicit IntVector4(const Vector4& rhs) noexcept;
    explicit IntVector4(const std::string& value) noexcept;

    IntVector4& operator=(const IntVector4& rhs) = default;
    IntVector4& operator=(IntVector4&& rhs) = default;

    [[nodiscard]] bool operator==(const IntVector4& rhs) noexcept;
    [[nodiscard]] bool operator!=(const IntVector4& rhs) noexcept;

    [[nodiscard]] IntVector2 GetXY() const noexcept;
    [[nodiscard]] IntVector2 GetZW() const noexcept;

    void SetXYZW(int newX, int newY, int newZ, int newW) noexcept;
    [[nodiscard]] std::tuple<int, int, int, int> GetXYZW() const noexcept;

    int x = 0;
    int y = 0;
    int z = 0;
    int w = 0;

protected:
private:
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const IntVector4& v) noexcept;
}
