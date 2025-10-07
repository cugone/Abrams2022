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

    using KerningMap = std::map<std::pair<unsigned long, unsigned long>, signed long>;

    struct FontData {
        FT_Face face{nullptr};         //Font face
        IntVector2 pixelDimensions{};  //Pixel size of font.
        bool hasKerning{false};        //Font face has kerning
    };
    struct GlyphData {
        unsigned int glyph_index{0u};  //Glyph Id.
        long width{0ul};               //Glyph texture width.
        long height{0ul};              //Glyph texture height.
        AABB2 uvs{};                   //UVs for glyph in texture atlas.
        IntVector2 offsets{};          //Absolute top-left of glyph in atlas.
        unsigned long charCode{};      //Char code of glyph.
        signed long advance{};         //Horizontal advance to next character.
    };
    struct KerningData {
        unsigned long first{};         //First character code in kerning pair.
        unsigned long second{};        //Second character code in kerning pair.
        signed long amount{};          //Kerning amount. Usually negative.
    };

protected:
private:

    std::vector<Font::GlyphData> LoadGlyphData(FT_Face face) noexcept;
    void LoadCommonData() noexcept;

    FontData m_data{};
    std::vector<GlyphData> m_glyphs{};
    KerningMap m_kerning{};
    bool m_loaded{false};
};
