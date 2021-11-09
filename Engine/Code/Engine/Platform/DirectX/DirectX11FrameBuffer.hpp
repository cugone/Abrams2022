#pragma once

#include "Engine/Renderer/FrameBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"

class DirectX11FrameBuffer : public FrameBuffer {
public:
    DirectX11FrameBuffer() noexcept = delete;
    DirectX11FrameBuffer(const DirectX11FrameBuffer& other) noexcept = default;
    DirectX11FrameBuffer(DirectX11FrameBuffer&& r_other) noexcept = default;
    DirectX11FrameBuffer& operator=(const DirectX11FrameBuffer& rhs) noexcept = default;
    DirectX11FrameBuffer& operator=(DirectX11FrameBuffer&& rhs) noexcept = default;
    virtual ~DirectX11FrameBuffer() noexcept;

    explicit DirectX11FrameBuffer(const FrameBufferDesc& desc) noexcept;

    const FrameBufferDesc& GetSpecification() const override;
    //FrameBufferDesc& GetSpecification() override;

    void Resize(uint32_t newWidth, uint32_t newHeight) noexcept override;
    void Invalidate() noexcept override;

    void Bind() noexcept override;
    void Unbind() noexcept override;

    const Texture* GetTexture() const noexcept override;
    Texture* GetTexture() noexcept override;
    
    const Texture* GetDepthStencil() const noexcept override;
    Texture* GetDepthStencil() noexcept override;

protected:
private:
    std::unique_ptr<Texture> m_Texture{};
    std::unique_ptr<Texture> m_DepthStencil{};
    FrameBufferDesc m_Desc{};
};
