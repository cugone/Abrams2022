#pragma once

#include "Engine/Math/Vector3.hpp"

class Sphere3 {
public:
    Vector3 center = Vector3::Zero;
    float radius = 0.0f;

    static const Sphere3 Unit_Sphere;

    Sphere3() = default;
    Sphere3(const Sphere3& rhs) = default;
    Sphere3(Sphere3&& rhs) = default;
    Sphere3& operator=(const Sphere3& rhs) = default;
    Sphere3& operator=(Sphere3&& rhs) = default;
    ~Sphere3() = default;

    explicit Sphere3(float initialX, float initialY, float initialZ, float initialRadius) noexcept;
    explicit Sphere3(const Vector3& initialCenter, float initialRadius) noexcept;

    void StretchToIncludePoint(const Vector3& point) noexcept;
    void AddPadding(float paddingRadius) noexcept;
    void Translate(const Vector3& translation) noexcept;

    [[nodiscard]] Sphere3 operator+(const Vector3& translation) const noexcept;
    [[nodiscard]] Sphere3 operator-(const Vector3& antiTranslation) const noexcept;

    Sphere3& operator+=(const Vector3& translation) noexcept;
    Sphere3& operator-=(const Vector3& antiTranslation) noexcept;

protected:
private:
};