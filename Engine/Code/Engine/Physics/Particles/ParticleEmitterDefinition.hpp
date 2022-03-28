#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Physics/Particles/Particle.hpp"

#include <map>
#include <string>

enum class EmitterType : unsigned char {
    Point,
    Line,
    Cone,
    Disc,
    Sphere,
    Max,
};

struct EmitterDefinition {
    EmitterType type{EmitterType::Point};
    Vector3 start{Vector3::Zero};
    Vector3 end{Vector3::Zero};
    Vector3 normal{Vector3::X_Axis};
    float radius{0.0f};
    float theta{0.0f};
    float length{0.0f};
};

class ParticleEmitterDefinition {
public:
    ParticleEmitterDefinition() noexcept = default;
    ParticleEmitterDefinition(const ParticleEmitterDefinition& other) noexcept = default;
    ParticleEmitterDefinition& operator=(const ParticleEmitterDefinition& rhs) noexcept = default;
    ParticleEmitterDefinition(ParticleEmitterDefinition&& other) noexcept = default;
    ParticleEmitterDefinition& operator=(ParticleEmitterDefinition&& rhs) noexcept = default;
    ~ParticleEmitterDefinition() noexcept = default;

    explicit ParticleEmitterDefinition(const XMLElement& element) noexcept;
    static inline std::map<std::string, std::unique_ptr<ParticleEmitterDefinition>> s_particleEmitterDefintions{};

    static ParticleEmitterDefinition* CreateOrGetParticleEmitterDefinition(const std::string& name, const XMLElement& element) noexcept;
    static ParticleEmitterDefinition* CreateAndRegisterParticleEmitterDefinition(const XMLElement& element) noexcept;
    static ParticleEmitterDefinition* GetParticleEmitterDefinition(const std::string& name) noexcept;
protected:
private:

    static std::unique_ptr<ParticleEmitterDefinition> CreateParticleEmitterDefinition(const XMLElement& element) noexcept;

    ParticleState m_initialState{};
    ParticleRenderState m_particleRenderState{};

    EmitterDefinition m_emitterPositionDefinition{};
    EmitterDefinition m_emitterVelocityDefinition{};
    Vector3 m_position{Vector3::Zero};
    Vector3 m_velocity{Vector3::Zero};
    Vector3 m_acceleration{Vector3::Zero};
    std::size_t m_initialBurst{0u};
    float m_spawnPerSecond{0.0f};
    float m_lifetime{0.0f};
    float m_particleLifetime{0.0f};
    float m_mass{0.0f};
    std::string m_materialName{};
    std::string m_name{};
    Rgba m_particleColor{Rgba::White};
    bool m_isPrewarmed{false};
    bool m_isBillboarded{true};
    bool m_isSorted{true};

    friend class ParticleEmitter;
    friend class ParticleEffectDefinition;
    friend class ParticleEffect;
};
