#include "Engine/Renderer/FrameBuffer.hpp"

#include "Engine/Platform/DirectX/DirectX11FrameBuffer.hpp"

FrameBuffer::~FrameBuffer() noexcept {
    /* DO NOTHING */
}

std::shared_ptr<FrameBuffer> FrameBuffer::Create(const FrameBufferDesc& desc) noexcept {
    //TODO: API selection
    return std::make_shared<DirectX11FrameBuffer>(desc);
}
