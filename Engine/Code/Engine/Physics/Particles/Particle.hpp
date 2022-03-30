#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Renderer/Mesh.hpp"
#include "Engine/Renderer/Vertex3D.hpp"

#include <functional>
#include <vector>

class Texture2D;
class Particle;
class Material;

class ParticleRenderState {
public:
    enum class ParticleShape {
        Quad,
        Cube,
    };
    ParticleRenderState() noexcept = default;

    ParticleRenderState(const ParticleRenderState& other) = default;
    ParticleRenderState& operator=(const ParticleRenderState& rhs) = default;

    void SetLifetime(float lifetimeSeconds);
    void SetScales(const Vector3& start, const Vector3& end);

    void SetColors(const Rgba& start, const Rgba& end);
    const Rgba& GetColor() const;
    Rgba& GetColor();

    void SetMaterial(Material* tex);
    void SetBillboarding(bool isBillboarded);

    void SetShape(const ParticleShape& renderShape);
    const ParticleShape& GetShape() const;

    const Rgba& GetStartColor() const;
    const Rgba& GetEndColor() const;
    const Vector3& GetStartScale() const;
    const Vector3& GetEndScale() const;

private:
    Vector3 m_start_scale{Vector3::One};
    Vector3 m_scale{Vector3::One};
    Vector3 m_end_scale{Vector3::One};
    float m_start_age{1.0f};
    float m_age{1.0f};
    Rgba m_start_color{Rgba::White};
    Rgba m_color{Rgba::White};
    Rgba m_end_color{Rgba::White};
    ParticleShape m_shape{ParticleShape::Quad};
    Material* m_particle_material{nullptr};
    bool m_billboarded{false};

    friend class Particle;
};

class ParticleState {
public:
    Vector3 position{};
    Vector3 velocity{};
    Vector3 acceleration{};
    float mass{1.0f};

    ParticleState();
    ParticleState& operator=(const ParticleState& rhs) = default;
    ParticleState operator*(float scalar);
    ParticleState operator*=(float scalar);
    ParticleState operator+(const ParticleState& rhs);
    ParticleState operator+=(const ParticleState& rhs);

    friend class Particle;
};

using ParticleIntegrator = std::function<ParticleState(float time, const ParticleState& state)>;

class Particle {
public:
    static ParticleIntegrator semi_implicit_euler;

    Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState);
    Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState, const ParticleIntegrator& model);
    Particle(const Particle& other) = default;
    Particle& operator=(const Particle& rhs) = default;
    ~Particle() = default;

    const ParticleState& GetState() const;
    ParticleState& GetState();

    const ParticleRenderState& GetRenderState() const;
    ParticleRenderState& GetRenderState();

    void SetIntegrationModel(const ParticleIntegrator& model);

    void Update(float time, float deltaSeconds);
    void Render(Mesh::Builder& builder) const;

    bool IsTransparent() const;
    bool IsAlive() const;
    bool IsDead() const;
    void Kill();

    void SetParentTransform(const Matrix4& transform);
    const Matrix4& GetParentTransform() const;
    Matrix4& GetParentTransform();

protected:
private:
    ParticleRenderState m_renderState{};
    ParticleState m_nextState{};
    ParticleState m_curState{};
    ParticleIntegrator m_model{};
    Matrix4 m_parentTransform{Matrix4::I};
};

bool operator<(const Particle& a, const Particle& b);