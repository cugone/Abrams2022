#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include <filesystem>
#include <string>

class Renderer;
class Texture;

class SpriteSheet {
public:
    explicit SpriteSheet(const XMLElement& elem) noexcept;
    ~SpriteSheet() = default;

    [[nodiscard]] AABB2 GetTexCoordsFromSpriteCoords(int spriteX, int spriteY) const noexcept;
    [[nodiscard]] AABB2 GetTexCoordsFromSpriteCoords(const IntVector2& spriteCoords) const noexcept;
    [[nodiscard]] AABB2 GetTexCoordsFromSpriteIndex(int spriteIndex) const noexcept;
    [[nodiscard]] int GetNumSprites() const noexcept;
    [[nodiscard]] int GetFrameWidth() const noexcept;
    [[nodiscard]] int GetFrameHeight() const noexcept;
    [[nodiscard]] IntVector2 GetFrameDimensions() const noexcept;
    [[nodiscard]] const IntVector2& GetLayout() const noexcept;
    [[nodiscard]] const Texture* GetTexture() const noexcept;
    [[nodiscard]] Texture* GetTexture() noexcept;

protected:
private:
    SpriteSheet(Texture* texture, int tilesWide, int tilesHigh) noexcept;
    SpriteSheet(const std::filesystem::path& texturePath, int tilesWide, int tilesHigh) noexcept;

    void LoadFromXml(const XMLElement& elem) noexcept;
    Texture* _spriteSheetTexture = nullptr;
    IntVector2 _spriteLayout{1, 1};

    friend class Renderer;
    friend class MapEditor;
};
