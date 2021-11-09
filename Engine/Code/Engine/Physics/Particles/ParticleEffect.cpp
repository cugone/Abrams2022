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
    : _definitionName{definitionName}
{
    auto* definition = ParticleEffectDefinition::GetDefinition(_definitionName);
    _emitters.reserve(definition->_emitter_names.size());
    for(const auto& emitter_name : definition->_emitter_names) {
        _emitters.emplace_back(emitter_name);
        auto& recent_emitter = _emitters.back();
        recent_emitter.parent_effect = this;
    }
}

void ParticleEffect::PlayOnce(bool value) {
    _destroy_on_finish = value;
    SetPlay(true);
}

bool ParticleEffect::PlayOnce() const {
    return _destroy_on_finish;
}

bool ParticleEffect::IsDestroyedOnFinish() const {
    return _destroy_on_finish;
}

bool ParticleEffect::IsPlaying() const {
    return _is_playing;
}

bool ParticleEffect::IsFinished() const {
    const auto lifetime = ParticleEffectDefinition::GetDefinition(_definitionName)->_lifetime;
    const auto oldest_emitter = std::max_element(_emitters.begin(), _emitters.end(), [](const ParticleEmitter& a, const ParticleEmitter& b) { return a.GetAge() < b.GetAge(); });
    return oldest_emitter->GetAge() > lifetime && !oldest_emitter->HasAliveParticles();
}

void ParticleEffect::Stop() {
    SetPlay(false);
}

void ParticleEffect::BeginFrame() {
    for(auto& emitter : _emitters) {
        emitter.BeginFrame();
    }
}

void ParticleEffect::Update(float time, float deltaSeconds) {
    position += velocity * deltaSeconds;
    for(auto& emitter : _emitters) {
        if(_is_playing) {
            emitter.Update(time, deltaSeconds);
        }
    }
    if(IsFinished()) {
        SetPlay(false);
    }
}

void ParticleEffect::Render() const {
    for(auto& emitter : _emitters) {
        emitter.Render();
    }
}

void ParticleEffect::EndFrame() {
    for(auto& emitter : _emitters) {
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
    return _definitionName;
}

const std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() const {
    return _emitters;
}

std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() {
    return _emitters;
}

void ParticleEffect::LoadFromXml(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "effect", "", "name,definition");

    std::string name = "UNNAMED_PARTICLE_EFFECT";
    name = DataUtils::ParseXmlAttribute(element, "name", name);
    std::string def_src{};
    def_src = DataUtils::ParseXmlAttribute(element, "definition", def_src);

    _definitionName = name;
    const auto* definition = ParticleEffectDefinition::GetDefinition(name);
    std::string err = "ParticleEffect: Referenced particle effect \"";
    err += name;
    err += "\" does not already exist.";
    GUARANTEE_OR_DIE(definition, err.c_str());

    _emitters.reserve(definition->_emitter_names.size());
    for(auto& emitter_name : definition->_emitter_names) {
        auto* emitter_def = ParticleEmitterDefinition::GetParticleEmitterDefinition(emitter_name);
        GUARANTEE_OR_DIE(emitter_def, "ParticleEffect: Referenced particle emitter does not already exist.");
    }
}

float ParticleEffect::GetLongestLifetime() const {
    auto first = std::begin(_emitters);
    auto last = std::end(_emitters);
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
    _is_playing = value;
    if(_is_playing) {
        for(auto& emitter : _emitters) {
            emitter.MakeAlive();
        }
    }
}
