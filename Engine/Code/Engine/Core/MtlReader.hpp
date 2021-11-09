#pragma once

#include "Engine/Core/Rgba.hpp"

#include <filesystem>
#include <vector>

class Renderer;
class Material;

namespace FileUtils {

class MtlReader {
public:
    MtlReader() noexcept = default;
    MtlReader(const MtlReader& other) = default;
    MtlReader(MtlReader&& other) = default;
    MtlReader& operator=(const MtlReader& rhs) = default;
    MtlReader& operator=(MtlReader&& rhs) = default;
    ~MtlReader() = default;

    [[nodiscard]] bool Load(std::filesystem::path filepath) noexcept;
    [[nodiscard]] bool Parse(std::filesystem::path filepath) noexcept;
    [[nodiscard]] bool Parse() noexcept;

    [[nodiscard]] std::vector<Material*> GetMaterials() noexcept;

    [[nodiscard]] std::unique_ptr<Material> Export(Renderer& renderer) const noexcept;

    std::string m_materialName{"UNNAMED_MATERIAL"};
    std::string m_diffuseTexture{"__diffuse"};
    std::string m_ambientTexture{"__ambient"};
    std::string m_specularTexture{"__specular"};
    std::string m_emissiveTexture{"__emissive"};
    std::string m_normalTexture{"__normal"};

    Rgba m_ambientColor{Rgba::Black};
    Rgba m_diffuseColor{Rgba::White};
    Rgba m_specularColor{Rgba::Black};
    Rgba m_emissiveColor{Rgba::Black};
    Rgba m_transmissionFilterColor{Rgba::White};
    float m_specularPower{2.0f};
    float m_specularExponent{8.0f};
    float m_transparencyWeight{1.0f};
    float m_indexOfRefraction{1.0f};
    int m_sharpness{60};
protected:
private:
    enum class IlluminationModel : uint8_t {
        ColorNoAmbient,
        ColorAmbient,
        Highlight,
        RayTrace,
        GlassRayTrace,
        FresnelRayTrace,
        RefractionRayTrace,
        RefractionFresnelRayTrace,
        ReflectionNoRayTrace,
        GlassNoRayTrace,
        ShadowsOnInvisible
    };
    struct IlluminationOptions {
        uint8_t color : 1;
        uint8_t ambient : 1;
        uint8_t highlight : 1;
        uint8_t reflection : 1;
        uint8_t raytrace : 1;
        uint8_t transparency : 1;
        uint8_t glass : 1;
        uint8_t refraction : 1;
        uint8_t fresnel : 1;
        uint8_t castOnInvisible : 1;
    };
    IlluminationOptions m_lightOptions{};
    IlluminationModel m_illuminationModel{IlluminationModel::ColorNoAmbient};
};

} // namespace FileUtils
