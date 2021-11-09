#include "Engine/Math/Disc2.hpp"

#include "Engine/Math/MathUtils.hpp"

const Disc2 Disc2::Unit_Circle(0.0f, 0.0f, 1.0f);

Disc2::Disc2(float initialX, float initialY, float initialRadius) noexcept
: center(initialX, initialY)
, radius(initialRadius) {
    /* DO NOTHING */
}

Disc2::Disc2(const Vector2& initialCenter, float initialRadius) noexcept
: center(initialCenter)
, radius(initialRadius) {
    /* DO NOTHING */
}

void Disc2::StretchToIncludePoint(const Vector2& point) noexcept {
    if(MathUtils::CalcDistanceSquared(center, point) < (radius * radius)) {
        return;
    }
    radius = MathUtils::CalcDistance(center, point);
}

void Disc2::AddPadding(float paddingRadius) noexcept {
    radius += paddingRadius;
}

void Disc2::Translate(const Vector2& translation) noexcept {
    center += translation;
}

Disc2 Disc2::operator-(const Vector2& antiTranslation) noexcept {
    return Disc2(center - antiTranslation, radius);
}

Disc2& Disc2::operator-=(const Vector2& antiTranslation) noexcept {
    center -= antiTranslation;
    return *this;
}

Disc2 Disc2::operator+(const Vector2& translation) noexcept {
    return Disc2(center + translation, radius);
}

Disc2& Disc2::operator+=(const Vector2& translation) noexcept {
    center += translation;
    return *this;
}
