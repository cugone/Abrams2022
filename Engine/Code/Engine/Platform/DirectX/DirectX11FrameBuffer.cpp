#include "Engine/Platform/DirectX/DirectX11FrameBuffer.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Texture2D.hpp"
#include "Engine/RHI/RHIDevice.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

DirectX11FrameBuffer::~DirectX11FrameBuffer() noexcept {
    Unbind();
    m_Texture.reset();
    m_DepthStencil.reset();
}

DirectX11FrameBuffer::DirectX11FrameBuffer(const FrameBufferDesc& desc) noexcept
    : m_Desc{desc}
{
    Invalidate();
}

const FrameBufferDesc& DirectX11FrameBuffer::GetSpecification() const {
    return m_Desc;
}

void DirectX11FrameBuffer::Resize(uint32_t newWidth, uint32_t newHeight) noexcept {
    if(newWidth && newHeight) {
        m_Desc.width = newWidth;
        m_Desc.height = newHeight;
        Invalidate();
    } else {
        if(m_Texture || m_DepthStencil) {
            m_Texture.reset();
            m_DepthStencil.reset();
        }
    }
}

void DirectX11FrameBuffer::Invalidate() noexcept {

    if(m_Texture || m_DepthStencil) {
        m_Texture.reset();
        m_DepthStencil.reset();
    }
    auto& renderer = ServiceLocator::get<IRendererService>();
    const auto data = std::vector<Rgba>(m_Desc.width * m_Desc.height, Rgba::Periwinkle);
    const auto usage = BufferBindUsage::Shader_Resource | BufferBindUsage::Render_Target;
    m_Texture = renderer.Create2DTextureFromMemory(data, m_Desc.width, m_Desc.height, BufferUsage::Default, usage, m_Desc.format);
    m_DepthStencil = renderer.CreateDepthStencil(*renderer.GetDevice(), IntVector2{(int)m_Desc.width, (int)m_Desc.height});

}

void DirectX11FrameBuffer::Bind() noexcept {
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetRenderTarget(m_Texture.get(), m_DepthStencil.get());
}

void DirectX11FrameBuffer::Unbind() noexcept {
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetRenderTarget(nullptr, nullptr);
}

const Texture* DirectX11FrameBuffer::GetTexture() const noexcept {
    return m_Texture.get();
}

Texture* DirectX11FrameBuffer::GetTexture() noexcept {
    return m_Texture.get();
}

const Texture* DirectX11FrameBuffer::GetDepthStencil() const noexcept {
    return m_DepthStencil.get();
}

Texture* DirectX11FrameBuffer::GetDepthStencil() noexcept {
    return m_DepthStencil.get();
}

//FrameBufferDesc& DirectX11FrameBuffer::GetSpecification() {
//    throw std::logic_error("The method or operation is not implemented.");
//}
