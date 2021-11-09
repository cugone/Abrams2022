#include "Engine/Physics/Collider.hpp"

#include "Engine/Core/Rgba.hpp"
#include "Engine/Physics/PhysicsTypes.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

ColliderPolygon::ColliderPolygon(int sides, const Vector2& position, const Vector2& half_extents, float orientationDegrees)
: Collider()
, _polygon{sides, position, half_extents, orientationDegrees} {
    /* DO NOTHING */
}

void ColliderPolygon::DebugRender() const noexcept {
    ServiceLocator::get<IRendererService>().DrawPolygon2D(Vector2::Zero, 0.5f, _polygon.GetSides(), Rgba::Pink);
}

int ColliderPolygon::GetSides() const {
    return _polygon.GetSides();
}

void ColliderPolygon::SetSides(int sides) {
    _polygon.SetSides(sides);
}

const std::vector<Vector2>& ColliderPolygon::GetVerts() const noexcept {
    return _polygon.GetVerts();
}

const Vector2& ColliderPolygon::GetPosition() const {
    return _polygon.GetPosition();
}

void ColliderPolygon::SetPosition(const Vector2& position) noexcept {
    _polygon.SetPosition(position);
}

void ColliderPolygon::Translate(const Vector2& translation) {
    _polygon.Translate(translation);
}

void ColliderPolygon::RotateDegrees(float displacementDegrees) {
    _polygon.RotateDegrees(displacementDegrees);
}

void ColliderPolygon::Rotate(float displacementRadians) {
    _polygon.Rotate(displacementRadians);
}

float ColliderPolygon::GetOrientationDegrees() const noexcept {
    return _polygon.GetOrientationDegrees();
}

void ColliderPolygon::SetOrientationDegrees(float degrees) noexcept {
    _polygon.SetOrientationDegrees(degrees);
}

Vector2 ColliderPolygon::GetHalfExtents() const noexcept {
    return _polygon.GetHalfExtents();
}

void ColliderPolygon::SetHalfExtents(const Vector2& newHalfExtents) {
    _polygon.SetHalfExtents(newHalfExtents);
}

Vector2 ColliderPolygon::CalcDimensions() const noexcept {
    const auto& verts = _polygon.GetVerts();
    const auto&& [min_x, max_x] = std::minmax_element(std::cbegin(verts), std::cend(verts), [](const Vector2& a, const Vector2& b) {
        return a.x < b.x;
    });
    const auto&& [min_y, max_y] = std::minmax_element(std::cbegin(verts), std::cend(verts), [](const Vector2& a, const Vector2& b) {
        return a.y < b.y;
    });
    const float width = (*max_x).x - (*min_x).x;
    const float height = (*max_y).y - (*min_y).y;
    return Vector2{width, height};
}

float ColliderPolygon::CalcArea() const noexcept {
    float A = 0.0f;
    const auto& verts = _polygon.GetVerts();
    auto s = verts.size();
    for(std::size_t i = 0; i < s; ++i) {
        std::size_t j = (i + 1) % s;
        A += (verts[i].x * verts[j].y) - (verts[j].x * verts[i].y);
    }
    return 0.5f * std::abs(A);
}

OBB2 ColliderPolygon::GetBounds() const noexcept {
    return OBB2(_polygon.GetPosition(), CalcDimensions() * 0.5f, _polygon.GetOrientationDegrees());
}

Vector2 ColliderPolygon::Support(const Vector2& d) const noexcept {
    const auto& verts = _polygon.GetVerts();
    return *std::max_element(std::cbegin(verts), std::cend(verts), [&d](const Vector2& a, const Vector2& b) { return MathUtils::DotProduct(a, d.GetNormalize()) < MathUtils::DotProduct(b, d.GetNormalize()); });
}

Vector2 ColliderPolygon::CalcCenter() const noexcept {
    return _polygon.GetPosition();
}

const Polygon2& ColliderPolygon::GetPolygon() const noexcept {
    return _polygon;
}

ColliderPolygon* ColliderPolygon::Clone() const noexcept {
    return new ColliderPolygon(GetSides(), GetPosition(), GetHalfExtents(), GetOrientationDegrees());
}

ColliderOBB::ColliderOBB(const Vector2& position, const Vector2& half_extents)
: m_obb{position, half_extents, 0.0f} {
    /* DO NOTHING */
}

float ColliderOBB::CalcArea() const noexcept {
    const auto dims = CalcDimensions();
    return dims.x * dims.y;
}

void ColliderOBB::DebugRender() const noexcept {
    ServiceLocator::get<IRendererService>().DrawOBB2(m_obb.orientationDegrees, Rgba::Pink);
}

Vector2 ColliderOBB::GetHalfExtents() const noexcept {
    return m_obb.half_extents;
}

Vector2 ColliderOBB::Support(const Vector2& d) const noexcept {
    return MathUtils::CalcClosestPoint(m_obb.position + d, m_obb);
}

void ColliderOBB::SetPosition(const Vector2& position) noexcept {
    m_obb.position = position;
}

float ColliderOBB::GetOrientationDegrees() const noexcept {
    return m_obb.orientationDegrees;
}

void ColliderOBB::SetOrientationDegrees(float degrees) noexcept {
    m_obb.orientationDegrees = degrees;
}

Vector2 ColliderOBB::CalcDimensions() const noexcept {
    return m_obb.CalcDimensions();
}

OBB2 ColliderOBB::GetBounds() const noexcept {
    return m_obb;
}

Vector2 ColliderOBB::CalcCenter() const noexcept {
    return m_obb.CalcCenter();
}

ColliderOBB* ColliderOBB::Clone() const noexcept {
    return new ColliderOBB(CalcCenter(), CalcDimensions() * 0.5f);
}

ColliderCircle::ColliderCircle(const Position& position, float radius)
: ColliderPolygon(16, position.Get(), Vector2(radius, radius), 0.0f) {
    /* DO NOTHING */
}

float ColliderCircle::CalcArea() const noexcept {
    const auto& half_extents = _polygon.GetHalfExtents();
    return MathUtils::M_PI * half_extents.x * half_extents.x;
}

Vector2 ColliderCircle::GetHalfExtents() const noexcept {
    return _polygon.GetHalfExtents();
}

Vector2 ColliderCircle::Support(const Vector2& d) const noexcept {
    return _polygon.GetPosition() + d.GetNormalize() * _polygon.GetHalfExtents().x;
    //return ColliderPolygon::Support(d);
}

void ColliderCircle::DebugRender() const noexcept {
    ServiceLocator::get<IRendererService>().DrawCircle2D(Vector2::Zero, 0.5f, Rgba::Pink); // _polygon.GetPosition(), _polygon.GetHalfExtents().x, Rgba::Pink);
}

void ColliderCircle::SetPosition(const Vector2& position) noexcept {
    ColliderPolygon::SetPosition(position);
}

float ColliderCircle::GetOrientationDegrees() const noexcept {
    return _polygon.GetOrientationDegrees();
}

void ColliderCircle::SetOrientationDegrees(float degrees) noexcept {
    return ColliderPolygon::SetOrientationDegrees(degrees);
}

Vector2 ColliderCircle::CalcDimensions() const noexcept {
    return _polygon.GetHalfExtents() * 2.0f;
}

OBB2 ColliderCircle::GetBounds() const noexcept {
    return OBB2(_polygon.GetPosition(), _polygon.GetHalfExtents(), _polygon.GetOrientationDegrees());
}

Vector2 ColliderCircle::CalcCenter() const noexcept {
    return _polygon.GetPosition();
}

ColliderCircle* ColliderCircle::Clone() const noexcept {
    return new ColliderCircle(GetPosition(), GetHalfExtents().x);
}

ColliderAABB::ColliderAABB(const Vector2& position, const Vector2& half_extents)
: ColliderPolygon(4, position, half_extents, 45.0f) {
    /* DO NOTHING */
}

float ColliderAABB::CalcArea() const noexcept {
    const auto& dims = CalcDimensions();
    return dims.x * dims.y;
}

void ColliderAABB::DebugRender() const noexcept {
    AABB2 aabb = AABB2::Neg_One_to_One;
    aabb.ScalePadding(0.5f, 0.5f);
    ServiceLocator::get<IRendererService>().DrawAABB2(aabb, Rgba::Pink, Rgba::NoAlpha);
}

Vector2 ColliderAABB::GetHalfExtents() const noexcept {
    return CalcDimensions() * 0.5f;
}

Vector2 ColliderAABB::Support(const Vector2& d) const noexcept {
    return ColliderPolygon::Support(d);
}

void ColliderAABB::SetPosition(const Vector2& position) noexcept {
    ColliderPolygon::SetPosition(position);
}

float ColliderAABB::GetOrientationDegrees() const noexcept {
    return 0.0f;
}

void ColliderAABB::SetOrientationDegrees(float /*degrees*/) noexcept {
    ColliderPolygon::SetOrientationDegrees(45.0f);
}

Vector2 ColliderAABB::CalcDimensions() const noexcept {
    return ColliderPolygon::CalcDimensions();
}

OBB2 ColliderAABB::GetBounds() const noexcept {
    return ColliderPolygon::GetBounds();
}

Vector2 ColliderAABB::CalcCenter() const noexcept {
    return ColliderPolygon::CalcCenter();
}

ColliderAABB* ColliderAABB::Clone() const noexcept {
    return new ColliderAABB(CalcCenter(), CalcDimensions() * 0.5f);
}
