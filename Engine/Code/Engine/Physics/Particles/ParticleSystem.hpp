#pragma once

#include <filesystem>
#include <string>

class ParticleEffect;
class ParticleEffectDefinition;

class ParticleSystem {
public:
    ParticleEffectDefinition* GetEffectDefinition(const std::string& name);
    void RegisterEffectsFromFolder(std::filesystem::path folderpath, bool recursive = false);
    bool RegisterEffectFromFile(const std::filesystem::path& filepath);

protected:
private:
};
