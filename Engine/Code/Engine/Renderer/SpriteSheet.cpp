#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "Engine/Services/ServiceLocator.hpp"

#include <sstream>

SpriteSheet::SpriteSheet(const XMLElement& elem) noexcept {
    LoadFromXml(elem);
}

SpriteSheet::SpriteSheet(Texture* texture, int tilesWide, int tilesHigh) noexcept
: _spriteSheetTexture(texture)
, _spriteLayout(tilesWide, tilesHigh) {
    /* DO NOTHING */
}

SpriteSheet::SpriteSheet(const std::filesystem::path& texturePath, int tilesWide, int tilesHigh) noexcept
: _spriteSheetTexture(ServiceLocator::get<IRendererService>().CreateOrGetTexture(texturePath, IntVector3::XY_Axis))
, _spriteLayout(tilesWide, tilesHigh) {
    /* DO NOTHING */
}

AABB2 SpriteSheet::GetTexCoordsFromSpriteCoords(int spriteX, int spriteY) const noexcept {
    const auto texCoords = Vector2{1.0f / _spriteLayout.x, 1.0f / _spriteLayout.y};

    const auto dims = Vector2{static_cast<float>(_spriteSheetTexture->GetDimensions().x), static_cast<float>(_spriteSheetTexture->GetDimensions().y)};
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
    const auto x = spriteIndex % _spriteLayout.x;
    const auto y = spriteIndex / _spriteLayout.x;
    return GetTexCoordsFromSpriteCoords(x, y);
}

int SpriteSheet::GetNumSprites() const noexcept {
    return _spriteLayout.x * _spriteLayout.y;
}

int SpriteSheet::GetFrameWidth() const noexcept {
    return ((*_spriteSheetTexture).GetDimensions().x / _spriteLayout.x);
}

int SpriteSheet::GetFrameHeight() const noexcept {
    return ((*_spriteSheetTexture).GetDimensions().y / _spriteLayout.y);
}

IntVector2 SpriteSheet::GetFrameDimensions() const noexcept {
    return IntVector2(GetFrameWidth(), GetFrameHeight());
}

const IntVector2& SpriteSheet::GetLayout() const noexcept {
    return _spriteLayout;
}

const Texture* SpriteSheet::GetTexture() const noexcept {
    return _spriteSheetTexture;
}

Texture* SpriteSheet::GetTexture() noexcept {
    return _spriteSheetTexture;
}

void SpriteSheet::LoadFromXml(const XMLElement& elem) noexcept {
    namespace FS = std::filesystem;
    DataUtils::ValidateXmlElement(elem, "spritesheet", "", "src,dimensions");
    _spriteLayout = DataUtils::ParseXmlAttribute(elem, "dimensions", _spriteLayout);
    std::string texturePathAsString{};
    texturePathAsString = DataUtils::ParseXmlAttribute(elem, "src", texturePathAsString);
    FS::path p{texturePathAsString};
    {
        std::error_code ec{};
        p = FS::canonical(p, ec);
        {
            const auto error_msg = std::string{"Error loading spritesheet at "} + texturePathAsString + ":\n" + ec.message();
            GUARANTEE_OR_DIE(!ec, error_msg.c_str());
        }
    }
    p.make_preferred();
    _spriteSheetTexture = ServiceLocator::get<IRendererService>().CreateOrGetTexture(p.string(), IntVector3::XY_Axis);
}
