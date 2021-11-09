#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <string>
#include <vector>

class ParticleEffectDefinition {
public:
    static inline std::map<std::string, std::unique_ptr<class ParticleEffectDefinition>> s_particleEffectDefinitions{};

    static ParticleEffectDefinition* CreateOrGetDefinition(const std::string& name, const XMLElement& element);
    static ParticleEffectDefinition* GetDefinition(const std::string& name);
    static bool LoadDefinition(const XMLElement& element);
    static ParticleEffectDefinition* CreateAndGetDefinition(const std::string& name, const XMLElement& element);

    const std::string& GetName() const;
    bool HasSound() const;
    const std::string& GetSoundSource() const;
    bool DestroyOnFinish() const;

    static float GetLongestLifetime(const ParticleEffectDefinition& def);
    float GetLongestLifetime();

private:
    std::string _name = "UNNAMED_PARTICLE_EFFECT";
    std::string _soundSrc{};
    std::vector<std::string> _emitter_names;
    float _lifetime = -1.0f;
    bool _destroyOnFinish = false;
    bool _hasSound = false;

    friend class ParticleEffect;
};
