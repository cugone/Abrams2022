#pragma once

#include "Engine/Math/LineSegment3.hpp"
#include "Engine/Math/Vector3.hpp"

class Capsule3 {
public:
    static const Capsule3 Unit_Horizontal;
    static const Capsule3 Unit_Vertical;
    static const Capsule3 Unit_Depth;
    static const Capsule3 Unit_Centered_Horizontal;
    static const Capsule3 Unit_Centered_Vertical;
    static const Capsule3 Unit_Centered_Depth;

    LineSegment3 line{};
    float radius = 0.0f;

    Capsule3() = default;
    Capsule3(const Capsule3& rhs) = default;
    Capsule3(Capsule3&& rhs) = default;
    Capsule3& operator=(const Capsule3& rhs) = default;
    Capsule3& operator=(Capsule3&& rhs) = default;
    ~Capsule3() = default;

    explicit Capsule3(const LineSegment3& line, float radius) noexcept;
    explicit Capsule3(float startX, float startY, float startZ, float endX, float endY, float endZ, float radius) noexcept;
    explicit Capsule3(const Vector3& start_position, const Vector3& end_position, float radius) noexcept;
    explicit Capsule3(const Vector3& start_position, const Vector3& direction, float length, float radius) noexcept;

    void SetLengthFromStart(float length) noexcept;
    void SetLengthFromCenter(float length) noexcept;
    void SetLengthFromEnd(float length) noexcept;

    [[nodiscard]] Vector3 CalcCenter() const noexcept;

    [[nodiscard]] float CalcLength() const noexcept;
    [[nodiscard]] float CalcLengthSquared() const noexcept;

    void SetStartEndPositions(const Vector3& start_position, const Vector3& end_position) noexcept;

    void Translate(const Vector3& translation) noexcept;

    [[nodiscard]] Vector3 CalcDisplacement() const noexcept;
    [[nodiscard]] Vector3 CalcDirection() const noexcept;

    [[nodiscard]] Capsule3 operator+(const Vector3& translation) const noexcept;
    [[nodiscard]] Capsule3 operator-(const Vector3& antiTranslation) const noexcept;

    Capsule3& operator+=(const Vector3& translation) noexcept;
    Capsule3& operator-=(const Vector3& antiTranslation) noexcept;

protected:
private:
};