#pragma once

#include "Engine/Math/Vector3.hpp"

class LineSegment3 {
public:
    Vector3 start = Vector3::Zero;
    Vector3 end = Vector3::Zero;

    static const LineSegment3 Unit_Horizontal;
    static const LineSegment3 Unit_Vertical;
    static const LineSegment3 Unit_Depth;
    static const LineSegment3 Unit_Centered_Horizontal;
    static const LineSegment3 Unit_Centered_Vertical;
    static const LineSegment3 Unit_Centered_Depth;

    LineSegment3() = default;
    LineSegment3(const LineSegment3& rhs) = default;
    LineSegment3(LineSegment3&& rhs) = default;
    ~LineSegment3() = default;
    LineSegment3& operator=(LineSegment3&& rhs) = default;
    LineSegment3& operator=(const LineSegment3& rhs) = default;

    explicit LineSegment3(float startX, float startY, float startZ, float endX, float endY, float endZ) noexcept;
    explicit LineSegment3(const Vector3& startPosition, const Vector3& endPosition) noexcept;
    explicit LineSegment3(const Vector3& startPosition, const Vector3& direction, float length) noexcept;

    void SetLengthFromStart(float length) noexcept;
    void SetLengthFromCenter(float length) noexcept;
    void SetLengthFromEnd(float length) noexcept;

    [[nodiscard]] Vector3 CalcCenter() const noexcept;

    [[nodiscard]] float CalcLength() const noexcept;
    [[nodiscard]] float CalcLengthSquared() const noexcept;

    void SetStartEndPositions(const Vector3& startPosition, const Vector3& endPosition) noexcept;

    void Translate(const Vector3& translation) noexcept;

    [[nodiscard]] Vector3 CalcDisplacement() const noexcept;
    [[nodiscard]] Vector3 CalcDirection() const noexcept;

    [[nodiscard]] LineSegment3 operator+(const Vector3& translation) const noexcept;
    [[nodiscard]] LineSegment3 operator-(const Vector3& antiTranslation) const noexcept;
    LineSegment3& operator+=(const Vector3& translation) noexcept;
    LineSegment3& operator-=(const Vector3& antiTranslation) noexcept;

protected:
private:
    friend class Capsule3;
};