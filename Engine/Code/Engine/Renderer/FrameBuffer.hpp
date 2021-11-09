#pragma once

#include "Engine/RHI/RHITypes.hpp"

#include <memory>

class Texture;

struct FrameBufferDesc {
    uint32_t width{1600u};
    uint32_t height{900u};
    ImageFormat format{ImageFormat::R8G8B8A8_UNorm};
    uint32_t samples{1u};
    bool SwapChainTarget{false};
};

class FrameBuffer {
public:
    virtual ~FrameBuffer() noexcept = 0;
    virtual const FrameBufferDesc& GetSpecification() const = 0;
    //virtual FrameBufferDesc& GetSpecification() = 0;

    static std::shared_ptr<FrameBuffer> Create(const FrameBufferDesc& desc) noexcept;

    virtual void Bind() noexcept = 0;
    virtual void Unbind() noexcept = 0;

    virtual void Resize(uint32_t newWidth, uint32_t newHeight) noexcept = 0;
    virtual void Invalidate() noexcept = 0;

    virtual const Texture* GetTexture() const noexcept = 0;
    virtual Texture* GetTexture() noexcept = 0;

    virtual const Texture* GetDepthStencil() const noexcept = 0;
    virtual Texture* GetDepthStencil() noexcept = 0;

protected:
private:
    
};
