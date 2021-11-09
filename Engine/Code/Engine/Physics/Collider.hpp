#pragma once

#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Polygon2.hpp"
#include "Engine/Math/Vector2.hpp"

#include <vector>

class Renderer;
struct Position;

class Collider {
public:
    virtual ~Collider() = default;
    virtual void DebugRender() const noexcept = 0;
    [[nodiscard]] virtual Vector2 CalcDimensions() const noexcept = 0;
    [[nodiscard]] virtual Vector2 CalcCenter() const noexcept = 0;
    [[nodiscard]] virtual float CalcArea() const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetHalfExtents() const noexcept = 0;
    virtual void SetPosition(const Vector2& position) noexcept = 0;
    virtual void SetOrientationDegrees(float orientationDegrees) noexcept = 0;
    [[nodiscard]] virtual float GetOrientationDegrees() const noexcept = 0;
    [[nodiscard]] virtual OBB2 GetBounds() const noexcept = 0;
    [[nodiscard]] virtual Vector2 Support(const Vector2& d) const noexcept = 0;
    [[nodiscard]] virtual Collider* Clone() const noexcept = 0;
};

class ColliderPolygon : public Collider {
public:
    ColliderPolygon() = default;
    explicit ColliderPolygon(int sides, const Vector2& position, const Vector2& half_extents, float orientationDegrees);

    virtual ~ColliderPolygon() = default;
    virtual void DebugRender() const noexcept override;
    virtual void SetPosition(const Vector2& position) noexcept override;
    [[nodiscard]] virtual float GetOrientationDegrees() const noexcept override;
    virtual void SetOrientationDegrees(float degrees) noexcept override;
    [[nodiscard]] virtual Vector2 CalcDimensions() const noexcept override;
    [[nodiscard]] virtual float CalcArea() const noexcept override;
    [[nodiscard]] virtual Vector2 Support(const Vector2& d) const noexcept override;
    [[nodiscard]] virtual Vector2 CalcCenter() const noexcept override;
    [[nodiscard]] virtual ColliderPolygon* Clone() const noexcept override;

    [[nodiscard]] int GetSides() const;
    void SetSides(int sides);
    [[nodiscard]] const std::vector<Vector2>& GetVerts() const noexcept;
    [[nodiscard]] const Vector2& GetPosition() const;
    void Translate(const Vector2& translation);
    void RotateDegrees(float displacementDegrees);
    void Rotate(float displacementDegrees);
    [[nodiscard]] Vector2 GetHalfExtents() const noexcept override;
    void SetHalfExtents(const Vector2& newHalfExtents);
    [[nodiscard]] OBB2 GetBounds() const noexcept override;

    [[nodiscard]] const Polygon2& GetPolygon() const noexcept;

protected:
    Polygon2 _polygon = Polygon2{4, Vector2::Zero, Vector2{0.5f, 0.5f}, 0.0f};

private:
};

class ColliderAABB : public ColliderPolygon {
public:
    ColliderAABB(const Vector2& position, const Vector2& half_extents);
    virtual ~ColliderAABB() = default;
    [[nodiscard]] virtual float CalcArea() const noexcept override;

    virtual void DebugRender() const noexcept override;
    [[nodiscard]] virtual Vector2 GetHalfExtents() const noexcept override;
    [[nodiscard]] virtual Vector2 Support(const Vector2& d) const noexcept override;
    virtual void SetPosition(const Vector2& position) noexcept override;
    [[nodiscard]] virtual float GetOrientationDegrees() const noexcept override;
    virtual void SetOrientationDegrees(float degrees) noexcept override;
    [[nodiscard]] virtual Vector2 CalcDimensions() const noexcept override;
    [[nodiscard]] virtual OBB2 GetBounds() const noexcept override;
    [[nodiscard]] virtual Vector2 CalcCenter() const noexcept override;
    [[nodiscard]] virtual ColliderAABB* Clone() const noexcept override;

protected:
private:
};

class ColliderOBB : public Collider {
public:
    ColliderOBB(const Vector2& position, const Vector2& half_extents);
    virtual ~ColliderOBB() = default;
    [[nodiscard]] virtual float CalcArea() const noexcept override;

    virtual void DebugRender() const noexcept override;
    [[nodiscard]] virtual Vector2 GetHalfExtents() const noexcept override;
    [[nodiscard]] virtual Vector2 Support(const Vector2& d) const noexcept override;
    virtual void SetPosition(const Vector2& position) noexcept override;
    [[nodiscard]] virtual float GetOrientationDegrees() const noexcept override;
    virtual void SetOrientationDegrees(float degrees) noexcept override;
    [[nodiscard]] virtual Vector2 CalcDimensions() const noexcept override;
    [[nodiscard]] virtual OBB2 GetBounds() const noexcept override;
    [[nodiscard]] virtual Vector2 CalcCenter() const noexcept override;
    [[nodiscard]] virtual ColliderOBB* Clone() const noexcept override;

protected:
private:
    OBB2 m_obb;
};

class ColliderCircle : public ColliderPolygon {
public:
    ColliderCircle(const Position& position, float radius);
    virtual ~ColliderCircle() = default;
    [[nodiscard]] virtual float CalcArea() const noexcept override;
    [[nodiscard]] virtual Vector2 GetHalfExtents() const noexcept override;
    [[nodiscard]] virtual Vector2 Support(const Vector2& d) const noexcept override;
    virtual void DebugRender() const noexcept override;
    virtual void SetPosition(const Vector2& position) noexcept override;
    [[nodiscard]] virtual float GetOrientationDegrees() const noexcept override;
    virtual void SetOrientationDegrees(float degrees) noexcept override;
    [[nodiscard]] virtual Vector2 CalcDimensions() const noexcept override;
    [[nodiscard]] virtual OBB2 GetBounds() const noexcept override;
    [[nodiscard]] virtual Vector2 CalcCenter() const noexcept override;
    [[nodiscard]] virtual ColliderCircle* Clone() const noexcept override;

protected:
private:
};
