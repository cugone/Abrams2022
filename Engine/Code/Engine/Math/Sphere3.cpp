#include "Engine/Math/Sphere3.hpp"

#include "Engine/Math/MathUtils.hpp"

const Sphere3 Sphere3::Unit_Sphere(0.0f, 0.0f, 0.0f, 1.0f);

Sphere3::Sphere3(float initialX, float initialY, float initialZ, float initialRadius) noexcept
: center(initialX, initialY, initialZ)
, radius(initialRadius) {
    /* DO NOTHING */
}

Sphere3::Sphere3(const Vector3& initialCenter, float initialRadius) noexcept
: center(initialCenter)
, radius(initialRadius) {
    /* DO NOTHING */
}

void Sphere3::StretchToIncludePoint(const Vector3& point) noexcept {
    if(MathUtils::CalcDistanceSquared(center, point) < (radius * radius)) {
        return;
    }
    radius = MathUtils::CalcDistance(center, point);
}

void Sphere3::AddPadding(float paddingRadius) noexcept {
    radius += paddingRadius;
}

void Sphere3::Translate(const Vector3& translation) noexcept {
    center += translation;
}

Sphere3 Sphere3::operator+(const Vector3& translation) const noexcept {
    return Sphere3(center + translation, radius);
}

Sphere3 Sphere3::operator-(const Vector3& antiTranslation) const noexcept {
    return Sphere3(center - antiTranslation, radius);
}

Sphere3& Sphere3::operator-=(const Vector3& antiTranslation) noexcept {
    center -= antiTranslation;
    return *this;
}

Sphere3& Sphere3::operator+=(const Vector3& translation) noexcept {
    center += translation;
    return *this;
}
