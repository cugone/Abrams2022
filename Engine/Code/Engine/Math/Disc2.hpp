#pragma once

#include "Engine/Math/Vector2.hpp"

class Disc2 {
public:
    static const Disc2 Unit_Circle;

    Disc2() = default;
    Disc2(const Disc2& rhs) = default;
    Disc2& operator=(const Disc2& rhs) = default;
    Disc2& operator=(Disc2&& rhs) = default;
    ~Disc2() = default;
    explicit Disc2(float initialX, float initialY, float initialRadius) noexcept;
    explicit Disc2(const Vector2& initialCenter, float initialRadius) noexcept;

    void StretchToIncludePoint(const Vector2& point) noexcept;
    void AddPadding(float paddingRadius) noexcept;
    void Translate(const Vector2& translation) noexcept;

    [[nodiscard]] Disc2 operator-(const Vector2& antiTranslation) noexcept;
    [[nodiscard]] Disc2 operator+(const Vector2& translation) noexcept;
    Disc2& operator+=(const Vector2& translation) noexcept;
    Disc2& operator-=(const Vector2& antiTranslation) noexcept;

    Vector2 center = Vector2::Zero;
    float radius = 0.0f;

protected:
private:
};