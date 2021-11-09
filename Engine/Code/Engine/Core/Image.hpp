#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/IntVector2.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

class Renderer;
class Texture;

class Image {
public:
    Image() = default;
    explicit Image(std::filesystem::path filepath) noexcept;
    Image(const Image& img) = delete;
    Image& operator=(const Image& rhs) = delete;
    Image(Image&& img) noexcept;
    Image& operator=(Image&& rhs) noexcept;
    ~Image() = default;

    explicit Image(const Texture* tex, const Renderer* renderer) noexcept;
    explicit Image(const Texture* tex) noexcept;
    Image(unsigned char* data, unsigned int width, unsigned int height) noexcept;
    Image(Rgba* data, unsigned int width, unsigned int height) noexcept;
    Image(unsigned int width, unsigned int height) noexcept;
    Image(const std::vector<Rgba>& data, unsigned int width, unsigned int height) noexcept;
    Image(const std::vector<unsigned char>& data, unsigned int width, unsigned int height) noexcept;

    [[nodiscard]] Rgba GetTexel(const IntVector2& texelPos) const noexcept;
    void SetTexel(const IntVector2& texelPos, const Rgba& color) noexcept;

    [[nodiscard]] const std::filesystem::path& GetFilepath() const noexcept;
    [[nodiscard]] const IntVector2& GetDimensions() const noexcept;

    [[nodiscard]] const unsigned char* GetData() const noexcept;
    [[nodiscard]] unsigned char* GetData() noexcept;
    [[nodiscard]] std::size_t GetDataLength() const noexcept;
    [[nodiscard]] int GetBytesPerTexel() const noexcept;

    [[nodiscard]] bool Export(std::filesystem::path filepath, int bytes_per_pixel = 4, int jpg_quality = 100) const noexcept;
    [[nodiscard]] static Image CreateImageFromFileBuffer(const std::vector<unsigned char>& data) noexcept;
    [[nodiscard]] static std::string GetSupportedExtensionsList() noexcept;

    friend void swap(Image& a, Image& b) noexcept;

protected:
private:
    IntVector2 m_dimensions{};
    unsigned int m_bytesPerTexel = 0;
    std::vector<unsigned char> m_texelBytes{};
    std::filesystem::path m_filepath{};
    mutable std::mutex _cs{};
};