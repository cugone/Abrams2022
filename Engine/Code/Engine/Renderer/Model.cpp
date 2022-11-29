#include "Engine/Renderer/Model.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Obj.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include <filesystem>
#include <format>

bool Model::Load(std::filesystem::path filepath) noexcept {
    return Import(filepath);
}

bool Model::Import(std::filesystem::path filepath) noexcept {
    const auto ext = StringUtils::ToLowerCase(filepath.extension().string());
/*
    {
        std::error_code ec{};
        if(const auto exists = std::filesystem::exists(filepath, ec); !exists) {
            DebuggerPrintf(std::vformat("Model import failed. File not found: \"{}\".\n", std::make_format_args(filepath.string())));
            return false;
        }
    }
*/
    if(!FileUtils::IsSafeReadPath(filepath)) {
        DebuggerPrintf(std::vformat("Model import failed. \"{}\" is not a safe read path.\n", std::make_format_args(filepath.string())));
        return false;
    }
    if(ext != ".obj") {
        DebuggerPrintf(std::format("Model import failed. \"{}\" is not a supported model type. Must be .obj.\n", filepath.string()));
        return false;
    }
    
    FileUtils::Obj obj;
    if(!obj.Load(filepath)) {
        DebuggerPrintf(std::format("Model import failed. \"{}\" could not be loaded.\n", filepath.string()));
        return false;
    }
    m_meshes.emplace_back(Mesh::Builder{obj.GetVbo(), obj.GetIbo()});
    return true;
}
