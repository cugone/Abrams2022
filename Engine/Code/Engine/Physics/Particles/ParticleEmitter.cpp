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
: m_name(name) {
    const auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name);
    if(definition->m_spawnPerSecond > 0.0f) {
        m_spawnClock.SetFrequency(static_cast<unsigned int>(definition->m_spawnPerSecond));
    } else {
        m_spawnClock.SetSeconds(TimeUtils::FPSeconds{0.016f});
    }
}

void ParticleEmitter::Initialize() {
    /* DO NOTHING */
}

void ParticleEmitter::BeginFrame() {
    /* DO NOTHING */
}

void ParticleEmitter::Prewarm(TimeUtils::FPSeconds secondsToWarm) {
    m_isWarming = true;
    Stopwatch oven;
    oven.SetSeconds(secondsToWarm);
    float time = 0.0f;
    constexpr float deltaSeconds = 1.0f / 60.0f;
    while(!oven.Check()) {
        Update(time, deltaSeconds);
        time += deltaSeconds;
    }
    m_isWarming = false;
}

void ParticleEmitter::Update(float time, float deltaSeconds) {
    auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name);
    if(m_age < definition->m_lifetime) {
        if(!m_isWarming) {
            m_age += deltaSeconds;
        }
        auto emitter_pos_def = definition->m_emitterPositionDefinition;
        Vector3 new_particle_position = parent_effect->position;
        switch(emitter_pos_def.type) {
        case EmitterType::Point:
            new_particle_position += definition->m_position;
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

        auto emitter_vel_def = definition->m_emitterVelocityDefinition;
        Vector3 new_particle_velocity = parent_effect->velocity;
        switch(emitter_vel_def.type) {
        case EmitterType::Point:
            new_particle_velocity += definition->m_velocity;
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

        Rgba c = definition->m_particleRenderState.GetStartColor();
        Rgba ec = definition->m_particleRenderState.GetEndColor();
        Vector3 s = definition->m_particleRenderState.GetStartScale();
        Vector3 es = definition->m_particleRenderState.GetEndScale();
        auto particle_count = m_spawnClock.DecrementAll();
        auto& renderer = ServiceLocator::get<IRendererService>();
        for(unsigned int i = 0; i < particle_count; ++i) {
            SpawnParticle(new_particle_position, new_particle_velocity, definition->m_particleLifetime, c, ec, s, es, renderer.GetMaterial(definition->m_materialName), definition->m_mass);
        }
    }
    UpdateParticles(time, deltaSeconds);
}

void ParticleEmitter::Render() const {
    const auto p = Matrix4::CreateTranslationMatrix(parent_effect->position);
    const auto t = Matrix4::CreateTranslationMatrix(ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name)->m_position);
    const auto s = Matrix4::I;
    const auto r = Matrix4::I;

    const auto pointlight_model = Matrix4::MakeRT(p, Matrix4::MakeSRT(s, r, t));

    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetModelMatrix(pointlight_model);

    for(auto& particle : m_particles) {
        particle.Render(m_builder);
    }
    Mesh::Render(m_builder);
}

void ParticleEmitter::EndFrame() {
    m_particles.erase(std::remove_if(std::begin(m_particles), std::end(m_particles), [](const Particle& a) { return a.IsDead(); }), std::end(m_particles));
}

float ParticleEmitter::GetAge() const {
    return m_age;
}

float ParticleEmitter::GetLifetime() const {
    return ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name)->m_lifetime;
}

std::size_t ParticleEmitter::GetParticleCount() const {
    return m_particles.size();
}

bool ParticleEmitter::HasAliveParticles() const {
    return !m_particles.empty();
}

void ParticleEmitter::LoadFromXML(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "emitter", "", "name");
    const auto name = DataUtils::ParseXmlAttribute(element, "name", std::string{});
    m_spawnClock.SetFrequency(static_cast<unsigned int>(ParticleEmitterDefinition::GetParticleEmitterDefinition(name)->m_spawnPerSecond));
}

bool ParticleEmitter::IsDead() const {
    return !IsAlive();
}

bool ParticleEmitter::IsAlive() const {
    return m_age > 0.0f && HasAliveParticles();
}
void ParticleEmitter::Kill() {
    m_age = GetLifetime();
}

void ParticleEmitter::MakeAlive() {
    m_age = 0.0f;
    m_spawnClock.Reset();
}

Mesh::Builder& ParticleEmitter::GetMeshBuilder() noexcept {
    return m_builder;
}

void ParticleEmitter::UpdateParticles(float time, float deltaSeconds) {
    std::sort(std::begin(m_particles), std::end(m_particles));
    auto* definition = ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name);
    const auto loc = Matrix4::CreateTranslationMatrix(definition->m_position);
    auto& renderer = ServiceLocator::get<IRendererService>();
    const auto billboard = definition->m_isBillboarded ? renderer.GetCamera().GetInverseViewMatrix() : Matrix4::I;
    const auto result = Matrix4::MakeRT(billboard, loc);
    for(Particle& p : m_particles) {
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
    initialRender.SetBillboarding(ParticleEmitterDefinition::GetParticleEmitterDefinition(m_name)->m_isBillboarded);

    m_particles.emplace_back(initialRender, initialState);
}

void ParticleEmitter::DestroyDeadEntities() {
    if(m_particles.empty()) {
        return;
    }

    m_particles.erase(
    std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.IsDead(); }), m_particles.end());
}
