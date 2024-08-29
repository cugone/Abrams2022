#include "Engine/Renderer/StructuredBuffer.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

StructuredBuffer::StructuredBuffer(const RHIDevice& owner, const buffer_t& buffer, std::size_t element_size, std::size_t element_count, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept
: Buffer<void*>()
, m_element_count(element_count)
, m_element_size(element_size)
, m_buffer_size(element_size * element_count) {
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.Usage = BufferUsageToD3DUsage(usage);
    buffer_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    buffer_desc.CPUAccessFlags = CPUAccessFlagFromUsage(usage);
    buffer_desc.StructureByteStride = static_cast<unsigned int>(m_element_size);
    buffer_desc.ByteWidth = static_cast<unsigned int>(m_buffer_size);
    buffer_desc.MiscFlags = ResourceMiscFlagToD3DMiscFlag(ResourceMiscFlag::Structured_Buffer);

    D3D11_SUBRESOURCE_DATA init_data{};
    init_data.pSysMem = buffer;

    m_dx_buffer = nullptr;
    HRESULT hr = owner.GetDxDevice()->CreateBuffer(&buffer_desc, &init_data, m_dx_buffer.GetAddressOf());
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "StructuredBuffer failed to create.");

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0U;
    srv_desc.Buffer.NumElements = static_cast<unsigned int>(element_count);

    hr = owner.GetDxDevice()->CreateShaderResourceView(m_dx_buffer.Get(), &srv_desc, dx_srv.GetAddressOf());
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create StructuredBuffer's SRV.");
}

StructuredBuffer::~StructuredBuffer() noexcept {
    if(IsValid()) {
        m_dx_buffer.Reset();
        m_dx_buffer = nullptr;
    }
}

void StructuredBuffer::Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept {
    D3D11_MAPPED_SUBRESOURCE resource = {};
    auto* dx_context = context.GetDxContext();
    HRESULT hr = dx_context->Map(m_dx_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        std::memcpy(resource.pData, buffer, m_buffer_size);
        dx_context->Unmap(m_dx_buffer.Get(), 0);
    }
}
