#include "Engine/RHI/RHIDevice.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"
#include "Engine/RHI/RHIFactory.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/DepthStencilState.hpp"
#include "Engine/Renderer/InputLayout.hpp"
#include "Engine/Renderer/InputLayoutInstanced.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <array>
#include <sstream>
#include <numeric>

std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> RHIDevice::CreateOutputAndContext(const IntVector2& clientSize, const IntVector2& clientPosition /*= IntVector2::ZERO*/) noexcept {
    auto window = std::make_unique<Window>(clientPosition, clientSize);
    return CreateOutputAndContextFromWindow(std::move(window));
}

std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> RHIDevice::CreateOutputAndContext(const WindowDesc& desc) noexcept {
    auto window = std::make_unique<Window>(desc);
    return CreateOutputAndContextFromWindow(std::move(window));
}

D3D_FEATURE_LEVEL RHIDevice::GetFeatureLevel() const noexcept {
    return _dx_highestSupportedFeatureLevel;
}

ID3D11Device5* RHIDevice::GetDxDevice() const noexcept {
    return _dx_device.Get();
}

IDXGISwapChain4* RHIDevice::GetDxSwapChain() const noexcept {
    return _dxgi_swapchain.Get();
}

bool RHIDevice::IsAllowTearingSupported() const noexcept {
    return _allow_tearing_supported;
}

std::unique_ptr<VertexBuffer> RHIDevice::CreateVertexBuffer(const VertexBuffer::buffer_t& vbo, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept {
    return std::make_unique<VertexBuffer>(*this, vbo, usage, bindusage);
}

std::unique_ptr<VertexBufferInstanced> RHIDevice::CreateVertexBufferInstanced(const VertexBufferInstanced::buffer_t& vbio, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept {
    return std::make_unique<VertexBufferInstanced>(*this, vbio, usage, bindusage);
}

std::unique_ptr<IndexBuffer> RHIDevice::CreateIndexBuffer(const IndexBuffer::buffer_t& ibo, const BufferUsage& usage, const BufferBindUsage& bindusage) const noexcept {
    return std::make_unique<IndexBuffer>(*this, ibo, usage, bindusage);
}

void RHIDevice::CreateInputLayout(InputLayout& layout, RHIDevice& device, void* byte_code, std::size_t byte_code_length) noexcept {
    layout._elements.shrink_to_fit();
    if(layout._dx_input_layout) {
        layout._dx_input_layout->Release();
        layout._dx_input_layout = nullptr;
    }
    auto* dx_device = device.GetDxDevice();
    HRESULT hr = dx_device->CreateInputLayout(layout._elements.data(), static_cast<unsigned int>(layout._elements.size()), byte_code, byte_code_length, layout._dx_input_layout.GetAddressOf());
    bool succeeded = SUCCEEDED(hr);
    GUARANTEE_OR_DIE(succeeded, "Create Input Layout failed.");
}

std::unique_ptr<StructuredBuffer> RHIDevice::CreateStructuredBuffer(const StructuredBuffer::buffer_t& buffer, std::size_t element_size, std::size_t element_count, const BufferUsage& usage, const BufferBindUsage& bindUsage) const noexcept {
    return std::make_unique<StructuredBuffer>(*this, buffer, element_size, element_count, usage, bindUsage);
}

std::unique_ptr<ConstantBuffer> RHIDevice::CreateConstantBuffer(const ConstantBuffer::buffer_t& buffer, std::size_t buffer_size, const BufferUsage& usage, const BufferBindUsage& bindUsage) const noexcept {
    return std::make_unique<ConstantBuffer>(*this, buffer, buffer_size, usage, bindUsage);
}

void RHIDevice::CreateVertexShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreateVertexShader(desc.vs_bytecode->GetBufferPointer(), desc.vs_bytecode->GetBufferSize(), nullptr, &desc.vs);
}

void RHIDevice::CreateHullShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreateHullShader(desc.hs_bytecode->GetBufferPointer(), desc.hs_bytecode->GetBufferSize(), nullptr, &desc.hs);
}

void RHIDevice::CreateDomainShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreateDomainShader(desc.ds_bytecode->GetBufferPointer(), desc.ds_bytecode->GetBufferSize(), nullptr, &desc.ds);
}

void RHIDevice::CreateGeometryShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreateGeometryShader(desc.gs_bytecode->GetBufferPointer(), desc.gs_bytecode->GetBufferSize(), nullptr, &desc.gs);
}

void RHIDevice::CreatePixelShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreatePixelShader(desc.ps_bytecode->GetBufferPointer(), desc.ps_bytecode->GetBufferSize(), nullptr, &desc.ps);
}

void RHIDevice::CreateComputeShader(ShaderProgramDesc& desc) const noexcept {
    GetDxDevice()->CreateComputeShader(desc.cs_bytecode->GetBufferPointer(), desc.cs_bytecode->GetBufferSize(), nullptr, &desc.cs);
}

std::pair<std::unique_ptr<RHIOutput>, std::unique_ptr<RHIDeviceContext>> RHIDevice::CreateOutputAndContextFromWindow(std::unique_ptr<Window> window) noexcept {
    window->Open();

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context{};
    {
        std::vector<AdapterInfo> adapters = _rhi_factory.GetAdaptersByHighPerformancePreference();
        if(adapters.empty()) {
            window.reset();
            ERROR_AND_DIE("RHIDevice: Graphics card not found.")
        }
        DebuggerPrintf((std::string{"Adapter count: "} + std::to_string(adapters.size()) + std::string{'\n'}).c_str());
        OutputAdapterInfo(adapters);
        GetDisplayModes(adapters);
        DeviceInfo device_info = CreateDeviceFromFirstAdapter(adapters);
        _dx_device = device_info.dx_device;
        _dx_highestSupportedFeatureLevel = device_info.highest_supported_feature_level;
        context = device_info.dx_context;
    }

    _dxgi_swapchain = CreateSwapChain(*window);
    _allow_tearing_supported = _rhi_factory.QueryForAllowTearingSupport(*this);
    _rhi_factory.RestrictAltEnterToggle(*this);

    SetupDebuggingInfo();

    return std::make_pair(
    std::make_unique<RHIOutput>(*this, std::move(window)),
    std::make_unique<RHIDeviceContext>(*this, context));
}

DeviceInfo RHIDevice::CreateDeviceFromFirstAdapter(const std::vector<AdapterInfo>& adapters) noexcept {
    GUARANTEE_OR_DIE(!adapters.empty(), "CreateDeviceFromFirstAdapter: adapters argument is empty.");
    DeviceInfo info{};

    unsigned int device_flags = 0U;
#ifdef RENDER_DEBUG
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    device_flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    std::array feature_levels{
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1,
    };

    auto first_adapter_info = std::begin(adapters);
    std::ostringstream ss;
    ss << "Selected Adapter: " << AdapterInfoToGraphicsCardDesc(*first_adapter_info).Description << std::endl;
    DebuggerPrintf(ss.str().c_str());

    const auto& first_adapter = first_adapter_info->adapter;
    const auto has_adapter = first_adapter != nullptr;
    Microsoft::WRL::ComPtr<ID3D11Device> temp_device{};
    const auto hr_device = ::D3D11CreateDevice(has_adapter ? first_adapter.Get() : nullptr, has_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, feature_levels.data(), static_cast<unsigned int>(feature_levels.size()), D3D11_SDK_VERSION, temp_device.GetAddressOf(), &info.highest_supported_feature_level, info.dx_context.GetAddressOf());

    GUARANTEE_OR_DIE(info.highest_supported_feature_level >= D3D_FEATURE_LEVEL_11_0, "Your graphics card does not support at least DirectX 11.0. Please update your drivers or hardware.");

    const auto hr_fail_str = StringUtils::FormatWindowsMessage(hr_device);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_device), hr_fail_str.c_str());

    const auto hr_dxdevice5i = temp_device.As(&info.dx_device);
    const auto hrdx5i_fail_str = StringUtils::FormatWindowsMessage(hr_dxdevice5i);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_dxdevice5i), hrdx5i_fail_str.c_str());

    return info;
}

void RHIDevice::OutputAdapterInfo(const std::vector<AdapterInfo>& adapters) const noexcept {
    const auto section_break_field_width = std::size_t{80u};
    const auto entry_name_field_width = std::size_t{40u};
    const auto entry_field_width = std::size_t{35u};
    std::ostringstream ss;
    ss << "ADAPTERS\n";
    std::size_t monitor_count{0u};
    for(const auto& adapter : adapters) {
        ss << std::right << std::setw(section_break_field_width) << std::setfill('-') << '\n'
           << std::setfill(' ');
        ss << AdapterInfoToGraphicsCardDesc(adapter) << '\n';
        const auto outputs = GetOutputsFromAdapter(adapter);
        monitor_count += outputs.size();
        ss << std::left << std::setw(entry_name_field_width) << "Monitors connected to this adapter: " << std::right << std::setw(entry_field_width) << outputs.size() << '\n';
    }
    ss << std::left << std::setw(entry_name_field_width) << "Total Monitor count: " << std::right << std::setw(entry_field_width) << monitor_count << '\n';
    ss << std::right << std::setw(section_break_field_width) << std::setfill('-') << '\n';
    ss << std::flush;
    DebuggerPrintf(ss.str().c_str());
}

void RHIDevice::GetDisplayModes(const std::vector<AdapterInfo>& adapters) const noexcept {
    for(auto& a : adapters) {
        const auto outputs = GetOutputsFromAdapter(a);
        for(const auto& o : outputs) {
            GetDisplayModeDescriptions(a, o, displayModes);
        }
    }
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> RHIDevice::CreateSwapChain(const Window& window) noexcept {
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
    const auto window_dims = window.GetClientDimensions();
    const auto width = static_cast<unsigned int>(window_dims.x);
    const auto height = static_cast<unsigned int>(window_dims.y);
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    return _rhi_factory.CreateSwapChainForHwnd(*this, window, swap_chain_desc);
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> RHIDevice::RecreateSwapChain(const Window& window) noexcept {
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
    const auto window_dims = window.GetClientDimensions();
    const auto width = static_cast<unsigned int>(window_dims.x);
    const auto height = static_cast<unsigned int>(window_dims.y);
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    return _rhi_factory.CreateSwapChainForHwnd(*this, window, swap_chain_desc);
}

std::vector<OutputInfo> RHIDevice::GetOutputsFromAdapter(const AdapterInfo& a) const noexcept {
    if(!a.adapter) {
        return {};
    }
    std::vector<OutputInfo> outputs{};
    auto i = 0u;
    Microsoft::WRL::ComPtr<IDXGIOutput6> cur_output{};
    while(a.adapter->EnumOutputs(i++, reinterpret_cast<IDXGIOutput**>(cur_output.GetAddressOf())) != DXGI_ERROR_NOT_FOUND) {
        OutputInfo cur_info{};
        cur_output->GetDesc1(&cur_info.desc);
        cur_info.output.Swap(cur_output);
        outputs.push_back(cur_info);
    }
    return outputs;
}

void RHIDevice::GetPrimaryDisplayModeDescriptions(const AdapterInfo& adapter, decltype(displayModes)& descriptions) const noexcept {
    const auto& outputs = GetOutputsFromAdapter(adapter);
    if(outputs.empty()) {
        return;
    }
    GetDisplayModeDescriptions(adapter, outputs.front(), descriptions);
}

void RHIDevice::GetDisplayModeDescriptions(const AdapterInfo& adapter, const OutputInfo& output, decltype(displayModes)& descriptions) const noexcept {
    if(!adapter.adapter) {
        return;
    }
    if(!output.output) {
        return;
    }
    auto display_count = 0u;
    auto display_mode_flags = DXGI_ENUM_MODES_SCALING | DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_STEREO | DXGI_ENUM_MODES_DISABLED_STEREO;

    //Call with nullptr to get display count;
    output.output->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, display_mode_flags, &display_count, nullptr);
    if(display_count == 0) {
        return;
    }

    //Call again to fill array.
    std::vector<DXGI_MODE_DESC1> dxgi_descriptions(static_cast<std::size_t>(display_count), DXGI_MODE_DESC1{});
    output.output->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, display_mode_flags, &display_count, dxgi_descriptions.data());

    for(const auto& dxgi_desc : dxgi_descriptions) {
        DisplayDesc display{};
        display.width = dxgi_desc.Width;
        display.height = dxgi_desc.Height;
        display.refreshRateHz = dxgi_desc.RefreshRate.Numerator / dxgi_desc.RefreshRate.Denominator;
        descriptions.insert(display);
    }
}

DisplayDesc RHIDevice::GetDisplayModeMatchingDimensions(const std::vector<DisplayDesc>& descriptions, unsigned int w, unsigned int h) noexcept {
    for(const auto& desc : descriptions) {
        if(desc.width == w && desc.height == h) {
            return desc;
        }
    }
    return {};
}

void RHIDevice::SetupDebuggingInfo([[maybe_unused]] bool breakOnWarningSeverityOrLower /*= true*/) noexcept {
#ifdef RENDER_DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> _dx_debug{};
    if(SUCCEEDED(_dx_device.As(&_dx_debug))) {
        Microsoft::WRL::ComPtr<ID3D11InfoQueue> _dx_infoqueue{};
        if(SUCCEEDED(_dx_debug.As(&_dx_infoqueue))) {
            _dx_infoqueue->SetMuteDebugOutput(false);
            _dx_infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            _dx_infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_ERROR, true);
            _dx_infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_WARNING, breakOnWarningSeverityOrLower);
            _dx_infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_INFO, breakOnWarningSeverityOrLower);
            _dx_infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_MESSAGE, breakOnWarningSeverityOrLower);
            std::vector<D3D11_MESSAGE_ID> hidden = {
            D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
            };
            D3D11_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = static_cast<unsigned int>(hidden.size());
            filter.DenyList.pIDList = hidden.data();
            _dx_infoqueue->AddStorageFilterEntries(&filter);
        }
    }
#endif
}

void RHIDevice::HandleDeviceLost() const noexcept {
    /* DO NOTHING */
}

std::vector<std::unique_ptr<ConstantBuffer>> RHIDevice::CreateConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept {
    auto vs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetVSByteCode());
    auto hs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetHSByteCode());
    auto ds_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetDSByteCode());
    auto gs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetGSByteCode());
    auto ps_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetPSByteCode());
    const auto sizes = std::vector<std::size_t>{
    vs_cbuffers.size(),
    hs_cbuffers.size(),
    ds_cbuffers.size(),
    gs_cbuffers.size(),
    ps_cbuffers.size()};
    auto cbuffer_count = std::accumulate(std::begin(sizes), std::end(sizes), static_cast<std::size_t>(0u));
    if(!cbuffer_count) {
        return {};
    }
    auto&& cbuffers = std::move(vs_cbuffers);
    std::move(std::begin(hs_cbuffers), std::end(hs_cbuffers), std::back_inserter(cbuffers));
    std::move(std::begin(ds_cbuffers), std::end(ds_cbuffers), std::back_inserter(cbuffers));
    std::move(std::begin(gs_cbuffers), std::end(gs_cbuffers), std::back_inserter(cbuffers));
    std::move(std::begin(ps_cbuffers), std::end(ps_cbuffers), std::back_inserter(cbuffers));
    cbuffers.shrink_to_fit();
    return cbuffers;
}

std::vector<std::unique_ptr<ConstantBuffer>> RHIDevice::CreateComputeConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept {
    auto&& cs_cbuffers = std::move(RHIDevice::CreateConstantBuffersFromByteCode(device, shaderProgram->GetCSByteCode()));
    const auto sizes = std::vector<std::size_t>{cs_cbuffers.size()};
    auto cbuffer_count = std::accumulate(std::begin(sizes), std::end(sizes), static_cast<std::size_t>(0u));
    if(!cbuffer_count) {
        return {};
    }
    auto&& ccbuffers = std::move(cs_cbuffers);
    ccbuffers.shrink_to_fit();
    return ccbuffers;
}

std::vector<std::unique_ptr<ConstantBuffer>> RHIDevice::CreateConstantBuffersFromByteCode(RHIDevice& device, ID3DBlob* bytecode) noexcept {
    if(!bytecode) {
        return {};
    }
    ID3D11ShaderReflection* cbufferReflection{};
    if(FAILED(::D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&cbufferReflection))) {
        return {};
    }
    return CreateConstantBuffersUsingReflection(device, *cbufferReflection);
}

void RHIDevice::ResetSwapChainForHWnd() const noexcept {
    const auto hr = _dxgi_swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, _rhi_factory.QueryForAllowTearingSupport(*this) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), StringUtils::FormatWindowsMessage(hr).c_str());
}

std::vector<std::unique_ptr<ConstantBuffer>> RHIDevice::CreateConstantBuffersUsingReflection(RHIDevice& device, ID3D11ShaderReflection& cbufferReflection) noexcept {
    D3D11_SHADER_DESC shader_desc{};
    if(FAILED(cbufferReflection.GetDesc(&shader_desc))) {
        return {};
    }
    if(!shader_desc.ConstantBuffers) {
        return {};
    }

    std::vector<std::unique_ptr<ConstantBuffer>> result{};
    result.reserve(shader_desc.ConstantBuffers);
    const auto& rs = ServiceLocator::get<IRendererService>();
    for(auto resource_idx = 0u; resource_idx < shader_desc.BoundResources; ++resource_idx) {
        D3D11_SHADER_INPUT_BIND_DESC input_desc{};
        if(FAILED(cbufferReflection.GetResourceBindingDesc(resource_idx, &input_desc))) {
            continue;
        }
        if(input_desc.Type != D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER) {
            continue;
        }
        if(input_desc.BindPoint < rs.GetConstantBufferStartIndex()) {
            continue;
        }
        for(auto cbuffer_idx = 0u; cbuffer_idx < shader_desc.ConstantBuffers; ++cbuffer_idx) {
            auto cbuffer_size = std::size_t{0u};
            if(auto* reflected_cbuffer = cbufferReflection.GetConstantBufferByIndex(cbuffer_idx)) {
                D3D11_SHADER_BUFFER_DESC buffer_desc{};
                if(FAILED(reflected_cbuffer->GetDesc(&buffer_desc))) {
                    continue;
                }
                if(buffer_desc.Type != D3D_CBUFFER_TYPE::D3D11_CT_CBUFFER) {
                    continue;
                }
                {
                    std::string buffer_name{buffer_desc.Name ? buffer_desc.Name : ""};
                    std::string input_name{input_desc.Name ? input_desc.Name : ""};
                    if(buffer_name != input_name) {
                        continue;
                    }
                }
                std::vector<std::size_t> var_offsets{};
                for(auto variable_idx = 0u; variable_idx < buffer_desc.Variables; ++variable_idx) {
                    if(auto* reflected_variable = reflected_cbuffer->GetVariableByIndex(variable_idx)) {
                        D3D11_SHADER_VARIABLE_DESC variable_desc{};
                        if(FAILED(reflected_variable->GetDesc(&variable_desc))) {
                            continue;
                        }
                        std::size_t variable_size = variable_desc.Size;
                        std::size_t offset = variable_desc.StartOffset;
                        if(auto* shader_reflection_type = reflected_variable->GetType()) {
                            D3D11_SHADER_TYPE_DESC type_desc{};
                            if(FAILED(shader_reflection_type->GetDesc(&type_desc))) {
                                continue;
                            }
                            cbuffer_size += variable_size;
                            var_offsets.push_back(offset);
                        }
                    }
                }
            }
            std::vector<std::byte> cbuffer_memory(cbuffer_size, std::byte{});
            result.push_back(device.CreateConstantBuffer(cbuffer_memory.data(), cbuffer_memory.size(), BufferUsage::Dynamic, BufferBindUsage::Constant_Buffer));
        }
    }
    return result;
}

std::unique_ptr<InputLayout> RHIDevice::CreateInputLayoutFromByteCode(RHIDevice& device, ID3DBlob* bytecode) noexcept {
    ID3D11ShaderReflection* vertexReflection = nullptr;
    if(FAILED(::D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&vertexReflection))) {
        return nullptr;
    }
    auto il = std::make_unique<InputLayout>();
    il->PopulateInputLayoutUsingReflection(*vertexReflection);
    RHIDevice::CreateInputLayout(*il, device, bytecode->GetBufferPointer(), bytecode->GetBufferSize());
    return il;
}

std::unique_ptr<InputLayoutInstanced> RHIDevice::CreateInputLayoutInstancedFromByteCode(RHIDevice& device, ID3DBlob* vs_bytecode) noexcept {
    ID3D11ShaderReflection* vertexReflection = nullptr;
    if(FAILED(::D3DReflect(vs_bytecode->GetBufferPointer(), vs_bytecode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&vertexReflection))) {
        return nullptr;
    }
    auto il = std::make_unique<InputLayoutInstanced>(device);
    il->PopulateInputLayoutUsingReflection(*vertexReflection);
    il->CreateInputLayout(vs_bytecode->GetBufferPointer(), vs_bytecode->GetBufferSize());
    return il;
}

std::unique_ptr<ShaderProgram> RHIDevice::CreateShaderProgramFromCsoBinaryBuffer(RHIDevice& device, std::vector<uint8_t>& compiledShader, const std::string& name, const PipelineStage& target) noexcept {
    const auto uses_vs_stage = static_cast<unsigned char>(target & PipelineStage::Vs) != 0;
    const auto uses_hs_stage = static_cast<unsigned char>(target & PipelineStage::Hs) != 0;
    const auto uses_ds_stage = static_cast<unsigned char>(target & PipelineStage::Ds) != 0;
    const auto uses_gs_stage = static_cast<unsigned char>(target & PipelineStage::Gs) != 0;
    const auto uses_ps_stage = static_cast<unsigned char>(target & PipelineStage::Ps) != 0;
    const auto uses_cs_stage = static_cast<unsigned char>(target & PipelineStage::Cs) != 0;

    ShaderProgramDesc desc{};
    desc.name = name;
    if(uses_vs_stage) {
        ID3D11VertexShader* vs = nullptr;
        auto hr = device._dx_device->CreateVertexShader(compiledShader.data(), compiledShader.size(), nullptr, &vs);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && vs, error_msg.c_str());
        }
        auto inputLayout = CreateInputLayoutFromByteCode(device, reinterpret_cast<ID3DBlob*>(compiledShader.data()));
        desc.vs = vs;
        desc.vs_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
        desc.input_layout = std::move(inputLayout);
    }

    if(uses_ps_stage) {
        ID3D11PixelShader* ps = nullptr;
        auto hr = device._dx_device->CreatePixelShader(compiledShader.data(), compiledShader.size(), nullptr, &ps);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && ps, error_msg.c_str());
        }
        desc.ps = ps;
        desc.ps_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
    }

    if(uses_hs_stage) {
        ID3D11HullShader* hs = nullptr;
        auto hr = device._dx_device->CreateHullShader(compiledShader.data(), compiledShader.size(), nullptr, &hs);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && hs, error_msg.c_str());
        }
        desc.hs = hs;
        desc.hs_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
    }

    if(uses_ds_stage) {
        ID3D11DomainShader* ds = nullptr;
        auto hr = device._dx_device->CreateDomainShader(compiledShader.data(), compiledShader.size(), nullptr, &ds);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && ds, error_msg.c_str());
        }
        desc.ds = ds;
        desc.ds_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
    }

    if(uses_gs_stage) {
        ID3D11GeometryShader* gs = nullptr;
        auto hr = device._dx_device->CreateGeometryShader(compiledShader.data(), compiledShader.size(), nullptr, &gs);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && gs, error_msg.c_str());
        }
        desc.gs = gs;
        desc.gs_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
    }

    if(uses_cs_stage) {
        ID3D11ComputeShader* cs = nullptr;
        auto hr = device._dx_device->CreateComputeShader(compiledShader.data(), compiledShader.size(), nullptr, &cs);
        {
            const auto error_msg = StringUtils::FormatWindowsMessage(hr);
            GUARANTEE_OR_DIE(SUCCEEDED(hr) && cs, error_msg.c_str());
        }
        desc.cs = cs;
        desc.cs_bytecode = reinterpret_cast<ID3DBlob*>(compiledShader.data());
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> RHIDevice::CreateShaderProgramFromCsoFile(RHIDevice& device, std::filesystem::path filepath, const PipelineStage& target) noexcept {
    if(auto compiled_source = FileUtils::ReadBinaryBufferFromFile(filepath); compiled_source.has_value()) {
        auto sp = CreateShaderProgramFromCsoBinaryBuffer(device, *compiled_source, filepath.string(), target);
        GUARANTEE_OR_DIE(sp, "Unrecoverable error. Cannot continue with malformed shader file.");
        return sp;
    }
    return nullptr;
}

ID3DBlob* RHIDevice::CompileShader(const std::string& name, const void* sourceCode, std::size_t sourceCodeSize, const std::string& entryPoint, const PipelineStage& target) noexcept {
    unsigned int compile_options = 0;
#ifdef RENDER_DEBUG
    compile_options |= D3DCOMPILE_DEBUG;
    compile_options |= D3DCOMPILE_SKIP_OPTIMIZATION;
    compile_options |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    compile_options |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
#ifdef FINAL_BUILD
    compile_options |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    compile_options |= D3DCOMPILE_SKIP_VALIDATION;
    compile_options |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    ID3DBlob* code_blob = nullptr;
    ID3DBlob* errors = nullptr;
    std::string target_string = {};
    switch(target) {
    case PipelineStage::Vs:
        target_string = "vs_5_0";
        break;
    case PipelineStage::Hs:
        target_string = "hs_5_0";
        break;
    case PipelineStage::Ds:
        target_string = "ds_5_0";
        break;
    case PipelineStage::Gs:
        target_string = "gs_5_0";
        break;
    case PipelineStage::Ps:
        target_string = "ps_5_0";
        break;
    case PipelineStage::Cs:
        target_string = "cs_5_0";
        break;
    case PipelineStage::None:
    case PipelineStage::All:
    default:
        DebuggerPrintf("Failed to compile [%s]. Invalid PipelineStage parameter.\n", name.c_str());
        return nullptr;
    }
    HRESULT compile_hr = ::D3DCompile(sourceCode, sourceCodeSize, name.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), target_string.c_str(), compile_options, 0, &code_blob, &errors);
    if(FAILED(compile_hr) || (errors != nullptr)) {
        if(errors != nullptr) {
            char* error_string = static_cast<char*>(errors->GetBufferPointer());
            DebuggerPrintf("Failed to compile [%s].  Compiler gave the following output;\n%s",
                           name.c_str(),
                           error_string);
            errors->Release();
            errors = nullptr;
        }
    }
    return code_blob;
}
