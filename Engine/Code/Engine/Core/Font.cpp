#include "Engine/Core/Font.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Camera2D.hpp"

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
#include <vector>

std::vector<stbrp_rect> CalculateGlyphPacking(std::vector<Font::GlyphData>& glyphs, unsigned int target_texture_size, unsigned int& actual_texture_size) noexcept;
void GenerateFontAtlas(FT_Face face, const std::vector<stbrp_rect>& rects, unsigned int target_width, unsigned int target_height) noexcept;

Font::Font(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept {
    GUARANTEE_OR_DIE(LoadFont(path, pixelDimensions), "Could not load Font.");
}

bool Font::LoadFont(std::filesystem::path path, const IntVector2& pixelDimensions) noexcept {
    FT_Library library{nullptr};
    if(const auto ft_init_error = FT_Init_FreeType(&library); ft_init_error != FT_Err_Ok) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush("Failed to initialize FreeType Library.");
        return false;
    }

    FT_Face face{nullptr};
    if(const auto ft_face_error = FT_New_Face(library, path.string().c_str(), 0, &face); ft_face_error != FT_Err_Ok) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush("Failed to create new freetype face object.");
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
    m_data.face = face;
    m_data.pixelDimensions = pixelDimensions;
    if(m_hasKerning = FT_HAS_KERNING(face); !m_hasKerning) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(std::format("No kerning pairs found in font \"{}{}\".", path.stem(), pixelDimensions.y));
    }
    auto glyphs = LoadGlyphData(face);

    std::sort(std::begin(glyphs), std::end(glyphs), [](const GlyphData& a, const GlyphData& b) { 
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
    const auto rects = CalculateGlyphPacking(glyphs, target_size, actual_texture_size);
    GenerateFontAtlas(face, rects, actual_texture_size, actual_texture_size);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    m_loaded = true;
    return m_loaded;
}

bool Font::LoadFont(std::span<unsigned char> buffer, const IntVector2& pixelDimensions) noexcept {
    FT_Library library{nullptr};
    if(const auto ft_init_error = FT_Init_FreeType(&library); ft_init_error != FT_Err_Ok) {
        return false;
    }
    FT_Face face{nullptr};
    if(const auto ft_face_error = FT_New_Memory_Face(library, buffer.data(), static_cast<long>(buffer.size()), 0, &face); ft_face_error != FT_Err_Ok) {
        FT_Done_FreeType(library);
        library = nullptr;
        return false;
    }
    if(const auto ft_size_error = FT_Set_Pixel_Sizes(face, pixelDimensions.x, pixelDimensions.y); ft_size_error != FT_Err_Ok) {
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(std::format("Failed to set font size to {}.", pixelDimensions));
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }
    m_hasKerning = FT_HAS_KERNING(face);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    m_loaded = true;
    return m_loaded;
}

std::vector<Font::GlyphData> Font::LoadGlyphData(FT_Face face) noexcept {
    FT_GlyphSlot slot{face->glyph};
    std::vector<GlyphData> glyphs;
    glyphs.reserve(face->num_glyphs);
    unsigned int glyph_index{0ul};
    //Load glyph data
    for(auto charcode = FT_Get_First_Char(face, &glyph_index); glyph_index != 0; charcode = FT_Get_Next_Char(face, charcode, &glyph_index)) {
        if(const auto ft_glyph_error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT); ft_glyph_error != FT_Err_Ok) {
            continue;
        }
        const auto metrics = slot->metrics;
        const auto advance = slot->advance.x >> 6;
        const auto advancev = slot->advance.y >> 6;
        const auto width = [&]() {
            const auto widthAsPixels = metrics.width >> 6;
            const auto isSpace = charcode == ' ';
            const auto isNarrow = widthAsPixels == 0;
            return isNarrow || isSpace ? advance : widthAsPixels;
        }();
        const auto height = [&]() {
            const auto heightAsPixels = metrics.height >> 6;
            const auto isNarrow = heightAsPixels == 0;
            return isNarrow ? advancev : heightAsPixels;
            }();
        glyphs.push_back(Font::GlyphData{glyph_index, width, height, AABB2{}, charcode, advance});
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

void GenerateFontAtlas(FT_Face face, const std::vector<stbrp_rect>& rects, unsigned int target_width, unsigned int target_height) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    std::vector<Rgba> data = std::vector<Rgba>(target_width * target_height, Rgba::Black);
    if(const auto texture_atlas = renderer->Create2DTextureFromMemory(data, target_width, target_height, BufferUsage::Default, BufferBindUsage::Shader_Resource | BufferBindUsage::Render_Target); texture_atlas != nullptr) {
        if(const auto texture_ds = renderer->CreateDepthStencil(*renderer->GetDevice(), target_width, target_height); texture_ds != nullptr) {
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
            const auto path = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Images" / std::format("{}", "dolphin_atlas.png");
            auto img = Image(texture_atlas.get());
            if(const auto exported = img.Export(path); !exported) {
                auto* logger = ServiceLocator::get<IFileLoggerService>();
                logger->LogLine(std::format("Font Atlas creation failed for atlas {}\n", "dolphin"));
            }
        }
    }
}
