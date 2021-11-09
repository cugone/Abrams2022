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

    ParticleState _initialState{};
    ParticleRenderState _particleRenderState{};

    EmitterDefinition _emitterPositionDefinition{};
    EmitterDefinition _emitterVelocityDefinition{};
    Vector3 _position{Vector3::Zero};
    Vector3 _velocity{Vector3::Zero};
    Vector3 _acceleration{Vector3::Zero};
    std::size_t _initialBurst{0u};
    float _spawnPerSecond{0.0f};
    float _lifetime{0.0f};
    float _particleLifetime{0.0f};
    float _mass{0.0f};
    std::string _materialName{};
    std::string _name{};
    Rgba _particleColor{Rgba::White};
    bool _isPrewarmed{false};
    bool _isBillboarded{true};
    bool _isSorted{true};

    friend class ParticleEmitter;
    friend class ParticleEffectDefinition;
    friend class ParticleEffect;
};
