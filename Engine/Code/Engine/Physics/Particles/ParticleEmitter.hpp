#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include "Engine/Physics/Particles/Particle.hpp"

#include <string>
#include <vector>

class ParticleEmitterDefinition;
class Texture2D;
class ParticleEffect;

class ParticleEmitter {
public:
    ParticleEmitter() noexcept = default;
    ParticleEmitter(const ParticleEmitter& rhs) noexcept = default;
    ParticleEmitter(ParticleEmitter&& rhs) noexcept = default;
    ParticleEmitter& operator=(ParticleEmitter&& rhs) noexcept = default;
    ParticleEmitter& operator=(const ParticleEmitter& rhs) noexcept = default;
    ~ParticleEmitter() noexcept = default;

    explicit ParticleEmitter(const std::string& name) noexcept;

    void Initialize();
    void BeginFrame();
    void Prewarm(TimeUtils::FPSeconds secondsToWarm);
    void Update(float time, float deltaSeconds);
    void Render() const;
    void EndFrame();

    float GetAge() const;
    float GetLifetime() const;

    std::size_t GetParticleCount() const;
    bool HasAliveParticles() const;

    bool IsDead() const;
    bool IsAlive() const;
    void Kill();
    void MakeAlive();

    ParticleEffect* parent_effect = nullptr;

    Mesh::Builder& GetMeshBuilder() noexcept;
protected:
private:
    void LoadFromXML(const XMLElement& element);

    void SpawnParticle(const Vector3& initialPosition, const Vector3& initialVelocity, float ttl, const Rgba& color /*= Rgba::WHITE*/, const Rgba& endColor /*= Rgba::WHITE*/, const Vector3& scale /*= Vector3::ONE*/, const Vector3& endScale /*= Vector3::ONE*/, Material* initialMaterial /*= nullptr*/, float initialMass /*= 1.0f*/);
    void UpdateParticles(float time, float deltaSeconds);
    void DestroyDeadEntities();

    std::string _name{};
    Stopwatch _spawnClock{};
    std::vector<Particle> _particles{};
    mutable Mesh::Builder _builder{};
    float _age{0.0f};
    bool _isWarming{false};
};
