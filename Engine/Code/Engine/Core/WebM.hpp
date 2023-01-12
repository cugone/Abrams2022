#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/Matrix4.hpp"

#include <chrono>
#include <filesystem>

namespace FileUtils {

class WebM {
public:
    WebM() = default;
    WebM(const WebM& other) = default;
    WebM(WebM&& other) = default;
    WebM& operator=(const WebM& other) = default;
    WebM& operator=(WebM&& other) = default;
    ~WebM() = default;

    explicit WebM(std::filesystem::path filesystem) noexcept;

    bool Load(std::filesystem::path filepath) noexcept;

    std::filesystem::path GetFilepath() const noexcept;

    void SetPixelDimensions(uint64_t width, uint64_t height) noexcept;
    void SetDuration(TimeUtils::FPSeconds newDuration) noexcept;

    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render([[maybe_unused]] const Matrix4& transform = Matrix4::I) const noexcept;

    auto GetDimensions() const noexcept -> const std::pair<uint64_t, uint64_t>;
    void AddFrame(const std::vector<uint8_t>& frameData) noexcept;

    void BindEncodedBufferToGpu(const std::vector<uint8_t>& encodedFrame) const noexcept;

protected:
private:
    std::filesystem::path m_path{};
    TimeUtils::FPSeconds m_length{};
    std::vector<uint8_t> m_frameData{};
    uint64_t m_frameCount{};
    uint64_t m_width{};
    uint64_t m_height{};
};

} // namespace FileUtils