#pragma once

#include "Engine/Core/IFont.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/IntVector4.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Material;
class Renderer;

class KerningFont : public a2de::IFont {
public:
    struct KerningFontInfoDef {
        std::string face{};     //name of true type font
        std::string charset{};  //OEM charset if not unicode (otherwise empty)
        float stretch_height{}; //font height stretch percentage
        int em_size{};          //size of font in em
        int is_aliased{};       //supersamping level 1 for none
        int outline{};          //outline thickness
        IntVector4 padding{};   //top-right-bottom-left padding
        IntVector2 spacing{};   //horizontal-vertical spacing
        bool is_bold{};         //is bold
        bool is_italic{};       //is italic
        bool is_unicode{};      //is unicode
        bool is_smoothed{};     //is smoothed
        bool is_fixedHeight{};  //is fixed height
    };

    struct KerningFontPageDef {
        int id{};
        std::string file{};
    };

    struct KerningFontCommonDef {
        int line_height{};  //Distance in pixels between each line
        int base{};         //Number of pixels from absolute top of line to base of the characters.
        IntVector2 scale{}; //The width/height of the texture, normally used to scale the x/y position of the character image.
        int page_count{};   //The number of texture pages included in the font.
        bool is_packed{};   //true if monochrome characters have been packed into each texture channel. See individual channels for descriptions.
        //0 -- channel holds glyph data.
        //1 -- channel holds outline.
        //2 -- channel holds glyph and outline.
        //3 -- channel set to zero.
        //4 -- channel set to one.
        int alpha_channel{};
        int red_channel{};
        int green_channel{};
        int blue_channel{};
    };

    struct KerningFontCharDef {
        int id{};
        IntVector2 position{};
        IntVector2 dimensions{};
        IntVector2 offsets{};
        int xadvance{};
        int page_id{};
        int channel_id{};
    };

    struct KerningFontKerningDef {
        int first{};
        int second{};
        int amount{};
    };

    using InfoDef = KerningFontInfoDef;
    using CommonDef = KerningFontCommonDef;
    using PageDef = KerningFontPageDef;
    using CharDef = KerningFontCharDef;
    using KerningDef = KerningFontKerningDef;
    using CharMap = std::map<int, CharDef>;
    using KerningMap = std::map<std::pair<int, int>, int>;

    KerningFont() = default;
    KerningFont(const KerningFont& font) = default;
    KerningFont(KerningFont&& font) = default;
    KerningFont& operator=(KerningFont&& font) = default;
    KerningFont& operator=(const KerningFont& font) = default;
    ~KerningFont() = default;

    [[nodiscard]] static float CalculateTextWidth(const KerningFont& font, const std::string& text, float scale = 1.0f) noexcept;
    [[nodiscard]] static float CalculateTextHeight(const KerningFont& font, float scale = 1.0f) noexcept;

    [[nodiscard]] static Vector2 CalculateTextDimensions(const KerningFont& font, const std::string& text, float scale = 1.0f) noexcept;
    [[nodiscard]] static AABB2 CalculateTextArea(const KerningFont& font, const std::string& text, float scale = 1.0f) noexcept;

    [[nodiscard]] float CalculateTextWidth(const std::string& text, float scale = 1.0f) const noexcept override;
    [[nodiscard]] float CalculateTextHeight(float scale = 1.0f) const noexcept override;

    [[nodiscard]] Vector2 CalculateTextDimensions(const std::string& text, float scale = 1.0f) const noexcept override;
    [[nodiscard]] AABB2 CalculateTextArea(const std::string& text, float scale = 1.0f) const noexcept override;

    [[nodiscard]] float GetLineHeight() const noexcept override;
    [[nodiscard]] float GetLineHeightAsUV() const noexcept override;

    [[nodiscard]] const std::string& GetName() const noexcept override;
    [[nodiscard]] KerningFont::CharDef GetCharDef(int ch) const noexcept;
    [[nodiscard]] const KerningFont::CommonDef& GetCommonDef() const noexcept;
    [[nodiscard]] const KerningFont::InfoDef& GetInfoDef() const noexcept;

    [[nodiscard]] const std::vector<std::string>& GetImagePaths() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetFilePath() const noexcept override;
    [[nodiscard]] bool LoadFromFile(std::filesystem::path filepath) noexcept override;
    [[nodiscard]] bool LoadFromBuffer(const std::vector<uint8_t>& buffer) noexcept override;

    [[nodiscard]] Material* GetMaterial() const noexcept override;
    void SetMaterial(Material* mat) noexcept override;

    [[nodiscard]] int GetKerningValue(unsigned long first, unsigned long second) const noexcept override;
    [[nodiscard]] int GetKerningValue(int first, int second) const noexcept;

    [[nodiscard]] bool IsLoaded() const noexcept override;
    [[nodiscard]] AABB2 GetGlyphUVs(int c) const noexcept override;
    [[nodiscard]] Vector2 GetGlyphOffsets(int c) const noexcept override;
    [[nodiscard]] Vector2 GetGlyphDimensions(int c) const noexcept override;
    [[nodiscard]] int GetGlyphAdvance(int c) const noexcept override;

    [[nodiscard]] int GetEmSize() const noexcept override;

protected:
private:

    void CreateMaterial() noexcept;
    void CreateTextures() noexcept;

    [[nodiscard]] bool LoadFromText(std::vector<unsigned char>& buffer) noexcept;
    [[nodiscard]] bool LoadFromXml(std::vector<unsigned char>& buffer) noexcept;
    [[nodiscard]] bool LoadFromBinary(std::vector<unsigned char>& buffer) noexcept;

    [[nodiscard]] bool IsInfoLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsCommonLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsPageLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsCharsLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsCharLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsKerningsLine(const std::string& cur_line) noexcept;
    [[nodiscard]] bool IsKerningLine(const std::string& cur_line) noexcept;

    [[nodiscard]] bool ParseInfoLine(const std::string& infoLine) noexcept;
    [[nodiscard]] bool ParseCommonLine(const std::string& commonLine) noexcept;
    [[nodiscard]] bool ParsePageLine(const std::string& pageLine) noexcept;
    [[nodiscard]] bool ParseCharsLine(const std::string& charsLine) noexcept;
    [[nodiscard]] bool ParseCharLine(const std::string& charLine) noexcept;
    [[nodiscard]] bool ParseKerningsLine(const std::string& kerningsLine) noexcept;
    [[nodiscard]] bool ParseKerningLine(const std::string& kerningLine) noexcept;

    Material* m_material = nullptr;
    std::string m_name{};
    std::vector<std::string> m_image_paths{};
    std::filesystem::path m_filepath{};
    CharMap m_charmap{};
    KerningMap m_kernmap{};
    InfoDef m_info{};
    CommonDef m_common{};
    std::size_t m_char_count{0u};
    std::size_t m_kerns_count{0u};
    bool m_is_loaded = false;
};
