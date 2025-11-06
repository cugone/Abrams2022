#include "Engine/Core/Font.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <Thirdparty/freetype/ft2build.h>
#include <Thirdparty/freetype/freetype/ftbitmap.h>
#include <Thirdparty/stb/stb_rect_pack.h>

#include <algorithm>
#include <bit>
#include <format>
#include <memory>

std::vector<stbrp_rect> CalculateGlyphPacking(std::vector<Font::GlyphData>& glyphs, unsigned int target_texture_size, unsigned int& actual_texture_size) noexcept;
void GenerateFontAtlas(FT_Face face, int font_height, const std::vector<stbrp_rect>& rects, unsigned int target_width, unsigned int target_height) noexcept;

Font::Font(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept {
    GUARANTEE_OR_DIE(LoadFont(path, pixelDimensions), "Could not load Font.");
}

bool Font::LoadFont(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept {
    if(auto file_contents = FileUtils::ReadBinaryBufferFromFile(path); !file_contents.has_value()) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(std::format("Failed to read file contents from {}", path));
    } else {
        m_filepath = path;
        return LoadFont(*file_contents, pixelDimensions);
    }
    return m_loaded;
}

bool Font::LoadFont(const std::vector<uint8_t>& buffer, const IntVector2& pixelDimensions) noexcept {
    FT_Library library{nullptr};
    if(const auto ft_init_error = FT_Init_FreeType(&library); ft_init_error != FT_Err_Ok) {
        return false;
    }
    FT_Face face{nullptr};
    if(const auto ft_face_error = FT_New_Memory_Face(library, buffer.data(), static_cast<long>(buffer.size()), 0, &face); ft_face_error != FT_Err_Ok) {
        FT_Done_FreeType(library);
        return false;
    }
    if(const auto ft_size_error = FT_Set_Pixel_Sizes(face, pixelDimensions.x, pixelDimensions.y); ft_size_error != FT_Err_Ok) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(std::format("Failed to set font size to {}.", pixelDimensions));
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }
    m_data.pixelDimensions = pixelDimensions;
    m_data.hasKerning = FT_HAS_KERNING(face);
    m_data.ascender = face->ascender >> 6;
    m_data.descender = face->descender >> 6;
    m_data.em_units = face->units_per_EM >> 6;
    m_data.base = face->height >> 6;
    const auto family_name = std::string(face->family_name ? face->family_name : "");
    m_name = std::format("{}{}", family_name, pixelDimensions.y);
    if(!m_data.hasKerning) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(std::format("No kerning pairs found in font \"{}\".", m_name));
    }
    LoadCommonData(face);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    m_loaded = true;
    return m_loaded;
}

bool Font::IsLoaded() const noexcept {
    return m_loaded;
}

const std::string& Font::GetName() const noexcept {
    return m_name;
}

const std::filesystem::path& Font::GetFilePath() const noexcept {
    return m_filepath;
}

float Font::CalculateTextWidth(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    auto cursor_x = 0.0f;

    for(auto char_iter = text.begin(); char_iter != text.end(); /* DO NOTHING */) {
        const auto current_char_def = GetCharDef(*char_iter);
        const auto previous_char = char_iter++;
        if(char_iter != text.end()) {
            auto kern_value = 0.0f;
            if(m_data.hasKerning) {
                if(const auto kern_iter = m_kerning.find(std::make_pair(*previous_char, *char_iter)); kern_iter != std::cend(m_kerning)) {
                    kern_value = static_cast<float>(kern_iter->second);
                }
            }
            cursor_x += current_char_def.advance + kern_value;
        } else {
            const auto previous_char_def = GetCharDef(*previous_char);
            cursor_x += previous_char_def.advance;
        }
    }

    return cursor_x * scale;
}

Font::GlyphData Font::GetCharDef(int c) const noexcept {
    const auto iter = std::find_if(std::cbegin(m_glyphs), std::cend(m_glyphs), [&c](const GlyphData& glyph) { return glyph.charCode == static_cast<unsigned long>(c); });
    if(iter == std::cend(m_glyphs)) {
        return {};
    }
    return *iter;
}

float Font::CalculateTextHeight(float scale /*= 1.0f*/) const noexcept {
    return GetLineHeight() * scale;
}

Vector2 Font::CalculateTextDimensions(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    return Vector2{CalculateTextWidth(text, scale), CalculateTextHeight(scale)};
}

AABB2 Font::CalculateTextArea(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    AABB2 result{};
    result.StretchToIncludePoint(CalculateTextDimensions(text, scale));
    return result;
}

float Font::GetLineHeight() const noexcept {
    return static_cast<float>(m_data.ascender - m_data.descender);
}

float Font::GetLineHeightAsUV() const noexcept {
    return GetLineHeight() / m_data.texture_size;
}

bool Font::LoadFromFile(std::filesystem::path filepath) noexcept {
    return LoadFont(filepath, IntVector2{32,32});
}

bool Font::LoadFromBuffer(const std::vector<uint8_t>& buffer) noexcept {
    return LoadFont(buffer, IntVector2{32,32});
}

Material* Font::GetMaterial() const noexcept {
    return m_material;
}

void Font::SetMaterial(Material* mat) noexcept {
    m_material = mat;
}

int Font::GetKerningValue(unsigned long first, unsigned long second) const noexcept {
    if(m_kerning.empty()) {
        return 0;
    }
    if(const auto iter = m_kerning.find(std::make_pair(first, second)); iter != std::cend(m_kerning)) {
        return static_cast<int>(iter->second);
    } else {
        return 0;
    }

}

AABB2 Font::GetGlyphUVs(int c) const noexcept {
    const auto& glyph = GetCharDef(c);
    return glyph.uvs;
}

Vector2 Font::GetGlyphOffsets(int c) const noexcept {
    const auto& glyph = GetCharDef(c);
    return Vector2(glyph.offsets);
}

Vector2 Font::GetGlyphDimensions(int c) const noexcept {
    const auto& glyph = GetCharDef(c);
    return Vector2(static_cast<float>(glyph.width), static_cast<float>(glyph.height));
}

int Font::GetGlyphAdvance(int c) const noexcept {
    const auto& glyph = GetCharDef(c);
    return glyph.advance;
}

int Font::GetEmSize() const noexcept {
    return m_data.em_units;
}

void Font::LoadCommonData(FT_Face face) noexcept {
    m_glyphs = LoadGlyphData(face);
    
    std::sort(std::begin(m_glyphs), std::end(m_glyphs), [](const GlyphData& a, const GlyphData& b) {
        if(b.height < a.height) {
            return true;
        } else if(b.height == a.height) {
            if(b.width < a.width) {
                return true;
            } else if(b.width == a.width) {
                if(b.glyph_index < a.glyph_index) {
                    return true;
                }
            }
        }
        return false;
    });

    const auto target_size = 128u;
    auto actual_texture_size = target_size;
    const auto rects = CalculateGlyphPacking(m_glyphs, target_size, actual_texture_size);
    m_data.texture_size = actual_texture_size;
    GenerateFontAtlas(face, m_data.pixelDimensions.y, rects, actual_texture_size, actual_texture_size);
    CreateMaterial();
}

std::vector<Font::GlyphData> Font::LoadGlyphData(FT_Face face) noexcept {
    FT_GlyphSlot slot{face->glyph};
    std::vector<GlyphData> glyphs;
    glyphs.reserve(face->num_glyphs);
    unsigned int glyph_index{0ul};

    for(auto charcode = FT_Get_First_Char(face, &glyph_index); glyph_index != 0; charcode = FT_Get_Next_Char(face, charcode, &glyph_index)) {
        if(const auto ft_glyph_error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT); ft_glyph_error != FT_Err_Ok) {
            continue;
        }
        const auto metrics = slot->metrics;
        const auto advance = slot->advance.x >> 6;
        const auto offsets = IntVector2{metrics.horiBearingX >> 6, metrics.horiBearingY >> 6};
        const auto width = [&]() {
            const auto widthAsPixels = metrics.width >> 6;
            const auto isSpace = charcode == ' ';
            const auto isNarrow = widthAsPixels == 0;
            return isNarrow || isSpace ? advance : widthAsPixels;
        }();
        const auto height = [&]() {
            const auto heightAsPixels = metrics.height >> 6;
            const auto isNarrow = heightAsPixels == 0;
            const auto isSpace = charcode == ' ';
            return isNarrow || isSpace ? static_cast<long>(GetLineHeight()) : heightAsPixels;
        }();
        glyphs.push_back(Font::GlyphData{glyph_index, width, height, AABB2{}, offsets, charcode, advance});
    }
    return glyphs;
}

std::vector<stbrp_rect> CalculateGlyphPacking(std::vector<Font::GlyphData>& glyphs, unsigned int target_texture_size, unsigned int& actual_texture_size) noexcept {
    auto target_multiplier = 1.5f;
    auto target = target_texture_size;
    auto target_width = target;
    auto target_height = target;
    std::vector<stbrp_rect> rects;
    rects.reserve(glyphs.size());
    for(const auto& g : glyphs) {
        rects.push_back(stbrp_rect(g.glyph_index, g.width, g.height, 0, 0, false));
    }

    stbrp_context context;
    target_width = target;
    target_height = target;
    const auto glyphs_size = static_cast<unsigned int>(glyphs.size());
    int num_nodes = std::bit_ceil(static_cast<unsigned int>(std::ceil(std::max({glyphs_size, target_width, target_height}))));
    auto nodes = std::make_unique<stbrp_node[]>(num_nodes);
    stbrp_init_target(&context, target_width, target_height, nodes.get(), num_nodes);
    stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BF_sortHeight);
    for(bool all_packed = !!stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size())); all_packed == false; /* DO NOTHING */) {
        nodes.reset(nullptr);
        target = std::bit_ceil(static_cast<unsigned int>(std::ceil(target * target_multiplier)));
        target_width = target;
        target_height = target;
        num_nodes = std::max(target_width, target_height);
        nodes = std::make_unique<stbrp_node[]>(num_nodes);
        stbrp_init_target(&context, target_width, target_height, nodes.get(), num_nodes);
        all_packed = !!stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()));
    }
    actual_texture_size = target;
    for(std::size_t i = 0u; i < glyphs.size(); ++i) {
        auto& glyph = glyphs[i];
        const auto& r = rects[i];
        glyph.uvs = AABB2(r.x / static_cast<float>(target_width), r.y / static_cast<float>(target_height), (r.x + r.w) / static_cast<float>(target_width), (r.y + r.h) / static_cast<float>(target_height));
    }
    return rects;
}

void GenerateFontAtlas(FT_Face face, int font_height, const std::vector<stbrp_rect>& rects, unsigned int target_width, unsigned int target_height) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    std::vector<Rgba> data = std::vector<Rgba>(target_width * target_height, Rgba::NoAlpha);
    if(auto texture_atlas = renderer->Create2DTextureFromMemory(data, target_width, target_height, BufferUsage::Default, BufferBindUsage::Shader_Resource | BufferBindUsage::Render_Target); texture_atlas != nullptr) {
        if(auto texture_ds = renderer->CreateDepthStencil(*renderer->GetDevice(), target_width, target_height); texture_ds != nullptr) {
            renderer->SetRenderTarget(texture_atlas.get(), texture_ds.get());
            for(auto i = std::size_t{0u}; i < rects.size(); ++i) {
                auto& r = rects[i];
                if(const auto ft_glyph_error = FT_Load_Glyph(face, r.id, FT_LOAD_DEFAULT); ft_glyph_error == FT_Err_Ok) {
                    if(const auto ft_render_error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL); ft_render_error != FT_Err_Ok) {
                        continue;
                    }
                    if(face->glyph->bitmap.buffer == nullptr) {
                        continue;
                    }
                    if(r.w == 0 || r.h == 0) {
                        continue;
                    }
                    const auto buffer = face->glyph->bitmap.buffer;
                    const auto height = r.h;
                    const auto width = r.w;
                    auto rgbaBuffer = std::vector<Rgba>(width * height);
                    for(std::size_t y = 0; y < height; ++y) {
                        for(std::size_t x = 0; x < width; ++x) {
                            const auto index = y * width + x;
                            const auto greyscalepixel = buffer[index];
                            rgbaBuffer[index] = Rgba(greyscalepixel, greyscalepixel, greyscalepixel, 255u);
                        }
                    }
                    if(const auto glyph_texture = renderer->Create2DTextureFromMemory(rgbaBuffer, width, height, BufferUsage::Dynamic); glyph_texture != nullptr) {
                        D3D11_BOX srcBox{0u, 0u, 0u, static_cast<unsigned int>(r.w), static_cast<unsigned int>(r.h), 1u};
                        renderer->GetDeviceContext()->GetDxContext()->CopySubresourceRegion(texture_atlas->GetDxResource(), 0, r.x, r.y, 0, glyph_texture->GetDxResource(), 0, &srcBox);
                    }
                }
            }
            const auto style_name_sv = std::string_view(face->style_name ? face->style_name : "");
            const auto family_name_sv = std::string_view(face->family_name ? face->family_name : "");
            const auto size = font_height;
            const auto font_name = std::format("{}{}", family_name_sv, size);
            const auto register_err = std::format("Failed to register texture for font \"{}\"", font_name);
            const auto material_name = std::format("__Font_{}", font_name);
            texture_atlas->SetDebugName(material_name);
            GUARANTEE_OR_DIE(renderer->RegisterTexture(material_name, std::move(texture_atlas)), register_err);
            renderer->SetRenderTargetsToBackBuffer();
        }
    }
}

void Font::CreateMaterial() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    const std::string name = GetName();
    const std::string shader = "__font";
    const std::string material_name = std::format("__Font_{}", name);
    const std::string& tex_name = material_name;
    std::ostringstream material_stream;
    std::string material_string = std::format(
R"(
<material name="{}">
    <shader src="{}" />
    <textures>
        <diffuse src="{}" />
    </textures>
</material>
)",material_name, shader, tex_name);
    tinyxml2::XMLDocument doc;
    const auto result = doc.Parse(material_string.c_str(), material_string.size());
    GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Failed to create default system32 font: Invalid XML file.\n");
    const auto* xml_root = doc.RootElement();
    {
        auto mat = std::make_unique<Material>(*xml_root);
        renderer->RegisterMaterial(std::move(mat));
    }
    auto mat = renderer->GetMaterial(material_name);
    SetMaterial(mat);
}
