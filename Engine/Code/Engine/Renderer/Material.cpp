#include "Engine/Renderer/Material.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>

bool IsIntrinsic(std::filesystem::path& p) noexcept;

bool IsIntrinsic(std::filesystem::path& p) noexcept {
    return StringUtils::StartsWith(p.string(), "__");
}

namespace StringUtils {
    std::string to_string(const Material::TextureID& slot) noexcept {
        switch(slot) {
        case Material::TextureID::Diffuse: return "Diffuse";
        case Material::TextureID::Normal: return "Normal";
        case Material::TextureID::Displacement: return "Displacement";
        case Material::TextureID::Specular: return "Specular";
        case Material::TextureID::Occlusion: return "Occlusion";
        case Material::TextureID::Emissive: return "Emissive";
        case Material::TextureID::Custom1: return "Texture 1";
        case Material::TextureID::Custom2: return "Texture 2";
        case Material::TextureID::Custom3: return "Texture 3";
        case Material::TextureID::Custom4: return "Texture 4";
        case Material::TextureID::Custom5: return "Texture 5";
        case Material::TextureID::Custom6: return "Texture 6";
        case Material::TextureID::Custom7: return "Texture 7";
        case Material::TextureID::Custom8: return "Texture 8";
        case Material::TextureID::Custom9: return "Texture 9";
        case Material::TextureID::Custom10: return "Texture 10";
        case Material::TextureID::Custom11: return "Texture 11";
        case Material::TextureID::Custom12: return "Texture 12";
        case Material::TextureID::Custom13: return "Texture 13";
        case Material::TextureID::Custom14: return "Texture 14";
        case Material::TextureID::Custom15: return "Texture 15";
        case Material::TextureID::Custom16: return "Texture 16";
        case Material::TextureID::Custom17: return "Texture 17";
        case Material::TextureID::Custom18: return "Texture 18";
        case Material::TextureID::Custom19: return "Texture 19";
        case Material::TextureID::Custom20: return "Texture 20";
        case Material::TextureID::Custom21: return "Texture 21";
        case Material::TextureID::Custom22: return "Texture 22";
        case Material::TextureID::Custom23: return "Texture 23";
        case Material::TextureID::Custom24: return "Texture 24";
        case Material::TextureID::Custom25: return "Texture 25";
        case Material::TextureID::Custom26: return "Texture 26";
        case Material::TextureID::Custom27: return "Texture 27";
        case Material::TextureID::Custom28: return "Texture 28";
        case Material::TextureID::Custom29: return "Texture 29";
        case Material::TextureID::Custom30: return "Texture 30";
        case Material::TextureID::Custom31: return "Texture 31";
        case Material::TextureID::Custom32: return "Texture 32";
        case Material::TextureID::Custom33: return "Texture 33";
        case Material::TextureID::Custom34: return "Texture 34";
        case Material::TextureID::Custom35: return "Texture 35";
        case Material::TextureID::Custom36: return "Texture 36";
        case Material::TextureID::Custom37: return "Texture 37";
        case Material::TextureID::Custom38: return "Texture 38";
        case Material::TextureID::Custom39: return "Texture 39";
        case Material::TextureID::Custom40: return "Texture 40";
        case Material::TextureID::Custom41: return "Texture 41";
        case Material::TextureID::Custom42: return "Texture 42";
        case Material::TextureID::Custom43: return "Texture 43";
        case Material::TextureID::Custom44: return "Texture 44";
        case Material::TextureID::Custom45: return "Texture 45";
        case Material::TextureID::Custom46: return "Texture 46";
        case Material::TextureID::Custom47: return "Texture 47";
        case Material::TextureID::Custom48: return "Texture 48";
        case Material::TextureID::Custom49: return "Texture 49";
        case Material::TextureID::Custom50: return "Texture 50";
        case Material::TextureID::Custom51: return "Texture 51";
        case Material::TextureID::Custom52: return "Texture 52";
        case Material::TextureID::Custom53: return "Texture 53";
        case Material::TextureID::Custom54: return "Texture 54";
        case Material::TextureID::Custom55: return "Texture 55";
        case Material::TextureID::Custom56: return "Texture 56";
        case Material::TextureID::Custom57: return "Texture 57";
        case Material::TextureID::Custom58: return "Texture 58";
        default: return "UNKNOWN TEXTURE ID";
        }
    }
}

Material::Material() noexcept
: m_textures(CustomTextureIndexSlotOffset, nullptr) {
    auto* rs = ServiceLocator::get<IRendererService>();
    m_textures[0] = rs->GetTexture("__diffuse");
    m_textures[1] = rs->GetTexture("__normal");
    m_textures[2] = rs->GetTexture("__displacement");
    m_textures[3] = rs->GetTexture("__specular");
    m_textures[4] = rs->GetTexture("__occlusion");
    m_textures[5] = rs->GetTexture("__emissive");

    m_name += "_" + std::to_string(m_defaultNameId++);
}

Material::Material(const XMLElement& element) noexcept
: m_textures(CustomTextureIndexSlotOffset, nullptr) {
    auto* rs = ServiceLocator::get<IRendererService>();
    m_textures[0] = rs->GetTexture("__diffuse");
    m_textures[1] = rs->GetTexture("__normal");
    m_textures[2] = rs->GetTexture("__displacement");
    m_textures[3] = rs->GetTexture("__specular");
    m_textures[4] = rs->GetTexture("__occlusion");
    m_textures[5] = rs->GetTexture("__emissive");

    m_name += "_" + std::to_string(m_defaultNameId++);

    GUARANTEE_OR_DIE(LoadFromXml(element), "Material constructor failed to load.");
}

bool Material::LoadFromXml(const XMLElement& element) noexcept {
    namespace FS = std::filesystem;

    DataUtils::ValidateXmlElement(element, "material", "shader", "name", "lighting,textures");

    m_name = DataUtils::ParseXmlAttribute(element, "name", m_name);
    {
        const auto* xml_shader = element.FirstChildElement("shader");
        DataUtils::ValidateXmlElement(*xml_shader, "shader", "", "src");
        const auto file = DataUtils::ParseXmlAttribute(*xml_shader, "src", std::string{});
        FS::path shader_src(file);
        if(!StringUtils::StartsWith(shader_src.string(), "__")) {
            std::error_code ec{};
            shader_src = FS::canonical(shader_src, ec);
            const auto error_msg = std::format("Shader:\n{}\nReferenced in Material file \"{}\n could not be found.\nThe filesystem returned an error:\n{}\n", file, m_name, ec.message());
            GUARANTEE_OR_DIE(!ec, error_msg.c_str());
        }
        shader_src.make_preferred();
        auto* rs = ServiceLocator::get<IRendererService>();
        if(auto* shader = rs->GetShader(shader_src.string())) {
            m_shader = shader;
        } else {
            DebuggerPrintf(std::format("Shader: {}\n referenced in Material file \"{}\" did not already exist. Attempting to create from source...", shader_src, m_name));
            if(!rs->RegisterShader(shader_src.string())) {
                DebuggerPrintf("failed.\n");
                return false;
            }
            DebuggerPrintf("done.\n");
            if(shader = rs->GetShader(shader_src.string()); shader == nullptr) {
                if(shader = rs->GetShader(rs->GetShaderName(shader_src)); shader != nullptr) {
                    m_shader = shader;
                }
            }
        }
    }

    if(const auto* xml_lighting = element.FirstChildElement("lighting")) {
        DataUtils::ValidateXmlElement(*xml_lighting, "lighting", "", "", "specularIntensity,specularFactor,specularPower,glossFactor,emissiveFactor");
        //specularIntensity and specularFactor are synonyms
        if(const auto* xml_specInt = xml_lighting->FirstChildElement("specularIntensity")) {
            m_specularIntensity = DataUtils::ParseXmlElementText(*xml_specInt, m_specularIntensity);
        }
        if(const auto* xml_specFactor = xml_lighting->FirstChildElement("specularFactor")) {
            m_specularIntensity = DataUtils::ParseXmlElementText(*xml_specFactor, m_specularIntensity);
        }
        //specularPower and glossFactor are synonyms
        if(const auto* xml_specPower = xml_lighting->FirstChildElement("specularPower")) {
            m_specularPower = DataUtils::ParseXmlElementText(*xml_specPower, m_specularPower);
        }
        if(const auto* xml_glossFactor = xml_lighting->FirstChildElement("glossFactor")) {
            m_specularPower = DataUtils::ParseXmlElementText(*xml_glossFactor, m_specularPower);
        }
        if(const auto* xml_emissiveFactor = xml_lighting->FirstChildElement("emissiveFactor")) {
            m_emissiveFactor = DataUtils::ParseXmlElementText(*xml_emissiveFactor, m_emissiveFactor);
        }
    }

    if(const auto* xml_textures = element.FirstChildElement("textures")) {
        if(const auto* xml_diffuse = xml_textures->FirstChildElement("diffuse"); xml_diffuse) {
            LoadTexture(Material::TextureID::Diffuse, FS::path{DataUtils::ParseXmlAttribute(*xml_diffuse, "src", std::string{})});
        }

        if(const auto* xml_normal = xml_textures->FirstChildElement("normal"); xml_normal) {
            LoadTexture(Material::TextureID::Normal, FS::path{DataUtils::ParseXmlAttribute(*xml_normal, "src", std::string{})});
        }

        if(const auto* xml_displacement = xml_textures->FirstChildElement("displacement"); xml_displacement) {
            LoadTexture(Material::TextureID::Displacement, FS::path{DataUtils::ParseXmlAttribute(*xml_displacement, "src", std::string{})});
        }

        if(const auto* xml_specular = xml_textures->FirstChildElement("specular"); xml_specular) {
            LoadTexture(Material::TextureID::Specular, FS::path{DataUtils::ParseXmlAttribute(*xml_specular, "src", std::string{})});
        }

        if(const auto* xml_occlusion = xml_textures->FirstChildElement("occlusion"); xml_occlusion) {
            LoadTexture(Material::TextureID::Occlusion, FS::path{DataUtils::ParseXmlAttribute(*xml_occlusion, "src", std::string{})});
        }

        if(const auto* xml_emissive = xml_textures->FirstChildElement("emissive")) {
            LoadTexture(Material::TextureID::Emissive, FS::path{DataUtils::ParseXmlAttribute(*xml_emissive, "src", std::string{})});
        }

        {
            const auto numTextures = DataUtils::GetChildElementCount(*xml_textures, "texture");
            if(numTextures >= MaxCustomTextureSlotCount) {
                DebuggerPrintf(std::format("Max custom texture count exceeded. Cannot bind more than {} custom textures.", MaxCustomTextureSlotCount));
            }
            AddTextureSlots(numTextures);
        }

        DataUtils::ForEachChildElement(*xml_textures, "texture", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "texture", "", "index,src");
            const std::size_t index = CustomTextureIndexSlotOffset + DataUtils::ParseXmlAttribute(elem, std::string("index"), std::size_t{0u});
            if(index >= CustomTextureIndexSlotOffset + MaxCustomTextureSlotCount) {
                return;
            }
            LoadTexture(static_cast<TextureID>(index), FS::path{DataUtils::ParseXmlAttribute(elem, "src", std::string{})});
        });
    }
    return true;
}

void Material::LoadTexture(const TextureID& slotId, std::filesystem::path p) noexcept {
    SetTextureSlotToInvalid(slotId);
    if(!IsIntrinsic(p)) {
        std::error_code ec{};
        p = std::filesystem::canonical(p, ec);
        if(ec) {
            DebuggerPrintf(std::format("{} texture referenced in Material file \"{}\" could not be found. The filesystem returned an error: {}\n", StringUtils::to_string(slotId), m_name, ec.message()));
            return;
        }
    }
    p.make_preferred();
    const auto& p_str = p.string();
    bool empty_path = p.empty();
    auto* rs = ServiceLocator::get<IRendererService>();
    bool texture_not_loaded = rs->IsTextureNotLoaded(p_str);
    if(texture_not_loaded) {
        texture_not_loaded = rs->CreateTexture(p.string(), IntVector3::XY_Axis) ? false : true;
    }
    bool texture_not_exist = !empty_path && texture_not_loaded;
    bool invalid_src = empty_path || texture_not_exist;
    auto* tex = invalid_src ? rs->GetTexture("__invalid") : (rs->GetTexture(p_str));
    SetTextureSlot(slotId, tex);
}

void Material::SetTextureSlotToInvalid(const TextureID& slotId) noexcept {
    auto* rs = ServiceLocator::get<IRendererService>();
    auto* const invalid_tex = rs->GetTexture("__invalid");
    const auto slotAsIndex = TypeUtils::GetUnderlyingValue(slotId);
    m_textures[slotAsIndex] = invalid_tex;
}

void Material::AddTextureSlots(std::size_t count) noexcept {
    const auto old_size = m_textures.size();
    const auto new_size = (std::min)(old_size + MaxCustomTextureSlotCount, old_size + (std::min)(MaxCustomTextureSlotCount, count));
    m_textures.resize(new_size);
    for(std::size_t i = old_size; i < new_size; ++i) {
        m_textures[i] = nullptr;
    }
}

void Material::ClearTextureSlot(const TextureID& slotId) noexcept {
    const auto slotAsIndex = TypeUtils::GetUnderlyingValue(slotId);
    m_textures[slotAsIndex] = nullptr;
}

void Material::ClearAllTextureSlots() noexcept {
    for(auto& t : m_textures) {
        t = nullptr;
    }
}

void Material::AddTextureSlot() noexcept {
    AddTextureSlots(1);
}

std::string Material::GetName() const noexcept {
    return m_name;
}

Shader* Material::GetShader() const noexcept {
    return m_shader;
}

std::size_t Material::GetTextureCount() const noexcept {
    return m_textures.size();
}

Texture* Material::GetTexture(std::size_t i) const noexcept {
    return m_textures[i];
}

Texture* Material::GetTexture(const TextureID& id) const noexcept {
    return GetTexture(std::underlying_type_t<TextureID>(id));
}

float Material::GetSpecularIntensity() const noexcept {
    return m_specularIntensity;
}

float Material::GetGlossyFactor() const noexcept {
    return m_specularPower;
}

float Material::GetEmissiveFactor() const noexcept {
    return m_emissiveFactor;
}

Vector3 Material::GetSpecGlossEmitFactors() const noexcept {
    return Vector3(GetSpecularIntensity(), GetGlossyFactor(), GetEmissiveFactor());
}

void Material::SetFilepath(const std::filesystem::path& p) noexcept {
    m_filepath = p;
}

const std::filesystem::path& Material::GetFilepath() const noexcept {
    return m_filepath;
}

void Material::SetTextureSlot(const TextureID& slotId, Texture* texture) noexcept {
    using underlying = std::underlying_type_t<TextureID>;
    const auto slotAsIndex = static_cast<underlying>(slotId);
    const auto count = GetTextureCount();
    if(count <= slotAsIndex) {
        AddTextureSlots(1 + count - slotAsIndex);
    }
    m_textures[slotAsIndex] = texture;
}
