#pragma once

#include "Engine/Math/Vector3.hpp"

class AABB3 {
public:
    Vector3 mins = Vector3::Zero;
    Vector3 maxs = Vector3::Zero;

    static const AABB3 Zero_to_One;
    static const AABB3 Neg_One_to_One;

    // clang-format off
    AABB3() = default;
    AABB3(const AABB3& rhs) = default;
    AABB3(AABB3&& rhs) = default;
    AABB3& operator=(const AABB3& rhs) = default;
    AABB3& operator=(AABB3&& rhs) = default;
    ~AABB3() = default;
    AABB3(float initialX, float initialY, float initialZ) noexcept;
    AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) noexcept;
    AABB3(const Vector3& mins, const Vector3& maxs) noexcept;
    AABB3(const Vector3& center, float radiusX, float radiusY, float radiusZ) noexcept;
    // clang-format on

    void StretchToIncludePoint(const Vector3& point) noexcept;
    void AddPaddingToSides(float paddingX, float paddingY, float paddingZ) noexcept;
    void AddPaddingToSidesClamped(float paddingX, float paddingY, float paddingZ) noexcept;
    void Translate(const Vector3& translation) noexcept;

    [[nodiscard]] const Vector3 CalcDimensions() const noexcept;
    [[nodiscard]] const Vector3 CalcCenter() const noexcept;

    [[nodiscard]] AABB3 operator+(const Vector3& translation) const noexcept;
    [[nodiscard]] AABB3 operator-(const Vector3& antiTranslation) const noexcept;
    AABB3& operator+=(const Vector3& translation) noexcept;
    AABB3& operator-=(const Vector3& antiTranslation) noexcept;

protected:
private:
};