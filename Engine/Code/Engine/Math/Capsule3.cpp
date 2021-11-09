#include "Engine/Math/Capsule3.hpp"

const Capsule3 Capsule3::Unit_Horizontal(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
const Capsule3 Capsule3::Unit_Vertical(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
const Capsule3 Capsule3::Unit_Depth(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
const Capsule3 Capsule3::Unit_Centered_Horizontal(-0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f);
const Capsule3 Capsule3::Unit_Centered_Vertical(0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f);
const Capsule3 Capsule3::Unit_Centered_Depth(0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 1.0f);

Capsule3::Capsule3(const LineSegment3& line, float radius) noexcept
: line(line)
, radius(radius) {
    /* DO NOTHING */
}

Capsule3::Capsule3(float startX, float startY, float startZ, float endX, float endY, float endZ, float radius) noexcept
: line(startX, startY, startZ, endX, endY, endZ)
, radius(radius) {
    /* DO NOTHING */
}

Capsule3::Capsule3(const Vector3& start_position, const Vector3& end_position, float radius) noexcept
: line(start_position, end_position)
, radius(radius) {
    /* DO NOTHING */
}

Capsule3::Capsule3(const Vector3& start_position, const Vector3& direction, float length, float radius) noexcept
: line(start_position, direction.GetNormalize(), length)
, radius(radius) {
    /* DO NOTHING */
}

void Capsule3::SetLengthFromStart(float length) noexcept {
    line.SetLengthFromStart(length);
}

void Capsule3::SetLengthFromCenter(float length) noexcept {
    line.SetLengthFromCenter(length);
}

void Capsule3::SetLengthFromEnd(float length) noexcept {
    line.SetLengthFromEnd(length);
}

Vector3 Capsule3::CalcCenter() const noexcept {
    return line.CalcCenter();
}

float Capsule3::CalcLength() const noexcept {
    return line.CalcLength();
}

float Capsule3::CalcLengthSquared() const noexcept {
    return line.CalcLengthSquared();
}

void Capsule3::SetStartEndPositions(const Vector3& start_position, const Vector3& end_position) noexcept {
    line.SetStartEndPositions(start_position, end_position);
}

void Capsule3::Translate(const Vector3& translation) noexcept {
    line.Translate(translation);
}

Vector3 Capsule3::CalcDisplacement() const noexcept {
    return line.CalcDisplacement();
}

Vector3 Capsule3::CalcDirection() const noexcept {
    return line.CalcDirection();
}

Capsule3 Capsule3::operator+(const Vector3& translation) const noexcept {
    return Capsule3(line + translation, radius);
}

Capsule3 Capsule3::operator-(const Vector3& antiTranslation) const noexcept {
    return Capsule3(line - antiTranslation, radius);
}

Capsule3& Capsule3::operator-=(const Vector3& antiTranslation) noexcept {
    line -= antiTranslation;
    return *this;
}

Capsule3& Capsule3::operator+=(const Vector3& translation) noexcept {
    line += translation;
    return *this;
}
