#include "Engine/Physics/Particles/ParticleEffect.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Physics/Particles/ParticleEmitterDefinition.hpp"
#include "Engine/Physics/Particles/ParticleEffectDefinition.hpp"

#include <algorithm>

ParticleEffect::ParticleEffect(const XMLElement& element) noexcept
{
    LoadFromXml(element);
}

ParticleEffect::ParticleEffect(std::string definitionName) noexcept
    : m_definitionName{definitionName}
{
    auto* definition = ParticleEffectDefinition::GetDefinition(m_definitionName);
    m_emitters.reserve(definition->m_emitter_names.size());
    for(const auto& emitter_name : definition->m_emitter_names) {
        m_emitters.emplace_back(emitter_name);
        auto& recent_emitter = m_emitters.back();
        recent_emitter.parent_effect = this;
    }
}

void ParticleEffect::PlayOnce(bool value) {
    m_destroy_on_finish = value;
    SetPlay(true);
}

bool ParticleEffect::PlayOnce() const {
    return m_destroy_on_finish;
}

bool ParticleEffect::IsDestroyedOnFinish() const {
    return m_destroy_on_finish;
}

bool ParticleEffect::IsPlaying() const {
    return m_is_playing;
}

bool ParticleEffect::IsFinished() const {
    const auto lifetime = ParticleEffectDefinition::GetDefinition(m_definitionName)->m_lifetime;
    const auto oldest_emitter = std::max_element(m_emitters.begin(), m_emitters.end(), [](const ParticleEmitter& a, const ParticleEmitter& b) { return a.GetAge() < b.GetAge(); });
    return oldest_emitter->GetAge() > lifetime && !oldest_emitter->HasAliveParticles();
}

void ParticleEffect::Stop() {
    SetPlay(false);
}

void ParticleEffect::BeginFrame() {
    for(auto& emitter : m_emitters) {
        emitter.BeginFrame();
    }
}

void ParticleEffect::Update(float time, float deltaSeconds) {
    position += velocity * deltaSeconds;
    for(auto& emitter : m_emitters) {
        if(m_is_playing) {
            emitter.Update(time, deltaSeconds);
        }
    }
    if(IsFinished()) {
        SetPlay(false);
    }
}

void ParticleEffect::Render() const {
    for(auto& emitter : m_emitters) {
        emitter.Render();
    }
}

void ParticleEffect::EndFrame() {
    for(auto& emitter : m_emitters) {
        emitter.EndFrame();
    }

    bool didEffectFinish = IsFinished();
    if(didEffectFinish) {
        if(IsDestroyedOnFinish()) {
            PlayOnce(false);
            SetPlay(false);
        }
    }
}

const std::string& ParticleEffect::GetName() const {
    return m_definitionName;
}

const std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() const {
    return m_emitters;
}

std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() {
    return m_emitters;
}

void ParticleEffect::LoadFromXml(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "effect", "", "name,definition");

    std::string name = "UNNAMED_PARTICLE_EFFECT";
    name = DataUtils::ParseXmlAttribute(element, "name", name);
    std::string def_src{};
    def_src = DataUtils::ParseXmlAttribute(element, "definition", def_src);

    m_definitionName = name;
    const auto* definition = ParticleEffectDefinition::GetDefinition(name);
    const auto error_str = std::format("ParticleEffect: Referenced particle effect \"{}\" does not already exist.", name);
    GUARANTEE_OR_DIE(definition, error_str.c_str());

    m_emitters.reserve(definition->m_emitter_names.size());
    for(auto& emitter_name : definition->m_emitter_names) {
        auto* emitter_def = ParticleEmitterDefinition::GetParticleEmitterDefinition(emitter_name);
        GUARANTEE_OR_DIE(emitter_def, "ParticleEffect: Referenced particle emitter does not already exist.");
    }
}

float ParticleEffect::GetLongestLifetime() const {
    auto first = std::begin(m_emitters);
    auto last = std::end(m_emitters);
    if(auto found_iter = first; found_iter != last) {
        while(++first != last) {
            if(first->GetLifetime() > found_iter->GetLifetime()) {
                found_iter = first;
            }
        }
        return found_iter->GetLifetime();
    }
    return 0.0f;
}

void ParticleEffect::SetPlay(bool value) {
    m_is_playing = value;
    if(m_is_playing) {
        for(auto& emitter : m_emitters) {
            emitter.MakeAlive();
        }
    }
}
