#include "Engine/Core/MtlReader.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

[[nodiscard]] std::optional<Rgba> GetColor(const std::string& cur_line) noexcept {
    std::stringstream K_ss{};
    const auto color_statements = StringUtils::Split(cur_line.substr(3), ' ');
    switch(color_statements.size()) {
    case 4: {
        if(auto xyz_str = color_statements[0]; xyz_str == "xyz") {
            const auto convert_xyz_sRGB = [&](float x, float y, float z) -> Rgba {
                //See https://en.wikipedia.org/wiki/SRGB#Specification_of_the_transformation
                //for explanation of values.
                Matrix4 transform{};
                transform.SetXComponents(Vector4{+3.2406f, -1.5372f, -0.4986f, 0.0f});
                transform.SetYComponents(Vector4{-0.9689f, +1.8758f, +0.0415f, 0.0f});
                transform.SetZComponents(Vector4{+0.0557f, -0.2040f, +1.0570f, 0.0f});
                Vector3 xyz_values{x, y, z};
                auto vrgba = Vector4(transform.TransformVector(xyz_values), 1.0f);
                Rgba result{};
                result.SetFromFloats({vrgba.x, vrgba.y, vrgba.z, vrgba.w});
                return result;
            };
            return std::make_optional(convert_xyz_sRGB(std::stof(color_statements[1]), std::stof(color_statements[2]), std::stof(color_statements[3])));
        } else {
            DebuggerPrintf("MtlReader: Invalid number of arguments.\n");
            return {};
        }
    }
    case 3: {
        if(auto is_spectral_file = color_statements[0] == "spectral") {
            DebuggerPrintf("MtlReader does not support spectral files. Yet.\n");
            return {};
        }
        K_ss << cur_line.substr(3);
        K_ss.clear();
        K_ss.seekg(0);
        K_ss.seekp(0);
        float r{};
        float g{};
        float b{};
        K_ss >> r >> g >> b;
        return std::make_optional(Rgba{r, g, b, 1.0f});
    }
    case 2: {
        K_ss << cur_line.substr(3);
        K_ss.clear();
        K_ss.seekg(0);
        K_ss.seekp(0);
        float r{};
        float g{};
        K_ss >> r >> g;
        return std::make_optional(Rgba{r, g, 0.0f, 1.0f});
    }
    case 1: {
        K_ss << cur_line.substr(3);
        K_ss.clear();
        K_ss.seekg(0);
        K_ss.seekp(0);
        float r{};
        K_ss >> r;
        return std::make_optional(Rgba{r, r, r, 1.0f});
    }
    default: {
        DebuggerPrintf("Ill-formed Mtl file.\n");
        return {};
    }
    }
}

std::optional<std::filesystem::path> GetTexturePath(const std::filesystem::path& filepath, const std::string& entryElement) noexcept {
    auto path = std::filesystem::path{entryElement};
    if(path.is_relative()) {
        path = std::filesystem::canonical(filepath.parent_path() / path);
    }
    path.make_preferred();
    //if(!std::filesystem::exists(path)) {
    //    DebuggerPrintf("MtlReader: Diffuse texture not found.\n");
    //    PrintErrorToDebugger(filepath, "map_Kd", line_index);
    //    return {};
    //}
    return std::make_optional(path);
}

namespace FileUtils {

    template<typename T>
    [[nodiscard]] T GetSingleValue(const std::string& cur_line, const std::size_t element_length) noexcept {
        auto value = T{};
        std::stringstream d_ss{};
        d_ss << cur_line.substr(element_length);
        d_ss.clear();
        d_ss.seekg(0);
        d_ss.seekp(0);
        d_ss >> value;
        return value;
    }

    bool MtlReader::Load(std::filesystem::path filepath) noexcept {
        PROFILE_LOG_SCOPE_FUNCTION();

        namespace FS = std::filesystem;
        bool not_exist = !FS::exists(filepath);
        std::string valid_extension = ".mtl";
        bool not_mtl = StringUtils::ToLowerCase(filepath.extension().string()) != valid_extension;
        bool invalid = not_exist || not_mtl;
        std::string error_msg = filepath.string() + "does not exist or is not a .mtl file.\n";
        GUARANTEE_OR_DIE(!invalid, error_msg);
        filepath = FS::canonical(filepath);
        filepath.make_preferred();
        return Parse(filepath);
    }

    bool MtlReader::Parse(std::filesystem::path filepath) noexcept {
        PROFILE_LOG_SCOPE_FUNCTION();
        if(auto buffer = FileUtils::ReadBinaryBufferFromFile(filepath); buffer.has_value()) {
            if(std::stringstream ss{}; ss.write(reinterpret_cast<const char*>(buffer->data()), buffer->size())) {
                buffer->clear();
                buffer->shrink_to_fit();
                ss.clear();
                ss.seekg(ss.beg);
                ss.seekp(ss.beg);
                std::string cur_line{};
                unsigned long long line_index = 0ull;

                while(std::getline(ss, cur_line, '\n')) {
                    ++line_index;
                    cur_line = cur_line.substr(0, cur_line.find_first_of('#'));
                    if(cur_line.empty()) {
                        continue;
                    }
                    cur_line = StringUtils::TrimWhitespace(cur_line);
                    if(StringUtils::StartsWith(cur_line, "newmtl ")) {
                        m_materialName = cur_line.substr(7);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "d ")) {
                        m_transparencyWeight = GetSingleValue<float>(cur_line, std::size_t{2u});
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Ns ")) {
                        m_specularPower = GetSingleValue<float>(cur_line, std::size_t{3u});
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Ka ")) {
                        m_ambientColor = GetColor(cur_line).value_or(Rgba::Black);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Kd ")) {
                        m_diffuseColor = GetColor(cur_line).value_or(Rgba::White);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Ks ")) {
                        m_specularColor = GetColor(cur_line).value_or(Rgba::Black);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Ke ")) {
                        m_emissiveColor = GetColor(cur_line).value_or(Rgba::Black);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Tf ")) {
                        m_transmissionFilterColor = GetColor(cur_line).value_or(Rgba::White);
                        m_transmissionFilterColor.InvertRGB();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "Ni ")) {
                        m_indexOfRefraction = std::clamp(GetSingleValue<float>(cur_line, std::size_t{3u}), 0.001f, 10.0f);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "sharpness ")) {
                        m_sharpness = std::clamp(GetSingleValue<int>(cur_line, std::size_t{10u}), 0, 1000);
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "map_Ka ")) {
                        m_ambientTexture = GetTexturePath(filepath, cur_line.substr(7)).value_or("__invalid").string();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "map_Kd ")) {
                        m_diffuseTexture = GetTexturePath(filepath, cur_line.substr(7)).value_or("__invalid").string();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "map_Ks ")) {
                        m_specularTexture = GetTexturePath(filepath, cur_line.substr(7)).value_or("__invalid").string();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "map_Ke ")) {
                        m_emissiveTexture = GetTexturePath(filepath, cur_line.substr(7)).value_or("__invalid").string();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "map_Kn ")) {
                        m_normalTexture = GetTexturePath(filepath, cur_line.substr(7)).value_or("__invalid").string();
                        continue;
                    }
                    if(StringUtils::StartsWith(cur_line, "illum ")) {
                        //TODO: Implement MtlReader illum modes.
                        using underlying_t = std::underlying_type_t<IlluminationModel>;
                        m_illuminationModel = static_cast<IlluminationModel>(static_cast<underlying_t>(GetSingleValue<int>(cur_line, std::size_t{6u})));
                        continue;
                    }
                }
                return true;
            }
        }
        return false;
    }

    std::unique_ptr<Material> MtlReader::Export(Renderer& /*renderer*/) const noexcept {
        //TODO: Export .mtl as Material object
        return {};
    }

} // namespace FileUtils

void PrintErrorToDebugger(std::filesystem::path filepath, std::string_view elementType, unsigned long long line_index) noexcept {
    namespace FS = std::filesystem;
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    DebuggerPrintf("%s(%lld): Invalid %s\n", filepath.string().c_str(), line_index, elementType.data());
}
