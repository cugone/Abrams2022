#include "Engine/Math/Polygon2.hpp"

Polygon2::Polygon2(int sides /*= 3*/, const Vector2& position /*= Vector2::ZERO*/, const Vector2& half_extents /*= Vector2(0.5f, 0.5f)*/, float orientationDegrees /*= 0.0f*/) noexcept
: _sides(sides)
, _orientationDegrees(orientationDegrees)
, _half_extents(half_extents)
, _position(position) {
    GUARANTEE_OR_DIE(!(_sides < 3), "A Polygon cannot have less than 3 sides!");
    CalcVerts();
    CalcNormals();
}

Polygon2::Polygon2(const OBB2& obb) noexcept
: Polygon2(4, obb.position, obb.half_extents, obb.orientationDegrees) {
    /* DO NOTHING */
}

std::vector<LineSegment2> Polygon2::GetEdges() const noexcept {
    const auto s = _verts.size();
    std::vector<LineSegment2> result{};
    result.reserve(s);
    for(std::size_t i = 0; i < s; ++i) {
        const auto& a = _verts[i];
        const auto& b = _verts[(i + 1) % _sides];
        result.push_back(LineSegment2{a, b});
    }
    return result;
}

AABB2 Polygon2::GetBounds() const noexcept {
    const auto&& [min_x, max_x] = std::minmax_element(std::cbegin(_verts), std::cend(_verts), [](const Vector2& a, const Vector2& b) { return a.x < b.x; });
    const auto&& [min_y, max_y] = std::minmax_element(std::cbegin(_verts), std::cend(_verts), [](const Vector2& a, const Vector2& b) { return a.y < b.y; });
    return AABB2{(*min_x).x, (*min_y).y, (*max_x).x, (*max_y).y};
}

int Polygon2::GetSides() const {
    return _sides;
}

void Polygon2::SetSides(int sides) {
    if(_sides == sides) {
        return;
    }
    _sides = sides;
    CalcVerts();
    CalcNormals();
}

const Vector2& Polygon2::GetPosition() const {
    return _position;
}

void Polygon2::SetPosition(const Vector2& position) {
    _position = position;
    CalcVerts();
}

void Polygon2::Translate(const Vector2& translation) {
    _position += translation;
    CalcVerts();
}

void Polygon2::RotateDegrees(float displacementDegrees) {
    SetOrientationDegrees(GetOrientationDegrees() + displacementDegrees);
}

void Polygon2::Rotate(float displacementRadians) {
    RotateDegrees(MathUtils::ConvertRadiansToDegrees(displacementRadians));
}

float Polygon2::GetOrientationDegrees() const {
    return _orientationDegrees;
}

void Polygon2::SetOrientationDegrees(float degrees) {
    _orientationDegrees = degrees;
    _orientationDegrees = MathUtils::Wrap(_orientationDegrees, 0.0f, 360.0f);
    CalcVerts();
    CalcNormals();
}

const std::vector<Vector2>& Polygon2::GetVerts() const {
    return _verts;
}

const std::vector<Vector2>& Polygon2::GetNormals() const {
    return _normals;
}

const Vector2& Polygon2::GetHalfExtents() const {
    return _half_extents;
}

void Polygon2::SetHalfExtents(const Vector2& newHalfExtents) {
    _half_extents = newHalfExtents;
    CalcVerts();
}

void Polygon2::AddPaddingToSides(float paddingX, float paddingY) {
    SetHalfExtents(GetHalfExtents() + Vector2{paddingX, paddingY});
}

void Polygon2::AddPaddingToSides(const Vector2& padding) {
    AddPaddingToSides(padding.x, padding.y);
}

void Polygon2::CalcNormals() {
    const auto s = _verts.size();
    _normals.clear();
    if(_normals.capacity() < s) {
        _normals.reserve(s);
    }
    for(std::size_t i = 0; i < s; ++i) {
        const auto j = (i + 1) % s;
        const auto n = (_verts[j] - _verts[i]).GetNormalize().GetLeftHandNormal();
        _normals.push_back(n);
    }
    const auto S = Matrix4::CreateScaleMatrix(_half_extents);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(_orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    for(auto& n : _normals) {
        n = M.TransformDirection(n);
    }
}

void Polygon2::CalcVerts() {
    const auto num_sides_as_float = static_cast<float>(_sides);
    _verts.clear();
    if(_verts.capacity() < _sides) {
        _verts.reserve(_sides);
    }
    const auto anglePerVertex = 360.0f / num_sides_as_float;
    for(auto degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        const auto radians = MathUtils::ConvertDegreesToRadians(degrees);
        const auto pX = 0.5f * std::cos(radians);
        const auto pY = 0.5f * std::sin(radians);
        _verts.emplace_back(Vector2(pX, pY));
    }
    const auto S = Matrix4::CreateScaleMatrix(_half_extents);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(_orientationDegrees);
    const auto T = Matrix4::CreateTranslationMatrix(_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    for(auto& v : _verts) {
        v = M.TransformPosition(v);
    }
}
