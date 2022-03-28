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
    definition->m_name = DataUtils::ParseXmlAttribute(element, "name", definition->m_name);
    if(auto found_iter = s_particleEffectDefinitions.find(definition->m_name); found_iter != std::end(s_particleEffectDefinitions)) {
        return false;
    }
    definition->m_emitter_names = [&element]() {
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
            definition->m_soundSrc = sound_src;
            definition->m_hasSound = true;
        }
    }
    definition->m_lifetime = GetLongestLifetime(*definition);
    const std::string name = definition->m_name;
    s_particleEffectDefinitions.insert_or_assign(name, std::move(definition));
    return true;
}

const std::string& ParticleEffectDefinition::GetName() const {
    return m_name;
}

bool ParticleEffectDefinition::HasSound() const {
    return m_hasSound;
}

const std::string& ParticleEffectDefinition::GetSoundSource() const {
    return m_soundSrc;
}

bool ParticleEffectDefinition::DestroyOnFinish() const {
    return m_destroyOnFinish;
}

float ParticleEffectDefinition::GetLongestLifetime() {
    return GetLongestLifetime(*this);
}

float ParticleEffectDefinition::GetLongestLifetime(const ParticleEffectDefinition& def) {
    float longest_lifetime = def.m_lifetime;
    for(auto& emitter_name : def.m_emitter_names) {
        auto* emitter = ParticleEmitterDefinition::GetParticleEmitterDefinition(emitter_name);
        if(longest_lifetime < emitter->m_lifetime) {
            longest_lifetime = emitter->m_lifetime;
        }
    }
    return longest_lifetime;
}
