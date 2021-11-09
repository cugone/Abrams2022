#include "Engine/Math/OBB2.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"

OBB2::OBB2(const Vector2& center, const Vector2& halfExtents, float orientationDegrees) noexcept
: half_extents(halfExtents)
, position(center)
, orientationDegrees(orientationDegrees) {
    /* DO NOTHING */
}

OBB2::OBB2(const Vector2& center, float halfExtentX, float halfExtentY, float orientationDegrees) noexcept
: half_extents(halfExtentX, halfExtentY)
, position(center)
, orientationDegrees(orientationDegrees) {
    /* DO NOTHING */
}

OBB2::OBB2(const Vector2& initialPosition, float initialOrientationDegrees) noexcept
: position(initialPosition)
, orientationDegrees(initialOrientationDegrees) {
    /* DO NOTHING */
}

OBB2::OBB2(float initialX, float initialY, float initialOrientationDegrees) noexcept
: position(initialX, initialY)
, orientationDegrees(initialOrientationDegrees) {
    /* DO NOTHING */
}

OBB2::OBB2(const AABB2& aabb) noexcept
: half_extents(aabb.CalcDimensions() * 0.5f)
, position(aabb.CalcCenter())
, orientationDegrees(0.0f) {
    /* DO NOTHING */
}

AABB2 OBB2::AsAABB2() const {
    return {position - half_extents, position + half_extents};
}

void OBB2::SetOrientationDegrees(float newOrientationDegrees) noexcept {
    orientationDegrees = newOrientationDegrees;
    orientationDegrees = MathUtils::Wrap(orientationDegrees, 0.0f, 360.0f);
}

void OBB2::SetOrientation(float newOrientationRadians) noexcept {
    SetOrientationDegrees(MathUtils::ConvertRadiansToDegrees(newOrientationRadians));
}

void OBB2::RotateDegrees(float rotationDegrees) noexcept {
    orientationDegrees += rotationDegrees;
}

void OBB2::Rotate(float rotationRadians) noexcept {
    RotateDegrees(MathUtils::ConvertRadiansToDegrees(rotationRadians));
}

void OBB2::StretchToIncludePoint(const Vector2& point) noexcept {
    const auto disp_to_point = point - position;
    Translate(-point);
    AddPaddingToSides(disp_to_point);
}

void OBB2::AddPaddingToSides(float paddingX, float paddingY) noexcept {
    AddPaddingToSides(Vector2{paddingX, paddingY});
}

void OBB2::AddPaddingToSides(const Vector2& padding) noexcept {
    half_extents += padding;
}

void OBB2::AddPaddingToSidesClamped(float paddingX, float paddingY) noexcept {
    const auto half_width = half_extents.x;
    const auto half_height = half_extents.y;

    paddingX = (std::max)(-half_width, paddingX);
    paddingY = (std::max)(-half_height, paddingY);

    AddPaddingToSides(paddingX, paddingY);
}

void OBB2::AddPaddingToSidesClamped(const Vector2& padding) noexcept {
    const auto half_width = half_extents.x;
    const auto half_height = half_extents.y;

    const auto clamped_padding = Vector2((std::max)(-half_width, padding.x), (std::max)(-half_height, padding.y));

    AddPaddingToSides(clamped_padding);
}

void OBB2::Translate(const Vector2& translation) noexcept {
    position += translation;
}

Vector2 OBB2::GetRight() const noexcept {
    const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
    return R.TransformDirection(Vector2::X_Axis);
}

Vector2 OBB2::GetUp() const noexcept {
    auto up = GetRight();
    up.RotateNegative90Degrees();
    return up;
}

Vector2 OBB2::GetLeft() const noexcept {
    return -GetRight();
}

Vector2 OBB2::GetDown() const noexcept {
    return -GetUp();
}

Vector2 OBB2::GetRightEdge() const noexcept {
    return GetTopRight() - GetBottomRight();
}

Vector2 OBB2::GetTopEdge() const noexcept {
    return GetTopLeft() - GetTopRight();
}

Vector2 OBB2::GetLeftEdge() const noexcept {
    return GetBottomLeft() - GetTopLeft();
}

Vector2 OBB2::GetBottomEdge() const noexcept {
    return GetBottomRight() - GetBottomLeft();
}

Vector2 OBB2::GetBottomLeft() const noexcept {
    const auto S = Matrix4::CreateScaleMatrix(half_extents * 2.0f);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    return M.TransformPosition(Vector2{-0.5, +0.5});
}

Vector2 OBB2::GetTopLeft() const noexcept {
    const auto S = Matrix4::CreateScaleMatrix(half_extents * 2.0f);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    return M.TransformPosition(Vector2{-0.5, -0.5});
}

Vector2 OBB2::GetTopRight() const noexcept {
    const auto S = Matrix4::CreateScaleMatrix(half_extents * 2.0f);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    return M.TransformPosition(Vector2{+0.5, -0.5});
}

Vector2 OBB2::GetBottomRight() const noexcept {
    const auto S = Matrix4::CreateScaleMatrix(half_extents * 2.0f);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    return M.TransformPosition(Vector2{+0.5, +0.5});
}

Vector2 OBB2::CalcDimensions() const noexcept {
    return half_extents * 2.0f;
}

Vector2 OBB2::CalcCenter() const noexcept {
    return position;
}

OBB2 OBB2::operator+(const Vector2& translation) const noexcept {
    return OBB2(position + translation, orientationDegrees);
}

OBB2 OBB2::operator-(const Vector2& antiTranslation) const noexcept {
    return OBB2(position - antiTranslation, orientationDegrees);
}

OBB2& OBB2::operator-=(const Vector2& antiTranslation) noexcept {
    position -= antiTranslation;
    return *this;
}

OBB2& OBB2::operator+=(const Vector2& translation) noexcept {
    position += translation;
    return *this;
}
