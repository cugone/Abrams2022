#pragma once

#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"

#include <memory>

class Window;
class RHIDevice;
class IntVector2;
class Rgba;

class RHIOutput {
public:
    RHIOutput(const RHIDevice& parent, std::unique_ptr<Window> wnd) noexcept;
    ~RHIOutput() noexcept = default;

    [[nodiscard]] const RHIDevice& GetParentDevice() const noexcept;

    [[nodiscard]] const Window* GetWindow() const noexcept;
    [[nodiscard]] Window* GetWindow() noexcept;

    [[nodiscard]] Texture* GetBackBuffer() const noexcept;
    [[nodiscard]] Texture* GetDepthStencil() const noexcept;

    void ResetBackbuffer() noexcept;

    [[nodiscard]] IntVector2 GetDimensions() const noexcept;
    [[nodiscard]] IntVector2 GetCenter() const noexcept;
    [[nodiscard]] float GetAspectRatio() const noexcept;

    void SetDisplayMode(const RHIOutputMode& newMode) noexcept;
    void SetDimensions(const IntVector2& clientSize) noexcept;
    void SetTitle(const std::string& newTitle) const noexcept;

    void Present(bool vsync) noexcept;

protected:
    void CreateBuffers() noexcept;

    const RHIDevice& m_parent_device;
    std::unique_ptr<Window> m_window = nullptr;
    std::shared_ptr<FrameBuffer> m_backbuffer{};

private:
};
