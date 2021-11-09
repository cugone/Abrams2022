#include "Engine/Renderer/VertexBufferInstanced.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

VertexBufferInstanced::VertexBufferInstanced(const RHIDevice& owner, const buffer_t& buffer, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept
: ArrayBuffer<Vertex3DInstanced>() {
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.Usage = BufferUsageToD3DUsage(usage);
    buffer_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    buffer_desc.CPUAccessFlags = CPUAccessFlagFromUsage(usage);
    buffer_desc.StructureByteStride = sizeof(arraybuffer_t);
    buffer_desc.ByteWidth = sizeof(arraybuffer_t) * static_cast<unsigned int>(buffer.size());
    //MiscFlags are unused.

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = buffer.data();

    _dx_buffer = nullptr;
    HRESULT hr = owner.GetDxDevice()->CreateBuffer(&buffer_desc, &init_data, _dx_buffer.GetAddressOf());
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "VertexBuffer failed to create.");
}

VertexBufferInstanced::~VertexBufferInstanced() noexcept {
    if(IsValid()) {
        _dx_buffer.Reset();
        _dx_buffer = nullptr;
    }
}

void VertexBufferInstanced::Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept {
    D3D11_MAPPED_SUBRESOURCE resource{};
    auto* dx_context = context.GetDxContext();
    HRESULT hr = dx_context->Map(_dx_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        std::memcpy(resource.pData, buffer.data(), sizeof(arraybuffer_t) * buffer.size());
        dx_context->Unmap(_dx_buffer.Get(), 0);
    }
}
