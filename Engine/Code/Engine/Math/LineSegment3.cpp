#include "Engine/Math/LineSegment3.hpp"

#include <cmath>

const LineSegment3 LineSegment3::Unit_Horizontal(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
const LineSegment3 LineSegment3::Unit_Vertical(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
const LineSegment3 LineSegment3::Unit_Depth(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
const LineSegment3 LineSegment3::Unit_Centered_Horizontal(-0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f);
const LineSegment3 LineSegment3::Unit_Centered_Vertical(0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f);
const LineSegment3 LineSegment3::Unit_Centered_Depth(0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f);

LineSegment3::LineSegment3(float startX, float startY, float startZ, float endX, float endY, float endZ) noexcept
: start(startX, startY, startZ)
, end(endX, endY, endZ) {
    /* DO NOTHING */
}

LineSegment3::LineSegment3(const Vector3& startPosition, const Vector3& endPosition) noexcept
: start(startPosition)
, end(endPosition) {
    /* DO NOTHING */
}

LineSegment3::LineSegment3(const Vector3& startPosition, const Vector3& direction, float length) noexcept
: start(startPosition)
, end(startPosition + (direction.GetNormalize() * length)) {
    /* DO NOTHING */
}

void LineSegment3::SetLengthFromStart(float length) noexcept {
    end = (end - start).GetNormalize() * length;
}

void LineSegment3::SetLengthFromCenter(float length) noexcept {
    const auto center = CalcCenter();
    const auto half_length = length * 0.5f;
    start = (start - center).GetNormalize() * half_length;
    end = (end - center).GetNormalize() * half_length;
}

void LineSegment3::SetLengthFromEnd(float length) noexcept {
    start = (start - end).GetNormalize() * length;
}

Vector3 LineSegment3::CalcCenter() const noexcept {
    return start + (end - start) * 0.5f;
}

float LineSegment3::CalcLength() const noexcept {
    return (end - start).CalcLength();
}

float LineSegment3::CalcLengthSquared() const noexcept {
    return (end - start).CalcLengthSquared();
}

void LineSegment3::SetStartEndPositions(const Vector3& startPosition, const Vector3& endPosition) noexcept {
    start = startPosition;
    end = endPosition;
}

void LineSegment3::Translate(const Vector3& translation) noexcept {
    start += translation;
    end += translation;
}

Vector3 LineSegment3::CalcDisplacement() const noexcept {
    return (end - start);
}

Vector3 LineSegment3::CalcDirection() const noexcept {
    return (end - start).GetNormalize();
}

LineSegment3 LineSegment3::operator+(const Vector3& translation) const noexcept {
    return LineSegment3(start + translation, end + translation);
}

LineSegment3 LineSegment3::operator-(const Vector3& antiTranslation) const noexcept {
    return LineSegment3(start - antiTranslation, end - antiTranslation);
}

LineSegment3& LineSegment3::operator-=(const Vector3& antiTranslation) noexcept {
    start -= antiTranslation;
    end -= antiTranslation;
    return *this;
}

LineSegment3& LineSegment3::operator+=(const Vector3& translation) noexcept {
    start += translation;
    end += translation;
    return *this;
}
