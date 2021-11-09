#pragma once

#include "Engine/Math/Vector2.hpp"

class AABB2;

class OBB2 {
public:
    Vector2 half_extents{};
    Vector2 position{};
    float orientationDegrees = 0.0f;
    // clang-format off
    OBB2() = default;
    OBB2(const OBB2& other) = default;
    OBB2(OBB2&& other) = default;
    OBB2& operator=(const OBB2& other) = default;
    OBB2& operator=(OBB2&& other) = default;
    OBB2(const Vector2& initialPosition, float initialOrientationDegrees) noexcept;
    OBB2(float initialX, float initialY, float initialOrientationDegrees) noexcept;
    OBB2(const Vector2& center, const Vector2& halfExtents, float orientationDegrees) noexcept;
    OBB2(const Vector2& center, float halfExtentX, float halfExtentY, float orientationDegrees) noexcept;
    explicit OBB2(const AABB2& aabb) noexcept;
    ~OBB2() = default;
    // clang-format on
    [[nodiscard]] AABB2 AsAABB2() const;

    void SetOrientationDegrees(float newOrientationDegrees) noexcept;
    void SetOrientation(float newOrientationRadians) noexcept;
    void RotateDegrees(float rotationDegrees) noexcept;
    void Rotate(float rotationRadians) noexcept;
    void StretchToIncludePoint(const Vector2& point) noexcept;
    void AddPaddingToSides(float paddingX, float paddingY) noexcept;
    void AddPaddingToSides(const Vector2& padding) noexcept;

    void AddPaddingToSidesClamped(float paddingX, float paddingY) noexcept;
    void AddPaddingToSidesClamped(const Vector2& padding) noexcept;
    void Translate(const Vector2& translation) noexcept;

    [[nodiscard]] Vector2 GetRight() const noexcept;
    [[nodiscard]] Vector2 GetUp() const noexcept;
    [[nodiscard]] Vector2 GetLeft() const noexcept;
    [[nodiscard]] Vector2 GetDown() const noexcept;

    [[nodiscard]] Vector2 GetRightEdge() const noexcept;
    [[nodiscard]] Vector2 GetTopEdge() const noexcept;
    [[nodiscard]] Vector2 GetLeftEdge() const noexcept;
    [[nodiscard]] Vector2 GetBottomEdge() const noexcept;

    [[nodiscard]] Vector2 GetBottomLeft() const noexcept;
    [[nodiscard]] Vector2 GetTopLeft() const noexcept;
    [[nodiscard]] Vector2 GetTopRight() const noexcept;
    [[nodiscard]] Vector2 GetBottomRight() const noexcept;

    [[nodiscard]] Vector2 CalcDimensions() const noexcept;
    [[nodiscard]] Vector2 CalcCenter() const noexcept;

    [[nodiscard]] OBB2 operator+(const Vector2& translation) const noexcept;
    [[nodiscard]] OBB2 operator-(const Vector2& antiTranslation) const noexcept;
    OBB2& operator+=(const Vector2& translation) noexcept;
    OBB2& operator-=(const Vector2& antiTranslation) noexcept;

protected:
private:
};
