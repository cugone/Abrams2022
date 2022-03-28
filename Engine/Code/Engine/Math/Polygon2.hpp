#pragma once

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Vector2.hpp"

#include <vector>

class Polygon2 {
public:
    Polygon2(int sides = 3, const Vector2& position = Vector2::Zero, const Vector2& half_extents = Vector2(0.5f, 0.5f), float orientationDegrees = 0.0f) noexcept;
    explicit Polygon2(const OBB2& obb) noexcept;
    //~Polygon2() = default;

    [[nodiscard]] std::vector<LineSegment2> GetEdges() const noexcept;

    [[nodiscard]] AABB2 GetBounds() const noexcept;

    [[nodiscard]] int GetSides() const;
    void SetSides(int sides);
    [[nodiscard]] const Vector2& GetPosition() const;
    void SetPosition(const Vector2& position);
    void Translate(const Vector2& translation);
    void RotateDegrees(float displacementDegrees);
    void Rotate(float displacementRadians);
    [[nodiscard]] float GetOrientationDegrees() const;
    void SetOrientationDegrees(float degrees);
    [[nodiscard]] const std::vector<Vector2>& GetVerts() const;
    [[nodiscard]] const std::vector<Vector2>& GetNormals() const;
    [[nodiscard]] const Vector2& GetHalfExtents() const;
    void SetHalfExtents(const Vector2& newHalfExtents);
    void AddPaddingToSides(const Vector2& padding);
    void AddPaddingToSides(float paddingX, float paddingY);

protected:
    void CalcNormals();
    void CalcVerts();

private:
    int m_sides = 3;
    float m_orientationDegrees = 0.0f;
    Vector2 m_half_extents = Vector2(0.5f, 0.5f);
    Vector2 m_position = Vector2::Zero;
    std::vector<Vector2> m_verts;
    std::vector<Vector2> m_normals;
};
