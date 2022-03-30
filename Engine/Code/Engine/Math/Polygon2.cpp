#include "Engine/Math/Polygon2.hpp"

Polygon2::Polygon2(int sides /*= 3*/, const Vector2& position /*= Vector2::ZERO*/, const Vector2& half_extents /*= Vector2(0.5f, 0.5f)*/, float orientationDegrees /*= 0.0f*/) noexcept
: m_sides(sides)
, m_orientationDegrees(orientationDegrees)
, m_half_extents(half_extents)
, m_position(position) {
    GUARANTEE_OR_DIE(!(m_sides < 3), "A Polygon cannot have less than 3 sides!");
    CalcVerts();
    CalcNormals();
}

Polygon2::Polygon2(const OBB2& obb) noexcept
: Polygon2(4, obb.position, obb.half_extents, obb.orientationDegrees) {
    /* DO NOTHING */
}

std::vector<LineSegment2> Polygon2::GetEdges() const noexcept {
    const auto s = m_verts.size();
    std::vector<LineSegment2> result{};
    result.reserve(s);
    for(std::size_t i = 0; i < s; ++i) {
        const auto& a = m_verts[i];
        const auto& b = m_verts[(i + 1) % m_sides];
        result.push_back(LineSegment2{a, b});
    }
    return result;
}

AABB2 Polygon2::GetBounds() const noexcept {
    const auto&& [min_x, max_x] = std::minmax_element(std::cbegin(m_verts), std::cend(m_verts), [](const Vector2& a, const Vector2& b) { return a.x < b.x; });
    const auto&& [min_y, max_y] = std::minmax_element(std::cbegin(m_verts), std::cend(m_verts), [](const Vector2& a, const Vector2& b) { return a.y < b.y; });
    return AABB2{(*min_x).x, (*min_y).y, (*max_x).x, (*max_y).y};
}

int Polygon2::GetSides() const {
    return m_sides;
}

void Polygon2::SetSides(int sides) {
    if(m_sides == sides) {
        return;
    }
    m_sides = sides;
    CalcVerts();
    CalcNormals();
}

const Vector2& Polygon2::GetPosition() const {
    return m_position;
}

void Polygon2::SetPosition(const Vector2& position) {
    m_position = position;
    CalcVerts();
}

void Polygon2::Translate(const Vector2& translation) {
    m_position += translation;
    CalcVerts();
}

void Polygon2::RotateDegrees(float displacementDegrees) {
    SetOrientationDegrees(GetOrientationDegrees() + displacementDegrees);
}

void Polygon2::Rotate(float displacementRadians) {
    RotateDegrees(MathUtils::ConvertRadiansToDegrees(displacementRadians));
}

float Polygon2::GetOrientationDegrees() const {
    return m_orientationDegrees;
}

void Polygon2::SetOrientationDegrees(float degrees) {
    m_orientationDegrees = degrees;
    m_orientationDegrees = MathUtils::Wrap(m_orientationDegrees, 0.0f, 360.0f);
    CalcVerts();
    CalcNormals();
}

const std::vector<Vector2>& Polygon2::GetVerts() const {
    return m_verts;
}

const std::vector<Vector2>& Polygon2::GetNormals() const {
    return m_normals;
}

const Vector2& Polygon2::GetHalfExtents() const {
    return m_half_extents;
}

void Polygon2::SetHalfExtents(const Vector2& newHalfExtents) {
    m_half_extents = newHalfExtents;
    CalcVerts();
}

void Polygon2::AddPaddingToSides(float paddingX, float paddingY) {
    SetHalfExtents(GetHalfExtents() + Vector2{paddingX, paddingY});
}

void Polygon2::AddPaddingToSides(const Vector2& padding) {
    AddPaddingToSides(padding.x, padding.y);
}

void Polygon2::CalcNormals() {
    const auto s = m_verts.size();
    m_normals.clear();
    if(m_normals.capacity() < s) {
        m_normals.reserve(s);
    }
    for(std::size_t i = 0; i < s; ++i) {
        const auto j = (i + 1) % s;
        const auto n = (m_verts[j] - m_verts[i]).GetNormalize().GetLeftHandNormal();
        m_normals.push_back(n);
    }
    const auto S = Matrix4::CreateScaleMatrix(m_half_extents);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(m_orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(m_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    for(auto& n : m_normals) {
        n = M.TransformDirection(n);
    }
}

void Polygon2::CalcVerts() {
    const auto num_sides_as_float = static_cast<float>(m_sides);
    m_verts.clear();
    if(m_verts.capacity() < m_sides) {
        m_verts.reserve(m_sides);
    }
    const auto anglePerVertex = 360.0f / num_sides_as_float;
    for(auto degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        const auto radians = MathUtils::ConvertDegreesToRadians(degrees);
        const auto pX = 0.5f * std::cos(radians);
        const auto pY = 0.5f * std::sin(radians);
        m_verts.emplace_back(Vector2(pX, pY));
    }
    const auto S = Matrix4::CreateScaleMatrix(m_half_extents);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(m_orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(m_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    for(auto& v : m_verts) {
        v = M.TransformPosition(v);
    }
}
