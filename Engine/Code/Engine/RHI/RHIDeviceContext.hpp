#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

#include <vector>

class Texture;
class Rgba;
class RHIDevice;
class VertexBuffer;
class IndexBuffer;
class StructuredBuffer;
class ConstantBuffer;
class Sampler;
class Shader;
class ShaderProgram;
class DepthStencilState;
class RasterState;
class BlendState;
class Material;

enum class BufferType : uint8_t;
enum class PrimitiveType : uint32_t;

class RHIDeviceContext {
public:
    RHIDeviceContext(const RHIDevice& parentDevice, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext) noexcept;
    ~RHIDeviceContext() = default;

    void ClearState() noexcept;
    void Flush() noexcept;

    void ClearColorTarget(Texture* output, const Rgba& color) noexcept;
    void ClearDepthStencilTarget(Texture* output, bool depth = true, bool stencil = true, float depthValue = 1.0f, unsigned char stencilValue = 0) noexcept;

    void SetMaterial(Material* material) noexcept;
    void SetTexture(unsigned int index, Texture* texture) noexcept;
    void SetUnorderedAccessView(unsigned int index, Texture* texture) noexcept;
    void SetVertexBuffer(unsigned int startIndex, VertexBuffer* buffer) noexcept;
    void SetIndexBuffer(IndexBuffer* buffer) noexcept;
    void SetConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept;
    void SetStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept;

    void SetComputeTexture(unsigned int index, Texture* texture) noexcept;
    void SetComputeConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept;
    void SetComputeStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept;

    void Draw(std::size_t vertexCount, std::size_t startVertex = 0) noexcept;
    void DrawInstanced(std::size_t vertexCountPerInstance, std::size_t instanceCount, std::size_t startVertexLocation, std::size_t startInstanceLocation) noexcept;
    void DrawIndexed(std::size_t vertexCount, std::size_t startVertex = 0, std::size_t baseVertexLocation = 0) noexcept;
    void DrawIndexedInstanced(std::size_t indexCountPerInstance, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept;

    [[nodiscard]] const RHIDevice* GetParentDevice() const noexcept;
    [[nodiscard]] ID3D11DeviceContext* GetDxContext() noexcept;

    void UnbindAllShaderResources() noexcept;
    void UnbindAllConstantBuffers() noexcept;

    void UnbindConstantBuffers() noexcept;
    void UnbindShaderResources() noexcept;
    void UnbindAllCustomConstantBuffers() noexcept;
    void UnbindComputeShaderResources() noexcept;
    void UnbindAllComputeUAVs() noexcept;
    void UnbindComputeCustomConstantBuffers() noexcept;
    void UnbindAllComputeConstantBuffers() noexcept;

private:
    void SetShader(Shader* shader) noexcept;
    void SetShaderProgram(ShaderProgram* shaderProgram = nullptr) noexcept;
    void SetComputeShaderProgram(ShaderProgram* shaderProgram = nullptr) noexcept;
    void SetDepthStencilState(DepthStencilState* depthStencilState = nullptr) noexcept;
    void SetRasterState(RasterState* rasterState = nullptr) noexcept;
    void SetBlendState(BlendState* blendState = nullptr) noexcept;
    void SetSampler(Sampler* sampler = nullptr) noexcept;

    static constexpr unsigned int StructuredBufferSlotOffset = (D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT / 2);

    const RHIDevice& _device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _dx_context{};

    friend class Renderer;
};
