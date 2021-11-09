#pragma once

#include "Engine/Math/IntVector2.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"
#include "Engine/RHI/RHIFactory.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/StructuredBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/VertexBufferInstanced.hpp"

#include <filesystem>
#include <memory>
#include <set>
#include <vector>

class RHIDeviceContext;
class RHIFactory;
class IntVector2;
class Window;
class RHIOutput;
class DepthStencilState;
class InputLayout;
class InputLayoutInstanced;
struct Vertex3D;
struct Vertex3DInstanced;
class ShaderProgram;
class Renderer;
struct WindowDesc;
struct ShaderProgramDesc;

class RHIDevice {
public:
    RHIDevice() noexcept = default;
    ~RHIDevice() = default;

    [[nodiscard]] std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> CreateOutputAndContext(const IntVector2& clientSize, const IntVector2& clientPosition = IntVector2::Zero) noexcept;
    [[nodiscard]] std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> CreateOutputAndContext(const WindowDesc& desc) noexcept;

    [[nodiscard]] std::unique_ptr<VertexBuffer> CreateVertexBuffer(const VertexBuffer::buffer_t& vbo, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept;
    [[nodiscard]] std::unique_ptr<VertexBufferInstanced> CreateVertexBufferInstanced(const VertexBufferInstanced::buffer_t& vbio, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept;
    [[nodiscard]] std::unique_ptr<IndexBuffer> CreateIndexBuffer(const IndexBuffer::buffer_t& ibo, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept;

    [[nodiscard]] std::unique_ptr<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::buffer_t& buffer, std::size_t element_size, std::size_t element_count, const BufferUsage& usage, const BufferBindUsage& bindUsage) const noexcept;
    [[nodiscard]] std::unique_ptr<ConstantBuffer> CreateConstantBuffer(const ConstantBuffer::buffer_t& buffer, std::size_t buffer_size, const BufferUsage& usage, const BufferBindUsage& bindUsage) const noexcept;

    void CreateVertexShader(ShaderProgramDesc& desc) const noexcept;
    void CreateHullShader(ShaderProgramDesc& desc) const noexcept;
    void CreateDomainShader(ShaderProgramDesc& desc) const noexcept;
    void CreateGeometryShader(ShaderProgramDesc& desc) const noexcept;
    void CreatePixelShader(ShaderProgramDesc& desc) const noexcept;
    void CreateComputeShader(ShaderProgramDesc& desc) const noexcept;

    [[nodiscard]] D3D_FEATURE_LEVEL GetFeatureLevel() const noexcept;
    [[nodiscard]] ID3D11Device5* GetDxDevice() const noexcept;
    [[nodiscard]] IDXGISwapChain4* GetDxSwapChain() const noexcept;
    [[nodiscard]] bool IsAllowTearingSupported() const noexcept;

    [[nodiscard]] static std::unique_ptr<ShaderProgram> CreateShaderProgramFromCsoBinaryBuffer(RHIDevice& device, std::vector<uint8_t>& compiledShader, const std::string& name, const PipelineStage& target) noexcept;
    [[nodiscard]] static std::unique_ptr<ShaderProgram> CreateShaderProgramFromCsoFile(RHIDevice& device, std::filesystem::path filepath, const PipelineStage& target) noexcept;

    [[nodiscard]] static ID3DBlob* CompileShader(const std::string& name, const void* sourceCode, std::size_t sourceCodeSize, const std::string& entryPoint, const PipelineStage& target) noexcept;
    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateConstantBuffersFromByteCode(RHIDevice& device, ID3DBlob* bytecode) noexcept;

    mutable std::set<DisplayDesc, DisplayDescGTComparator> displayModes{};

    void ResetSwapChainForHWnd() const noexcept;

    void HandleDeviceLost() const noexcept;

    static void CreateInputLayout(InputLayout& layout, RHIDevice& device, void* byte_code, std::size_t byte_code_length) noexcept;

    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept;
    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateComputeConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept;
    [[nodiscard]] static std::unique_ptr<InputLayout> CreateInputLayoutFromByteCode(RHIDevice& device, ID3DBlob* bytecode) noexcept;
    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateConstantBuffersUsingReflection(RHIDevice& device, ID3D11ShaderReflection& cbufferReflection) noexcept;
    [[nodiscard]] static std::unique_ptr<InputLayoutInstanced> CreateInputLayoutInstancedFromByteCode(RHIDevice& device, ID3DBlob* vs_bytecode) noexcept;

private:
    [[nodiscard]] std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> CreateOutputAndContextFromWindow(std::unique_ptr<Window> window) noexcept;

    [[nodiscard]] DeviceInfo CreateDeviceFromFirstAdapter(const std::vector<AdapterInfo>& adapters) noexcept;
    void OutputAdapterInfo(const std::vector<AdapterInfo>& adapters) const noexcept;
    void GetDisplayModes(const std::vector<AdapterInfo>& adapters) const noexcept;

    [[nodiscard]] Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(const Window& window) noexcept;
    [[nodiscard]] Microsoft::WRL::ComPtr<IDXGISwapChain4> RecreateSwapChain(const Window& window) noexcept;

    [[nodiscard]] std::vector<OutputInfo> GetOutputsFromAdapter(const AdapterInfo& a) const noexcept;
    void GetPrimaryDisplayModeDescriptions(const AdapterInfo& adapter, decltype(displayModes)& descriptions) const noexcept;
    void GetDisplayModeDescriptions(const AdapterInfo& adapter, const OutputInfo& output, decltype(displayModes)& descriptions) const noexcept;
    [[nodiscard]] DisplayDesc GetDisplayModeMatchingDimensions(const std::vector<DisplayDesc>& descriptions, unsigned int w, unsigned int h) noexcept;

    RHIFactory _rhi_factory{};
    D3D_FEATURE_LEVEL _dx_highestSupportedFeatureLevel{};
    Microsoft::WRL::ComPtr<IDXGISwapChain4> _dxgi_swapchain{};
    Microsoft::WRL::ComPtr<ID3D11Device5> _dx_device{};
    bool _allow_tearing_supported = false;

    void SetupDebuggingInfo([[maybe_unused]] bool breakOnWarningSeverityOrLower = true) noexcept;
};
