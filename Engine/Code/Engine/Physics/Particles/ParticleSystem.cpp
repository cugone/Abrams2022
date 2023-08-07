#include "Engine/Physics/Particles/ParticleSystem.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Physics/Particles/ParticleEffectDefinition.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>

ParticleEffectDefinition* ParticleSystem::GetEffectDefinition(const std::string& name) {
    namespace FS = std::filesystem;
    FS::path p(name);
    if(auto found_effect = ParticleEffectDefinition::s_particleEffectDefinitions.find(p.string()); found_effect == std::end(ParticleEffectDefinition::s_particleEffectDefinitions)) {
        return nullptr;
    } else {
        return found_effect->second.get();
    }
}

void ParticleSystem::RegisterEffectsFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        DebuggerPrintf(std::format("Attempting to Register Particle Effect from unknown path: {}\n", FS::absolute(folderpath).string()));
        return;
    }
    {
        std::error_code ec{};
        folderpath = FS::canonical(folderpath, ec);
        if(ec || !FileUtils::IsSafeReadPath(folderpath)) {
            DebuggerPrintf(std::format("File: {} is inaccessible.\n", folderpath.string()));
            return;
        }
    }
    folderpath.make_preferred();
    auto cb =
    [this](const FS::path& p) {
        const auto pathAsString = p.string();
        if(!RegisterEffectFromFile(p)) {
            DebuggerPrintf(std::format("Failed to load particle effect at {}\n", pathAsString));
        }
    };
    FileUtils::ForEachFileInFolder(folderpath, ".effect", cb, recursive);
}

bool ParticleSystem::RegisterEffectFromFile(const std::filesystem::path& filepath) {
    namespace FS = std::filesystem;
    FS::path p(filepath);
    tinyxml2::XMLDocument doc;
    auto load_result = doc.LoadFile(p.string().c_str());
    bool success = load_result == tinyxml2::XML_SUCCESS;
    if(success) {
        ParticleEffectDefinition::LoadDefinition(*doc.RootElement());
        return success;
    }
    return success;
}

