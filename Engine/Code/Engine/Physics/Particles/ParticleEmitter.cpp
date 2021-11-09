#include "Engine/Physics/Particles/ParticleEmitter.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/LineSegment3.hpp"
#include "Engine/Math/Sphere3.hpp"

#include "Engine/Physics/Particles/ParticleEffect.hpp"
#include "Engine/Physics/Particles/ParticleEmitterDefinition.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Texture2D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <algorithm>
#include <numeric>

ParticleEmitter::ParticleEmitter(const std::string& name) noexcept
: _name(name) {
    const auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(_name);
    if(definition->_spawnPerSecond > 0.0f) {
        _spawnClock.SetFrequency(static_cast<unsigned int>(definition->_spawnPerSecond));
    } else {
        _spawnClock.SetSeconds(TimeUtils::FPSeconds{0.016f});
    }
}

void ParticleEmitter::Initialize() {
    /* DO NOTHING */
}

void ParticleEmitter::BeginFrame() {
    /* DO NOTHING */
}

void ParticleEmitter::Prewarm(TimeUtils::FPSeconds secondsToWarm) {
    _isWarming = true;
    Stopwatch oven;
    oven.SetSeconds(secondsToWarm);
    float time = 0.0f;
    constexpr float deltaSeconds = 1.0f / 60.0f;
    while(!oven.Check()) {
        Update(time, deltaSeconds);
        time += deltaSeconds;
    }
    _isWarming = false;
}

void ParticleEmitter::Update(float time, float deltaSeconds) {
    auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(_name);
    if(_age < definition->_lifetime) {
        if(!_isWarming) {
            _age += deltaSeconds;
        }
        auto emitter_pos_def = definition->_emitterPositionDefinition;
        Vector3 new_particle_position = parent_effect->position;
        switch(emitter_pos_def.type) {
        case EmitterType::Point:
            new_particle_position += definition->_position;
            break;
        case EmitterType::Line: {
            new_particle_position += MathUtils::GetRandomPointOn(LineSegment3{emitter_pos_def.start, emitter_pos_def.end});
            break;
        }
        case EmitterType::Cone:
            /* DO NOTHING */
            break;
        case EmitterType::Disc:
            new_particle_position += MathUtils::GetRandomPointInsideDisc(emitter_pos_def.start, emitter_pos_def.normal, emitter_pos_def.radius);
            break;
        case EmitterType::Sphere:
            new_particle_position += MathUtils::GetRandomPointOn(Sphere3{emitter_pos_def.start, emitter_pos_def.radius});
            break;
        }

        auto emitter_vel_def = definition->_emitterVelocityDefinition;
        Vector3 new_particle_velocity = parent_effect->velocity;
        switch(emitter_vel_def.type) {
        case EmitterType::Point:
            new_particle_velocity += definition->_velocity;
            break;
        case EmitterType::Line: {
            new_particle_velocity += MathUtils::GetRandomPointOn(LineSegment3{emitter_vel_def.start, emitter_vel_def.end});
            break;
        }
        case EmitterType::Cone:
            /* DO NOTHING */
            break;
        case EmitterType::Disc:
            new_particle_velocity += MathUtils::GetRandomPointInsideDisc(emitter_vel_def.start, emitter_vel_def.normal, emitter_vel_def.radius);
            break;
        case EmitterType::Sphere:
            new_particle_velocity += MathUtils::GetRandomPointOn(Sphere3{emitter_vel_def.start, emitter_vel_def.radius});
            break;
        }

        Rgba c = definition->_particleRenderState.GetStartColor();
        Rgba ec = definition->_particleRenderState.GetEndColor();
        Vector3 s = definition->_particleRenderState.GetStartScale();
        Vector3 es = definition->_particleRenderState.GetEndScale();
        auto particle_count = _spawnClock.DecrementAll();
        auto& renderer = ServiceLocator::get<IRendererService>();
        for(unsigned int i = 0; i < particle_count; ++i) {
            SpawnParticle(new_particle_position, new_particle_velocity, definition->_particleLifetime, c, ec, s, es, renderer.GetMaterial(definition->_materialName), definition->_mass);
        }
    }
    UpdateParticles(time, deltaSeconds);
}

void ParticleEmitter::Render() const {
    const auto p = Matrix4::CreateTranslationMatrix(parent_effect->position);
    const auto t = Matrix4::CreateTranslationMatrix(ParticleEmitterDefinition::GetParticleEmitterDefinition(_name)->_position);
    const auto s = Matrix4::I;
    const auto r = Matrix4::I;

    const auto pointlight_model = Matrix4::MakeRT(p, Matrix4::MakeSRT(s, r, t));

    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetModelMatrix(pointlight_model);

    for(auto& particle : _particles) {
        particle.Render(_builder);
    }
    Mesh::Render(_builder);
}

void ParticleEmitter::EndFrame() {
    _particles.erase(std::remove_if(std::begin(_particles), std::end(_particles), [](const Particle& a) { return a.IsDead(); }), std::end(_particles));
}

float ParticleEmitter::GetAge() const {
    return _age;
}

float ParticleEmitter::GetLifetime() const {
    return ParticleEmitterDefinition::GetParticleEmitterDefinition(_name)->_lifetime;
}

std::size_t ParticleEmitter::GetParticleCount() const {
    return _particles.size();
}

bool ParticleEmitter::HasAliveParticles() const {
    return !_particles.empty();
}

void ParticleEmitter::LoadFromXML(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "emitter", "", "name");
    const auto name = DataUtils::ParseXmlAttribute(element, "name", std::string{});
    _spawnClock.SetFrequency(static_cast<unsigned int>(ParticleEmitterDefinition::GetParticleEmitterDefinition(name)->_spawnPerSecond));
}

bool ParticleEmitter::IsDead() const {
    return !IsAlive();
}

bool ParticleEmitter::IsAlive() const {
    return _age > 0.0f && HasAliveParticles();
}
void ParticleEmitter::Kill() {
    _age = GetLifetime();
}

void ParticleEmitter::MakeAlive() {
    _age = 0.0f;
    _spawnClock.Reset();
}

Mesh::Builder& ParticleEmitter::GetMeshBuilder() noexcept {
    return _builder;
}

void ParticleEmitter::UpdateParticles(float time, float deltaSeconds) {
    std::sort(std::begin(_particles), std::end(_particles));
    auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(_name);
    const auto loc = Matrix4::CreateTranslationMatrix(definition->_position);
    auto& renderer = ServiceLocator::get<IRendererService>();
    const auto billboard = definition->_isBillboarded ? renderer.GetCamera().GetInverseViewMatrix() : Matrix4::I;
    const auto result = Matrix4::MakeRT(billboard, loc);
    for(Particle& p : _particles) {
        renderer.AppendModelMatrix(result);
        p.Update(time, deltaSeconds);
    }
}

void ParticleEmitter::SpawnParticle(const Vector3& initialPosition, const Vector3& initialVelocity, float ttl, const Rgba& color /*= Rgba::WHITE*/, const Rgba& endColor /*= Rgba::WHITE*/, const Vector3& scale /*= Vector3::ONE*/, const Vector3& endScale /*= Vector3::ONE*/, Material* initialMaterial /*= nullptr*/, float initialMass /*= 1.0f*/) {
    ParticleState initialState;
    initialState.mass = initialMass;
    initialState.position = initialPosition;
    initialState.velocity = initialVelocity;
    initialState.acceleration = Vector3::Zero;

    ParticleRenderState initialRender;
    initialRender.SetColors(color, endColor);
    initialRender.SetScales(scale, endScale);
    initialRender.SetLifetime(ttl);
    initialRender.SetMaterial(initialMaterial);
    initialRender.SetBillboarding(ParticleEmitterDefinition::GetParticleEmitterDefinition(_name)->_isBillboarded);

    _particles.emplace_back(initialRender, initialState);
}

void ParticleEmitter::DestroyDeadEntities() {
    if(_particles.empty()) {
        return;
    }

    _particles.erase(
    std::remove_if(_particles.begin(), _particles.end(), [](const Particle& p) { return p.IsDead(); }), _particles.end());
}
