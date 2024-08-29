#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "Engine/Services/ServiceLocator.hpp"

#include <string>
#include <sstream>

SpriteSheet::SpriteSheet(const XMLElement& elem) noexcept {
    LoadFromXml(elem);
}

SpriteSheet::SpriteSheet(Texture* texture, int tilesWide, int tilesHigh) noexcept
: m_spriteSheetTexture(texture)
, m_spriteLayout(tilesWide, tilesHigh) {
    /* DO NOTHING */
}

SpriteSheet::SpriteSheet(const std::filesystem::path& texturePath, int tilesWide, int tilesHigh) noexcept
: m_spriteSheetTexture(ServiceLocator::get<IRendererService>()->CreateOrGetTexture(texturePath, IntVector3::XY_Axis))
, m_spriteLayout(tilesWide, tilesHigh) {
    /* DO NOTHING */
}

AABB2 SpriteSheet::GetTexCoordsFromSpriteCoords(int spriteX, int spriteY) const noexcept {
    const auto texCoords = Vector2{1.0f / m_spriteLayout.x, 1.0f / m_spriteLayout.y};

    const auto dims = Vector2{static_cast<float>(m_spriteSheetTexture->GetDimensions().x), static_cast<float>(m_spriteSheetTexture->GetDimensions().y)};
    const auto epsilon = Vector2{1.0f / dims.x, 1.0f / dims.y};

    auto mins = Vector2{texCoords.x * spriteX, texCoords.y * spriteY};
    auto maxs = Vector2{texCoords.x * (spriteX + 1), texCoords.y * (spriteY + 1)};

    mins += epsilon;
    maxs -= epsilon;

    return AABB2(mins, maxs);
}

AABB2 SpriteSheet::GetTexCoordsFromSpriteCoords(const IntVector2& spriteCoords) const noexcept {
    return GetTexCoordsFromSpriteCoords(spriteCoords.x, spriteCoords.y);
}

AABB2 SpriteSheet::GetTexCoordsFromSpriteIndex(int spriteIndex) const noexcept {
    const auto x = spriteIndex % m_spriteLayout.x;
    const auto y = spriteIndex / m_spriteLayout.x;
    return GetTexCoordsFromSpriteCoords(x, y);
}

int SpriteSheet::GetNumSprites() const noexcept {
    return m_spriteLayout.x * m_spriteLayout.y;
}

int SpriteSheet::GetFrameWidth() const noexcept {
    return ((*m_spriteSheetTexture).GetDimensions().x / m_spriteLayout.x);
}

int SpriteSheet::GetFrameHeight() const noexcept {
    return ((*m_spriteSheetTexture).GetDimensions().y / m_spriteLayout.y);
}

IntVector2 SpriteSheet::GetFrameDimensions() const noexcept {
    return IntVector2(GetFrameWidth(), GetFrameHeight());
}

const IntVector2& SpriteSheet::GetLayout() const noexcept {
    return m_spriteLayout;
}

const Texture* SpriteSheet::GetTexture() const noexcept {
    return m_spriteSheetTexture;
}

Texture* SpriteSheet::GetTexture() noexcept {
    return m_spriteSheetTexture;
}

void SpriteSheet::LoadFromXml(const XMLElement& elem) noexcept {
    namespace FS = std::filesystem;
    DataUtils::ValidateXmlElement(elem, "spritesheet", "", "src,dimensions");
    m_spriteLayout = DataUtils::ParseXmlAttribute(elem, "dimensions", m_spriteLayout);
    std::string texturePathAsString{};
    texturePathAsString = DataUtils::ParseXmlAttribute(elem, "src", texturePathAsString);
    FS::path p{texturePathAsString};
    {
        std::error_code ec{};
        p = FS::canonical(p, ec);
        const auto error_msg = std::format("Error loading spritesheet at {}:\n{}", p, ec.message());
        GUARANTEE_OR_DIE(!ec, error_msg.c_str());
    }
    p.make_preferred();
    if(auto* r = ServiceLocator::get<IRendererService>(); r != nullptr) {
        m_spriteSheetTexture = r->CreateOrGetTexture(p.string(), IntVector3::XY_Axis);
    }
}
