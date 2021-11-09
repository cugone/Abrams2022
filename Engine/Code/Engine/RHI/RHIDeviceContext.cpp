#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Core/Rgba.hpp"
#include "Engine/Renderer/BlendState.hpp"
#include "Engine/Renderer/Buffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DepthStencilState.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/InputLayout.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/RasterState.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Sampler.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"
#include "Engine/Renderer/StructuredBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

RHIDeviceContext::RHIDeviceContext(const RHIDevice& parentDevice, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext) noexcept
: _device(parentDevice)
, _dx_context(deviceContext) {
    /* DO NOTHING */
}

void RHIDeviceContext::ClearState() noexcept {
    _dx_context->ClearState();
}

void RHIDeviceContext::Flush() noexcept {
    _dx_context->Flush();
}

void RHIDeviceContext::ClearColorTarget(Texture* output, const Rgba& color) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    _dx_context->ClearRenderTargetView(output->GetRenderTargetView(), Vector4{r, g, b, a}.GetAsFloatArray());
}

void RHIDeviceContext::ClearDepthStencilTarget(Texture* output, bool depth /*= true */, bool stencil /*= true */, float depthValue /*= 1.0f */, unsigned char stencilValue /*= 0*/) noexcept {
    auto clear_flag = 0u;
    if(depth) {
        clear_flag |= D3D11_CLEAR_DEPTH;
    }
    if(stencil) {
        clear_flag |= D3D11_CLEAR_STENCIL;
    }
    _dx_context->ClearDepthStencilView(output->GetDepthStencilView(), clear_flag, depthValue, stencilValue);
}

void RHIDeviceContext::SetMaterial(Material* material) noexcept {
    if(material == nullptr) {
        return;
    }
    SetShader(material->GetShader());
    const auto tex_count = material->GetTextureCount();
    for(unsigned int i = 0; i < tex_count; ++i) {
        SetTexture(i, material->GetTexture(i));
    }
}

void RHIDeviceContext::SetTexture(unsigned int index, Texture* texture) noexcept {
    if(texture) {
        ID3D11ShaderResourceView* srvs = {texture->GetShaderResourceView()};
        _dx_context->VSSetShaderResources(index, 1, &srvs);
        _dx_context->PSSetShaderResources(index, 1, &srvs);
        _dx_context->DSSetShaderResources(index, 1, &srvs);
        _dx_context->HSSetShaderResources(index, 1, &srvs);
        _dx_context->GSSetShaderResources(index, 1, &srvs);
    } else {
        ID3D11ShaderResourceView* no_srvs = {nullptr};
        _dx_context->VSSetShaderResources(index, 1, &no_srvs);
        _dx_context->PSSetShaderResources(index, 1, &no_srvs);
        _dx_context->DSSetShaderResources(index, 1, &no_srvs);
        _dx_context->HSSetShaderResources(index, 1, &no_srvs);
        _dx_context->GSSetShaderResources(index, 1, &no_srvs);
    }
}

void RHIDeviceContext::SetUnorderedAccessView(unsigned int index, Texture* texture) noexcept {
    if(texture) {
        ID3D11UnorderedAccessView* uavs = {texture->GetUnorderedAccessView()};
        _dx_context->CSSetUnorderedAccessViews(index, 1, &uavs, nullptr);
    } else {
        ID3D11UnorderedAccessView* no_uavs = {nullptr};
        _dx_context->CSSetUnorderedAccessViews(index, 1, &no_uavs, nullptr);
    }
}

void RHIDeviceContext::SetVertexBuffer(unsigned int startIndex, VertexBuffer* buffer) noexcept {
    unsigned int stride = sizeof(VertexBuffer::buffer_t);
    unsigned int offsets = 0u;
    if(buffer) {
        ID3D11Buffer* const dx_buffer = buffer->GetDxBuffer().Get();
        _dx_context->IASetVertexBuffers(startIndex, 1, &dx_buffer, &stride, &offsets);
    } else {
        ID3D11Buffer* nobuffer[1] = {nullptr};
        _dx_context->IASetVertexBuffers(startIndex, 1, nobuffer, &stride, &offsets);
    }
}

void RHIDeviceContext::SetIndexBuffer(IndexBuffer* buffer) noexcept {
    if(buffer) {
        ID3D11Buffer* const dx_buffer = buffer->GetDxBuffer().Get();
        _dx_context->IASetIndexBuffer(dx_buffer, DXGI_FORMAT_R32_UINT, 0);
    } else {
        _dx_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    }
}

void RHIDeviceContext::SetConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept {
    if(buffer) {
        ID3D11Buffer* const dx_buffer = buffer->GetDxBuffer().Get();
        _dx_context->VSSetConstantBuffers(index, 1, &dx_buffer);
        _dx_context->PSSetConstantBuffers(index, 1, &dx_buffer);
        _dx_context->DSSetConstantBuffers(index, 1, &dx_buffer);
        _dx_context->HSSetConstantBuffers(index, 1, &dx_buffer);
        _dx_context->GSSetConstantBuffers(index, 1, &dx_buffer);
    } else {
        ID3D11Buffer* nobuffer[1] = {nullptr};
        _dx_context->VSSetConstantBuffers(index, 1, nobuffer);
        _dx_context->PSSetConstantBuffers(index, 1, nobuffer);
        _dx_context->DSSetConstantBuffers(index, 1, nobuffer);
        _dx_context->HSSetConstantBuffers(index, 1, nobuffer);
        _dx_context->GSSetConstantBuffers(index, 1, nobuffer);
    }
}

void RHIDeviceContext::SetStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept {
    if(buffer) {
        ID3D11ShaderResourceView* const dx_buffer = buffer->dx_srv.Get();
        _dx_context->VSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
        _dx_context->PSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
        _dx_context->DSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
        _dx_context->HSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
        _dx_context->GSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
        _dx_context->CSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
    } else {
        ID3D11ShaderResourceView* nobuffer[1] = {nullptr};
        _dx_context->VSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
        _dx_context->PSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
        _dx_context->DSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
        _dx_context->HSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
        _dx_context->GSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
        _dx_context->CSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
    }
}

void RHIDeviceContext::SetComputeTexture(unsigned int index, Texture* texture) noexcept {
    if(texture) {
        ID3D11ShaderResourceView* srvs = {texture->GetShaderResourceView()};
        _dx_context->CSSetShaderResources(index, 1, &srvs);
    } else {
        ID3D11ShaderResourceView* no_srvs = {nullptr};
        _dx_context->CSSetShaderResources(index, 1, &no_srvs);
    }
}

void RHIDeviceContext::SetComputeConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept {
    if(buffer) {
        ID3D11Buffer* const dx_buffer = buffer->GetDxBuffer().Get();
        _dx_context->CSSetConstantBuffers(index, 1, &dx_buffer);
    } else {
        ID3D11Buffer* nobuffer[1] = {nullptr};
        _dx_context->CSSetConstantBuffers(index, 1, nobuffer);
    }
}

void RHIDeviceContext::SetComputeStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept {
    if(buffer) {
        ID3D11ShaderResourceView* const dx_buffer = buffer->dx_srv.Get();
        _dx_context->CSSetShaderResources(index + StructuredBufferSlotOffset, 1, &dx_buffer);
    } else {
        ID3D11ShaderResourceView* nobuffer[1] = {nullptr};
        _dx_context->CSSetShaderResources(index + StructuredBufferSlotOffset, 1, nobuffer);
    }
}

void RHIDeviceContext::Draw(std::size_t vertexCount, std::size_t startVertex /*= 0*/) noexcept {
    _dx_context->Draw(static_cast<unsigned int>(vertexCount), static_cast<unsigned int>(startVertex));
}

void RHIDeviceContext::DrawInstanced(std::size_t vertexCountPerInstance, std::size_t instanceCount, std::size_t startVertexLocation, std::size_t startInstanceLocation) noexcept {
    _dx_context->DrawInstanced(static_cast<unsigned int>(vertexCountPerInstance), static_cast<unsigned int>(instanceCount), static_cast<unsigned int>(startVertexLocation), static_cast<unsigned int>(startInstanceLocation));
}

void RHIDeviceContext::DrawIndexed(std::size_t vertexCount, std::size_t startVertex /*= 0*/, std::size_t baseVertexLocation /*= 0*/) noexcept {
    _dx_context->DrawIndexed(static_cast<unsigned int>(vertexCount), static_cast<unsigned int>(startVertex), static_cast<int>(baseVertexLocation));
}

void RHIDeviceContext::DrawIndexedInstanced(std::size_t indexCountPerInstance, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept {
    _dx_context->DrawIndexedInstanced(static_cast<unsigned int>(indexCountPerInstance), static_cast<unsigned int>(instanceCount), static_cast<unsigned int>(startIndexLocation), static_cast<unsigned int>(baseVertexLocation), static_cast<unsigned int>(startInstanceLocation));
}

const RHIDevice* RHIDeviceContext::GetParentDevice() const noexcept {
    return &_device;
}

ID3D11DeviceContext* RHIDeviceContext::GetDxContext() noexcept {
    return _dx_context.Get();
}

void RHIDeviceContext::SetComputeShaderProgram(ShaderProgram* shaderProgram /*= nullptr*/) noexcept {
    if(shaderProgram == nullptr) {
        _dx_context->CSSetShader(nullptr, nullptr, 0);
    } else {
        _dx_context->CSSetShader(shaderProgram->GetCS(), nullptr, 0);
    }
}

void RHIDeviceContext::SetShaderProgram(ShaderProgram* shaderProgram /*= nullptr*/) noexcept {
    if(shaderProgram == nullptr) {
        _dx_context->IASetInputLayout(nullptr);
        _dx_context->VSSetShader(nullptr, nullptr, 0);
        _dx_context->PSSetShader(nullptr, nullptr, 0);
        _dx_context->DSSetShader(nullptr, nullptr, 0);
        _dx_context->HSSetShader(nullptr, nullptr, 0);
        _dx_context->GSSetShader(nullptr, nullptr, 0);
    } else {
        auto* il = shaderProgram->GetInputLayout();
        auto* dx_il = il ? il->GetDxInputLayout() : nullptr;
        _dx_context->IASetInputLayout(dx_il);
        _dx_context->VSSetShader(shaderProgram->GetVS(), nullptr, 0);
        _dx_context->PSSetShader(shaderProgram->GetPS(), nullptr, 0);
        _dx_context->DSSetShader(shaderProgram->GetDS(), nullptr, 0);
        _dx_context->HSSetShader(shaderProgram->GetHS(), nullptr, 0);
        _dx_context->GSSetShader(shaderProgram->GetGS(), nullptr, 0);
    }
}

void RHIDeviceContext::SetDepthStencilState(DepthStencilState* depthStencilState /*= nullptr*/) noexcept {
    if(depthStencilState == nullptr) {
        _dx_context->OMSetDepthStencilState(nullptr, 0);
    } else {
        _dx_context->OMSetDepthStencilState(depthStencilState->GetDxDepthStencilState(), 0);
    }
}

void RHIDeviceContext::SetRasterState(RasterState* rasterState /*= nullptr*/) noexcept {
    if(rasterState == nullptr) {
        _dx_context->RSSetState(nullptr);
    } else {
        _dx_context->RSSetState(rasterState->GetDxRasterState());
    }
}

void RHIDeviceContext::SetBlendState(BlendState* blendState /*= nullptr*/) noexcept {
    unsigned int mask = 0xffffffff;
    float blend_factor[4] = {1, 1, 1, 1};
    if(blendState == nullptr) {
        _dx_context->OMSetBlendState(nullptr, blend_factor, mask);
    } else {
        _dx_context->OMSetBlendState(blendState->GetDxBlendState(), blend_factor, mask);
    }
}

void RHIDeviceContext::SetSampler(Sampler* sampler) noexcept {
    if(sampler == nullptr) {
        ID3D11SamplerState* no_samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {nullptr};
        _dx_context->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
        _dx_context->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
        _dx_context->DSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
        _dx_context->HSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
        _dx_context->GSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
        _dx_context->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, no_samplers);
    } else {
        ID3D11SamplerState* const dx_sampler = sampler->GetDxSampler();
        _dx_context->VSSetSamplers(0, 1, &dx_sampler);
        _dx_context->PSSetSamplers(0, 1, &dx_sampler);
        _dx_context->DSSetSamplers(0, 1, &dx_sampler);
        _dx_context->HSSetSamplers(0, 1, &dx_sampler);
        _dx_context->GSSetSamplers(0, 1, &dx_sampler);
        _dx_context->CSSetSamplers(0, 1, &dx_sampler);
    }
}

void RHIDeviceContext::UnbindAllConstantBuffers() noexcept {
    UnbindConstantBuffers();
    UnbindAllComputeConstantBuffers();
}

void RHIDeviceContext::UnbindConstantBuffers() noexcept {
    constexpr auto nobuffers_count = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    ID3D11Buffer* nobuffers[nobuffers_count] = {nullptr};
    _dx_context->VSSetConstantBuffers(0, nobuffers_count, nobuffers);
    _dx_context->PSSetConstantBuffers(0, nobuffers_count, nobuffers);
    _dx_context->DSSetConstantBuffers(0, nobuffers_count, nobuffers);
    _dx_context->HSSetConstantBuffers(0, nobuffers_count, nobuffers);
    _dx_context->GSSetConstantBuffers(0, nobuffers_count, nobuffers);
}

void RHIDeviceContext::UnbindShaderResources() noexcept {
    ID3D11ShaderResourceView* no_srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {nullptr};
    _dx_context->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
    _dx_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
    _dx_context->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
    _dx_context->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
    _dx_context->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
}

void RHIDeviceContext::UnbindAllCustomConstantBuffers() noexcept {
    const auto& renderer = ServiceLocator::get<IRendererService>();
    const auto startSlot = renderer.GetConstantBufferStartIndex();
    const auto nobuffers_count = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - startSlot;
    const std::vector<ID3D11Buffer*> nobuffers(nobuffers_count, nullptr);
    _dx_context->VSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
    _dx_context->PSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
    _dx_context->DSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
    _dx_context->HSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
    _dx_context->GSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
}

void RHIDeviceContext::UnbindComputeShaderResources() noexcept {
    ID3D11ShaderResourceView* no_srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {nullptr};
    _dx_context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, no_srvs);
}

void RHIDeviceContext::UnbindAllComputeUAVs() noexcept {
    ID3D11UnorderedAccessView* no_uavs[D3D11_1_UAV_SLOT_COUNT] = {nullptr};
    _dx_context->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, no_uavs, nullptr);
}

void RHIDeviceContext::UnbindComputeCustomConstantBuffers() noexcept {
    const auto& renderer = ServiceLocator::get<IRendererService>();
    const auto startSlot = renderer.GetConstantBufferStartIndex();
    const auto nobuffers_count = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - startSlot;
    const std::vector<ID3D11Buffer*> nobuffers(nobuffers_count, nullptr);
    _dx_context->CSSetConstantBuffers(startSlot, nobuffers_count, nobuffers.data());
}

void RHIDeviceContext::UnbindAllComputeConstantBuffers() noexcept {
    constexpr auto nobuffers_count = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    ID3D11Buffer* nobuffers[nobuffers_count] = {nullptr};
    _dx_context->CSSetConstantBuffers(0, nobuffers_count, nobuffers);
}

void RHIDeviceContext::UnbindAllShaderResources() noexcept {
    UnbindShaderResources();
    UnbindComputeShaderResources();
}

void RHIDeviceContext::SetShader(Shader* shader) noexcept {
    if(shader == nullptr) {
        SetShaderProgram(nullptr);
        SetRasterState(nullptr);
        SetDepthStencilState(nullptr);
        SetBlendState(nullptr);
        SetSampler(nullptr);
        UnbindAllCustomConstantBuffers();
    } else {
        SetShaderProgram(shader->GetShaderProgram());
        SetRasterState(shader->GetRasterState());
        SetDepthStencilState(shader->GetDepthStencilState());
        SetBlendState(shader->GetBlendState());
        SetSampler(shader->GetSampler());
        UnbindAllCustomConstantBuffers();
        const auto& cbs = shader->GetConstantBuffers();
        const auto s = cbs.size();
        const auto& renderer = ServiceLocator::get<IRendererService>();
        const auto startSlot = renderer.GetConstantBufferStartIndex();
        for(auto i = 0u; i < s; ++i) {
            SetConstantBuffer(i + startSlot, &(cbs[i].get()));
        }
    }
}
