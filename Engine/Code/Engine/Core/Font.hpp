#pragma once

#include "Engine/Math/AABB2.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <Thirdparty/freetype/ft2build.h>
#include <Thirdparty/freetype/freetype/freetype.h>

#include <filesystem>
#include <memory>
#include <span>

class Font {
public:
    Font() = default;
    explicit Font(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept;
    ~Font() noexcept = default;

    [[nodiscard]] bool LoadFont(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept;
    [[nodiscard]] bool LoadFont(std::span<unsigned char> buffer, const IntVector2& pixelDimensions) noexcept;

    struct FontData {
        FT_Face face{nullptr};
        IntVector2 pixelDimensions{};
        bool hasKerning{false};
    };
    struct GlyphData {
        unsigned int glyph_index{0u};
        long width{0ul};
        long height{0ul};
        AABB2 uvs{};
        unsigned long charCode{};
        signed long advance{};
    };
    struct KerningData {
        std::pair<unsigned long, unsigned long> glyphs{};
        signed long advance{};
    };

protected:
private:

    std::vector<Font::GlyphData> LoadGlyphData(FT_Face face) noexcept;
    void LoadCommonData() noexcept;

    FontData m_data{};
    std::vector<GlyphData> m_glyphs{};
    std::vector<KerningData> m_kerning{};
    bool m_loaded{false};
};
