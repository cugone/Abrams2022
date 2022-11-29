#pragma once

#include "Engine/Renderer/Mesh.hpp"

#include <filesystem>
#include <vector>

class Model {
public:
    [[nodiscard]] bool Load(std::filesystem::path filepath) noexcept;
    [[nodiscard]] bool Import(std::filesystem::path filepath) noexcept;

protected:
private:
    std::vector<Mesh> m_meshes{};
};
