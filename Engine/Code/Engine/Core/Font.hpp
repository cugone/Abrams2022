#pragma once

#include "Engine/Math/AABB2.hpp"

#include "Engine/Core/IFont.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <Thirdparty/freetype/ft2build.h>
#include <Thirdparty/freetype/freetype/freetype.h>

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

class Material;

class Font : public a2de::IFont {
public:
    Font() = default;
    explicit Font(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept;
    ~Font() noexcept = default;

    [[nodiscard]] bool LoadFont(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept;
    [[nodiscard]] bool LoadFont(const std::vector<uint8_t>& buffer, const IntVector2& pixelDimensions) noexcept;

    [[nodiscard]] bool IsLoaded() const noexcept override;
    [[nodiscard]] const std::string& GetName() const noexcept override;
    [[nodiscard]] const std::filesystem::path& GetFilePath() const noexcept override;

    using KerningMap = std::map<std::pair<unsigned long, unsigned long>, signed long>;

    struct FontData {
        IntVector2 pixelDimensions{};  //Pixel size of font.
        int ascender{};                //Distance from baseline to top of glyph
        int descender{};               //Distance from baseline to bottom of glyph
        int em_units{};                //Units per EM
        int base{};                    //Distance from baseline to baseline. i.e. Height of a line.
        int texture_size{};            //Texture size. Always square.
        bool hasKerning{false};        //Font face has kerning
    };
    struct GlyphData {
        unsigned int glyph_index{0u};  //Glyph Id.
        long width{0ul};               //Glyph texture width.
        long height{0ul};              //Glyph texture height.
        AABB2 uvs{};                   //UVs for glyph in texture atlas.
        IntVector2 offsets{};          //Offsets of glyph from cursor X-position and top of line.
        unsigned long charCode{};      //Char code of glyph.
        signed long advance{};         //Horizontal advance to next character.
    };
    struct KerningData {
        unsigned long first{};         //First character code in kerning pair.
        unsigned long second{};        //Second character code in kerning pair.
        signed long amount{};          //Kerning amount. Usually negative.
    };


     [[nodiscard]] float CalculateTextWidth(const std::string& text, float scale = 1.0f) const noexcept override;
     [[nodiscard]] float CalculateTextHeight(float scale = 1.0f) const noexcept override;
     [[nodiscard]] Vector2 CalculateTextDimensions(const std::string& text, float scale = 1.0f) const noexcept override;
     [[nodiscard]] AABB2 CalculateTextArea(const std::string& text, float scale = 1.0f) const noexcept override;
     [[nodiscard]] float GetLineHeight() const noexcept override;
     [[nodiscard]] float GetLineHeightAsUV() const noexcept override;
     [[nodiscard]] bool LoadFromFile(std::filesystem::path filepath) noexcept override;
     [[nodiscard]] bool LoadFromBuffer(const std::vector<uint8_t>& buffer) noexcept override;
     [[nodiscard]] Material* GetMaterial() const noexcept override;
     [[nodiscard]] void SetMaterial(Material* mat) noexcept override;
     [[nodiscard]] int GetKerningValue(unsigned long first, unsigned long second) const noexcept override;

     [[nodiscard]] AABB2 GetGlyphUVs(int c) const noexcept override;
     [[nodiscard]] Vector2 GetGlyphOffsets(int c) const noexcept override;
     [[nodiscard]] Vector2 GetGlyphDimensions(int c) const noexcept override;
     [[nodiscard]] int GetGlyphAdvance(int c) const noexcept override;

     [[nodiscard]] int GetEmSize() const noexcept override;

 protected:
 private:


    GlyphData GetCharDef(int c) const noexcept;
    void LoadCommonData(FT_Face face) noexcept;
    [[nodiscard]] std::vector<Font::GlyphData> LoadGlyphData(FT_Face face) noexcept;
    void CreateMaterial() noexcept;

    FontData m_data{};
    std::vector<GlyphData> m_glyphs{};
    KerningMap m_kerning{};
    std::string m_name{};
    std::filesystem::path m_filepath{};
    Material* m_material{nullptr};
    bool m_loaded{false};
};
