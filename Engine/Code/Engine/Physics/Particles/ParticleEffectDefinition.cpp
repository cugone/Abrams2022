#include "Engine/Physics/Particles/ParticleEffectDefinition.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Physics/Particles/ParticleEmitterDefinition.hpp"

ParticleEffectDefinition* ParticleEffectDefinition::CreateOrGetDefinition(const std::string& name, const XMLElement& element) {
    ParticleEffectDefinition* def = nullptr;
    def = GetDefinition(name);
    if(!def) {
        def = CreateAndGetDefinition(name, element);
    }
    return def;
}

ParticleEffectDefinition* ParticleEffectDefinition::CreateAndGetDefinition(const std::string& name, const XMLElement& element) {
    ParticleEffectDefinition* def = nullptr;
    if(LoadDefinition(element)) {
        def = GetDefinition(name);
    }
    return def;
}

ParticleEffectDefinition* ParticleEffectDefinition::GetDefinition(const std::string& name) {
    auto found_emitter = s_particleEffectDefinitions.find(name);
    if(found_emitter != s_particleEffectDefinitions.end()) {
        return (*found_emitter).second.get();
    }
    return nullptr;
}

bool ParticleEffectDefinition::LoadDefinition(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "effect", "emitter", "name", "sound");

    auto definition = std::make_unique<ParticleEffectDefinition>();
    definition->_name = DataUtils::ParseXmlAttribute(element, "name", definition->_name);
    if(auto found_iter = s_particleEffectDefinitions.find(definition->_name); found_iter != std::end(s_particleEffectDefinitions)) {
        return false;
    }
    definition->_emitter_names = [&element]() {
        const auto emitter_count = DataUtils::GetChildElementCount(element, "emitter");
        std::vector<std::string> emitter_names{};
        emitter_names.reserve(emitter_count);
        DataUtils::ForEachChildElement(element, "emitter", [&emitter_names](const XMLElement& elem) {
            std::string name = DataUtils::ParseXmlAttribute(elem, "name", std::string{"UNNAMED_PARTICLE_EMITTER"});
            ParticleEmitterDefinition::CreateAndRegisterParticleEmitterDefinition(elem);
            emitter_names.push_back(name);
            });
        return emitter_names;
    }(); //IIIL

    if(auto xml_sound = element.FirstChildElement("sound"); xml_sound) {
        DataUtils::ValidateXmlElement(*xml_sound, "sound", "", "src");
        std::string sound_src = DataUtils::ParseXmlAttribute(*xml_sound, "src", std::string{});
        if(!sound_src.empty()) {
            definition->_soundSrc = sound_src;
            definition->_hasSound = true;
        }
    }
    definition->_lifetime = GetLongestLifetime(*definition);
    const std::string name = definition->_name;
    s_particleEffectDefinitions.insert_or_assign(name, std::move(definition));
    return true;
}

const std::string& ParticleEffectDefinition::GetName() const {
    return _name;
}

bool ParticleEffectDefinition::HasSound() const {
    return _hasSound;
}

const std::string& ParticleEffectDefinition::GetSoundSource() const {
    return _soundSrc;
}

bool ParticleEffectDefinition::DestroyOnFinish() const {
    return _destroyOnFinish;
}

float ParticleEffectDefinition::GetLongestLifetime() {
    return GetLongestLifetime(*this);
}

float ParticleEffectDefinition::GetLongestLifetime(const ParticleEffectDefinition& def) {
    float longest_lifetime = def._lifetime;
    for(auto& emitter_name : def._emitter_names) {
        auto* emitter = ParticleEmitterDefinition::GetParticleEmitterDefinition(emitter_name);
        if(longest_lifetime < emitter->_lifetime) {
            longest_lifetime = emitter->_lifetime;
        }
    }
    return longest_lifetime;
}
