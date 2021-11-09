#include "Engine/Math/AABB2.hpp"

#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Vector4.hpp"

#include <algorithm>

const AABB2 AABB2::Zero_to_One(0.0f, 0.0f, 1.0f, 1.0f);
const AABB2 AABB2::Neg_One_to_One(-1.0f, -1.0f, 1.0f, 1.0f);

AABB2::AABB2(float initialX, float initialY) noexcept
: mins(initialX, initialY)
, maxs(initialX, initialY) {
    /* DO NOTHING */
}

AABB2::AABB2(float minX, float minY, float maxX, float maxY) noexcept
: mins(minX, minY)
, maxs(maxX, maxY) {
    /* DO NOTHING */
}

AABB2::AABB2(const Vector2& mins, const Vector2& maxs) noexcept
: mins(mins)
, maxs(maxs) {
    /* DO NOTHING */
}

AABB2::AABB2(const Vector4& minsMaxs) noexcept
: AABB2(minsMaxs.GetXY(), minsMaxs.GetZW()) {
    /* DO NOTHING */
}
AABB2::AABB2(const Vector2& center, float radiusX, float radiusY) noexcept
: mins(center.x - radiusX, center.y - radiusY)
, maxs(center.x + radiusX, center.y + radiusY) {
    /* DO NOTHING */
}

AABB2::AABB2(const OBB2& obb) noexcept
: AABB2(obb.position, obb.half_extents.x, obb.half_extents.y) {
    /* DO NOTHING */
}

void AABB2::StretchToIncludePoint(const Vector2& point) noexcept {
    if(point.x < mins.x) {
        mins.x = point.x;
    }
    if(point.y < mins.y) {
        mins.y = point.y;
    }
    if(maxs.x < point.x) {
        maxs.x = point.x;
    }
    if(maxs.y < point.y) {
        maxs.y = point.y;
    }
}

void AABB2::ScalePadding(float scaleX, float scaleY) noexcept {
    mins.x *= scaleX;
    maxs.x *= scaleX;

    mins.y *= scaleY;
    maxs.y *= scaleY;

    if(maxs.x < mins.x) {
        std::swap(maxs.x, mins.x);
    }
    if(maxs.y < mins.y) {
        std::swap(maxs.y, mins.y);
    }
}

void AABB2::AddPaddingToSides(float paddingX, float paddingY) noexcept {
    mins.x -= paddingX;
    mins.y -= paddingY;

    maxs.x += paddingX;
    maxs.y += paddingY;

    if(maxs.x < mins.x) {
        std::swap(maxs.x, mins.x);
    }
    if(maxs.y < mins.y) {
        std::swap(maxs.y, mins.y);
    }
}

void AABB2::AddPaddingToSidesClamped(float paddingX, float paddingY) noexcept {
    const auto width = maxs.x - mins.x;
    const auto height = maxs.y - mins.y;
    const auto half_width = width * 0.5f;
    const auto half_height = height * 0.5f;

    paddingX = (std::max)(-half_width, paddingX);
    paddingY = (std::max)(-half_height, paddingY);

    mins.x -= paddingX;
    mins.y -= paddingY;

    maxs.x += paddingX;
    maxs.y += paddingY;
}

void AABB2::Translate(const Vector2& translation) noexcept {
    mins += translation;
    maxs += translation;
}

Vector2 AABB2::CalcDimensions() const noexcept {
    return Vector2(maxs.x - mins.x, maxs.y - mins.y);
}

Vector2 AABB2::CalcCenter() const noexcept {
    return Vector2(mins.x + (maxs.x - mins.x) * 0.5f, mins.y + (maxs.y - mins.y) * 0.5f);
}

void AABB2::SetPosition(const Vector2& center) noexcept {
    const auto half_extents = CalcDimensions() * 0.5f;
    mins = Vector2(center.x - half_extents.x, center.y - half_extents.y);
    maxs = Vector2(center.x + half_extents.x, center.y + half_extents.y);
}

AABB2 AABB2::operator+(const Vector2& translation) const noexcept {
    return AABB2(mins.x + translation.x, mins.y + translation.y, maxs.x + translation.x, maxs.y + translation.y);
}

AABB2 AABB2::operator-(const Vector2& antiTranslation) const noexcept {
    return AABB2(mins.x - antiTranslation.x, mins.y - antiTranslation.y, maxs.x - antiTranslation.x, maxs.y - antiTranslation.y);
}

AABB2& AABB2::operator-=(const Vector2& antiTranslation) noexcept {
    mins -= antiTranslation;
    maxs -= antiTranslation;
    return *this;
}

AABB2& AABB2::operator+=(const Vector2& translation) noexcept {
    mins += translation;
    maxs += translation;
    return *this;
}
