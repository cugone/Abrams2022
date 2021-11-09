#pragma once

#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/Texture.hpp"

#include <memory>

class Window;
class RHIDevice;
class IntVector2;
class Rgba;

class RHIOutput {
public:
    RHIOutput(const RHIDevice& parent, std::unique_ptr<Window> wnd) noexcept;
    ~RHIOutput() = default;

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

    [[nodiscard]] std::unique_ptr<Texture> CreateBackbuffer() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDepthStencil() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateFullscreenTexture() noexcept;

    const RHIDevice& _parent_device;
    std::unique_ptr<Window> _window = nullptr;
    std::unique_ptr<Texture> _back_buffer = nullptr;
    std::unique_ptr<Texture> _depthstencil = nullptr;

private:
};
