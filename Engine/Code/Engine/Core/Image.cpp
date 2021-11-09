#include "Engine/Core/Image.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Texture1D.hpp"
#include "Engine/Renderer/Texture2D.hpp"
#include "Engine/Renderer/Texture3D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <Thirdparty/stb/stb_image.h>
#include <Thirdparty/stb/stb_image_write.h>

#include <algorithm>
#include <sstream>
#include <vector>

Image::Image(std::filesystem::path filepath) noexcept
: m_filepath(filepath) {
    namespace FS = std::filesystem;

    {
        const auto error_msg = std::string{"Failed to load image. Could not find file: "} + filepath.string() + ".\n";
        GUARANTEE_OR_DIE(FS::exists(filepath), error_msg.c_str());
    }

    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    if(const auto& buf = FileUtils::ReadBinaryBufferFromFile(filepath); buf.has_value()) {
        int comp = 0;
        int req_comp = 4;
        auto* texel_bytes = stbi_load_from_memory(buf->data(), static_cast<int>(buf->size()), &m_dimensions.x, &m_dimensions.y, &comp, req_comp);
        m_bytesPerTexel = req_comp;
        m_texelBytes = std::vector<unsigned char>(texel_bytes, texel_bytes + (static_cast<std::size_t>(m_dimensions.x) * m_dimensions.y * m_bytesPerTexel));
        stbi_image_free(texel_bytes);
    } else {
        const auto ss = std::string{"Failed to load image. "} + filepath.string() + " is not a supported image type.";
        GUARANTEE_RECOVERABLE(!m_texelBytes.empty(), ss.c_str());
    }
}
Image::Image(unsigned int width, unsigned int height) noexcept
: m_dimensions(width, height)
, m_bytesPerTexel(4)
, m_texelBytes(static_cast<std::size_t>(width) * height * m_bytesPerTexel, 0u) {
    /* DO NOTHING */
}

Image::Image(unsigned char* data, unsigned int width, unsigned int height) noexcept
: m_dimensions(width, height)
, m_bytesPerTexel(4)
, m_texelBytes(data, data + (static_cast<std::size_t>(width) * height * m_bytesPerTexel)) {
    /* DO NOTHING */
}

Image::Image(Rgba* data, unsigned int width, unsigned int height) noexcept
: m_dimensions(width, height)
, m_bytesPerTexel(4)
, m_texelBytes(reinterpret_cast<unsigned char*>(data), reinterpret_cast<unsigned char*>(data) + static_cast<std::size_t>(width) * height * m_bytesPerTexel) {
    /* DO NOTHING */
}

Image::Image(const std::vector<Rgba>& data, unsigned int width, unsigned int height) noexcept
: m_dimensions(width, height)
, m_bytesPerTexel(4)
, m_texelBytes(reinterpret_cast<const unsigned char*>(data.data()), reinterpret_cast<const unsigned char*>(data.data()) + static_cast<std::size_t>(width) * height * m_bytesPerTexel) {
    /* DO NOTHING */
}

Image::Image(const std::vector<unsigned char>& data, unsigned int width, unsigned int height) noexcept
: m_dimensions(width, height)
, m_bytesPerTexel(4)
, m_texelBytes(data.data(), data.data() + static_cast<std::size_t>(width) * height * m_bytesPerTexel) {
    /* DO NOTHING */
}

Image::Image(Image&& img) noexcept
: m_dimensions(std::move(img.m_dimensions))
, m_bytesPerTexel(std::move(img.m_bytesPerTexel))
, m_filepath(std::move(img.m_filepath))
{
    std::scoped_lock<std::mutex, std::mutex> lock(_cs, img._cs);
    m_texelBytes = std::move(img.m_texelBytes);
}

Image::Image(const Texture* tex, const Renderer* /*renderer*/) noexcept
    : Image(tex)
{
    /* DO NOTHING */
}

Image::Image(const Texture* tex) noexcept {
    auto* tex2d = tex->GetDxResourceAs<ID3D11Texture2D>();
    D3D11_TEXTURE2D_DESC desc{};
    tex2d->GetDesc(&desc);

    m_dimensions = IntVector2(desc.Width, desc.Height);
    m_bytesPerTexel = 4;
    const auto size = desc.Width * desc.Height * m_bytesPerTexel;
    m_texelBytes.resize(size);
    const auto& renderer = ServiceLocator::get<IRendererService>();
    auto stage = renderer.Create2DTextureFromMemory(m_texelBytes.data(), desc.Width, desc.Height, BufferUsage::Staging);
    renderer.CopyTexture(tex, stage.get());

    D3D11_MAPPED_SUBRESOURCE resource{};
    auto* dc = renderer.GetDeviceContext();
    auto* dc_dx = dc->GetDxContext();
    auto hr = dc_dx->Map(stage->GetDxResource(), 0u, D3D11_MAP_READ, 0u, &resource);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), StringUtils::FormatWindowsMessage(hr));

    auto* src = reinterpret_cast<unsigned int*>(resource.pData);
    auto* dst = reinterpret_cast<unsigned int*>(m_texelBytes.data());
    const auto stride = static_cast<std::size_t>(desc.Width) * m_bytesPerTexel;
    for(std::size_t i = 0u; i < desc.Height; ++i) {
        std::memcpy(dst, src, stride);
        dst += resource.RowPitch >> 2;
        src += desc.Width;
    }
    dc_dx->Unmap(stage->GetDxResource(), 0u);
}

Image& Image::operator=(Image&& rhs) noexcept {
    std::scoped_lock<std::mutex, std::mutex> lock(_cs, rhs._cs);
    m_bytesPerTexel = std::move(rhs.m_bytesPerTexel);
    m_dimensions = std::move(rhs.m_dimensions);
    m_filepath = std::move(rhs.m_filepath);
    m_texelBytes = std::move(rhs.m_texelBytes);

    rhs.m_bytesPerTexel = 0;
    rhs.m_dimensions = IntVector2::Zero;
    rhs.m_filepath = std::string{};
    rhs.m_texelBytes.clear();
    rhs.m_texelBytes.shrink_to_fit();
    return *this;
}

Rgba Image::GetTexel(const IntVector2& texelPos) const noexcept {
    std::size_t index = static_cast<std::size_t>(texelPos.x) + static_cast<std::size_t>(texelPos.y) * m_dimensions.x;
    std::size_t byteOffset = index * m_bytesPerTexel;
    Rgba color;
    color.r = m_texelBytes[byteOffset + 0];
    color.g = m_texelBytes[byteOffset + 1];
    color.b = m_texelBytes[byteOffset + 2];
    if(m_bytesPerTexel == 4) {
        color.a = m_texelBytes[byteOffset + 3];
    } else {
        color.a = 255;
    }
    return color;
}
void Image::SetTexel(const IntVector2& texelPos, const Rgba& color) noexcept {
    std::size_t index = static_cast<std::size_t>(texelPos.x) + static_cast<std::size_t>(texelPos.y) * m_dimensions.x;
    std::size_t byteOffset = index * m_bytesPerTexel;
    m_texelBytes[byteOffset + 0] = color.r;
    m_texelBytes[byteOffset + 1] = color.g;
    m_texelBytes[byteOffset + 2] = color.b;
    if(m_bytesPerTexel == 4) {
        m_texelBytes[byteOffset + 3] = color.a;
    }
}

const std::filesystem::path& Image::GetFilepath() const noexcept {
    return m_filepath;
}

const IntVector2& Image::GetDimensions() const noexcept {
    return m_dimensions;
}

const unsigned char* Image::GetData() const noexcept {
    return m_texelBytes.data();
}

unsigned char* Image::GetData() noexcept {
    return m_texelBytes.data();
}

std::size_t Image::GetDataLength() const noexcept {
    return static_cast<std::size_t>(m_dimensions.x) * m_dimensions.y * m_bytesPerTexel;
}

int Image::GetBytesPerTexel() const noexcept {
    return m_bytesPerTexel;
}

bool Image::Export(std::filesystem::path filepath, int bytes_per_pixel /*= 4*/, int jpg_quality /*= 100*/) const noexcept {
    if(m_texelBytes.empty()) {
        DebuggerPrintf("Attempting to write empty Image: %s", filepath.string().c_str());
        return false;
    }
    namespace FS = std::filesystem;
    filepath = FS::absolute(filepath);
    filepath.make_preferred();
    std::string extension = StringUtils::ToLowerCase(filepath.extension().string());
    std::string p_str = filepath.string();
    const auto& dims = GetDimensions();
    int w = dims.x;
    int h = dims.y;
    int bbp = bytes_per_pixel;
    int stride = bbp * w;
    int quality = std::clamp(jpg_quality, 0, 100);
    int result = 0;
    if(extension == ".png") {
        std::scoped_lock<std::mutex> lock(_cs);
        result = stbi_write_png(p_str.c_str(), w, h, bbp, m_texelBytes.data(), stride);
    } else if(extension == ".bmp") {
        std::scoped_lock<std::mutex> lock(_cs);
        result = stbi_write_bmp(p_str.c_str(), w, h, bbp, m_texelBytes.data());
    } else if(extension == ".tga") {
        std::scoped_lock<std::mutex> lock(_cs);
        result = stbi_write_tga(p_str.c_str(), w, h, bbp, m_texelBytes.data());
    } else if(extension == ".jpg") {
        std::scoped_lock<std::mutex> lock(_cs);
        result = stbi_write_jpg(p_str.c_str(), w, h, bbp, m_texelBytes.data(), quality);
    } else if(extension == ".hdr") {
        const auto ss = std::string{"Attempting to export "} + filepath.string() + " to an unsupported type: " + extension + "\nHigh Dynamic Range output is not supported.";
        ERROR_RECOVERABLE(ss.c_str());
    }
    return 0 != result;
}

Image Image::CreateImageFromFileBuffer(const std::vector<unsigned char>& data) noexcept {
    if(data.empty()) {
        DebuggerPrintf("Attempting to create image from empty data buffer.\n");
        return {};
    }
    int dim_x = 0;
    int dim_y = 0;
    int comp = 0;
    std::vector<unsigned char> texel_bytes{};
    {
        int req_comp = 4;
        auto* bytes = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &dim_x, &dim_y, &comp, req_comp);
        if(!bytes) {
            DebuggerPrintf("Data does not represent an image.\n");
            return {};
        }
        std::size_t size = static_cast<std::size_t>(dim_x) * dim_y * comp;
        texel_bytes.assign(bytes, bytes + size);
        stbi_image_free(bytes);
        bytes = nullptr;
    }
    Image result{};
    result.m_dimensions = IntVector2(dim_x, dim_y);
    result.m_bytesPerTexel = comp;
    result.m_texelBytes = texel_bytes;
    return result;
}

std::string Image::GetSupportedExtensionsList() noexcept {
    return std::string(".png,.bmp,.tga,.jpg");
}

void swap(Image& a, Image& b) noexcept {
    std::scoped_lock<std::mutex, std::mutex> lock(a._cs, b._cs);
    std::swap(a.m_bytesPerTexel, b.m_bytesPerTexel);
    std::swap(a.m_dimensions, b.m_dimensions);
    std::swap(a.m_filepath, b.m_filepath);
    std::swap(a.m_texelBytes, b.m_texelBytes);
}
