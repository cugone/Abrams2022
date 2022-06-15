#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/Obj.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/Frustum.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Polygon2.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Profiling/ProfileLogScope.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"
#include "Engine/RHI/RHIInstance.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Camera3D.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DepthStencilState.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"
#include "Engine/Renderer/InputLayout.hpp"
#include "Engine/Renderer/InputLayoutInstanced.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Mesh.hpp"
#include "Engine/Renderer/RasterState.hpp"
#include "Engine/Renderer/RenderTargetStack.hpp"
#include "Engine/Renderer/Sampler.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/StructuredBuffer.hpp"
#include "Engine/Renderer/Texture1D.hpp"
#include "Engine/Renderer/Texture2D.hpp"
#include "Engine/Renderer/Texture3D.hpp"
#include "Engine/Renderer/TextureArray2D.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IConfigService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <Thirdparty/TinyXML2/tinyxml2.h>
#include <Thirdparty/stb/stb_image.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ostream>
#include <sstream>
#include <tuple>

ComputeJob::ComputeJob(std::size_t uavCount,
                       const std::vector<Texture*>& uavTextures,
                       Shader* computeShader,
                       unsigned int threadGroupCountX,
                       unsigned int threadGroupCountY,
                       unsigned int threadGroupCountZ) noexcept
: uavCount(uavCount)
, uavTextures{uavTextures}
, computeShader(computeShader)
, threadGroupCountX(threadGroupCountX)
, threadGroupCountY(threadGroupCountY)
, threadGroupCountZ(threadGroupCountZ) {
    /* DO NOTHING */
}

ComputeJob::~ComputeJob() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService, NullRendererService>();
    auto* dc = renderer->GetDeviceContext();
    dc->UnbindAllComputeConstantBuffers();
    dc->UnbindComputeShaderResources();
    dc->UnbindAllComputeUAVs();
    renderer->SetComputeShader(nullptr);
}

Renderer::Renderer() noexcept {
    namespace FS = std::filesystem;
    auto* config = ServiceLocator::get<IConfigService, NullConfigService>();
    if(FS::path path{"Engine/Config/options.config"}; FS::exists(path)) {
        if(!config->AppendFromFile(path)) {
            DebuggerPrintf(std::format("Could not load existing configuration from \"{}\"\n", path.string()));
        }
    }
    m_current_outputMode = [this, &config]() -> RHIOutputMode {
        auto windowed = true;
        if(config->HasKey("windowed")) {
            config->GetValue("windowed", windowed);
        }
        config->SetValue("windowed", windowed);
        if(windowed) {
            return RHIOutputMode::Windowed;
        } else {
            return RHIOutputMode::Borderless_Fullscreen;
        }
    }(); //IIIL
    m_window_dimensions = [this, &config]() -> IntVector2 {
        const auto width = [this, &config]() -> int {
            auto value = 0;
            if(config->HasKey("width")) {
                config->GetValue("width", value);
            }
            if(value <= 0) {
                value = 1600;
            }
            return value;
        }(); //IIIL
        const auto height = [this, &config]() -> int {
            auto value = 0;
            if(config->HasKey("height")) {
                config->GetValue("height", value);
            }
            if(value <= 0) {
                value = 900;
            }
            return value;
        }(); //IIIL
        config->SetValue("width", width);
        config->SetValue("height", height);
        return IntVector2{width, height};
    }(); //IIIL
    if(FS::path path{"Engine/Config/options.config"}; !FS::exists(path)) {
        if(!config->SaveToFile(path)) {
            DebuggerPrintf(std::format("Could not save configuration to \"{}\"\n", path.string()));
        }
    }
}

Renderer::~Renderer() noexcept {
    UnbindAllConstantBuffers();
    UnbindComputeConstantBuffers();
    UnbindAllShaderResources();
    UnbindComputeShaderResources();

    m_temp_vbo.reset();
    m_temp_vbio.reset();
    m_circle_vbo.reset();
    m_temp_ibo.reset();

    m_matrix_cb.reset();
    m_time_cb.reset();
    m_lighting_cb.reset();

    m_textures.clear();
    m_textures.shrink_to_fit();
    m_shader_programs.clear();
    m_shader_programs.shrink_to_fit();
    m_materials.clear();
    m_materials.shrink_to_fit();
    m_shaders.clear();
    m_shaders.shrink_to_fit();
    m_samplers.clear();
    m_samplers.shrink_to_fit();
    m_rasters.clear();
    m_rasters.shrink_to_fit();
    m_fonts.clear();
    m_fonts.shrink_to_fit();
    m_depthstencils.clear();
    m_depthstencils.shrink_to_fit();

    m_default_depthstencil = nullptr;
    m_current_target = nullptr;
    m_current_depthstencil = nullptr;
    m_current_depthstencil_state = nullptr;
    m_current_raster_state = nullptr;
    m_current_sampler = nullptr;
    m_current_material = nullptr;

    m_rhi_output.reset();
    m_rhi_context.reset();
    m_rhi_device.reset();
    RHIInstance::DestroyInstance();
    m_rhi_instance = nullptr;
}

bool Renderer::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    switch(msg.wmMessageCode) {
    case WindowsSystemMessage::Menu_SysCommand: {
        WPARAM wp = msg.wparam;
        switch(wp) {
        case SC_CLOSE: {
            return false; //App needs to respond
        }
        case SC_CONTEXTHELP: break;
        case SC_DEFAULT: break;
        case SC_HOTKEY: break;
        case SC_HSCROLL: break;
        case SCF_ISSECURE: break;
        case SC_KEYMENU: break;
        case SC_MAXIMIZE: {
            return false; //App needs to respond
        }
        case SC_MINIMIZE: {
            m_is_minimized = true;
            return false; //App needs to respond
        }
        case SC_MONITORPOWER: break;
        case SC_MOUSEMENU: break;
        case SC_MOVE: break;
        case SC_NEXTWINDOW: break;
        case SC_PREVWINDOW: break;
        case SC_RESTORE: {
            if(m_is_minimized) {
                m_is_minimized = false;
            }
            return false; //UI needs to respond
        }
        case SC_SCREENSAVE: {
            return true; // Disable screen saver from activating
        }
        case SC_SIZE: {
            return false; //App needs to respond
        }
        case SC_TASKLIST: break;
        case SC_VSCROLL: break;
        default: break;
        }
        return false;
    }
    case WindowsSystemMessage::Window_ActivateApp: {
        WPARAM wp = msg.wparam;
        bool losing_focus = wp == FALSE;
        bool gaining_focus = wp == TRUE;
        if(losing_focus) {
        }
        if(gaining_focus) {
        }
        return false;
    }
    case WindowsSystemMessage::Keyboard_Activate: {
        WPARAM wp = msg.wparam;
        auto active_type = LOWORD(wp);
        switch(active_type) {
        case WA_ACTIVE:      /* FALLTHROUGH */
        case WA_CLICKACTIVE: //Gained Focus
            return false;    //App needs to respond
        case WA_INACTIVE:    //Lost focus
            return false;    //App needs to respond
        default:
            return false; //App needs to respond
        }
    }
    case WindowsSystemMessage::Window_EnterSizeMove: {
        m_enteredSizeMove = true;
        return false; //UI needs to respond
    }
    case WindowsSystemMessage::Window_ExitSizeMove: {
        if(m_enteredSizeMove) {
            m_doneSizeMove = true;
            return false;
        }
        return false;
    }
    case WindowsSystemMessage::Window_Size: {
        LPARAM lp = msg.lparam;
        m_is_minimized = false;
        const auto resize_type = EngineSubsystem::GetResizeTypeFromWmSize(msg);
        if(auto* window = GetOutput()->GetWindow(); window != nullptr) {
            switch(resize_type) {
            case WindowResizeType::Maximized: {
                window->SetDisplayMode(RHIOutputMode::Borderless_Fullscreen);
                break;
            }
            case WindowResizeType::Restored: {
                if(const auto prev_displaymode = window->GetDisplayMode(); prev_displaymode == RHIOutputMode::Borderless_Fullscreen) {
                    const auto w = LOWORD(lp);
                    const auto h = HIWORD(lp);
                    const auto new_size = IntVector2{w, h};
                    const auto new_position = IntVector2{GetScreenCenter()} - new_size / 2;
                    window->SetDisplayMode(RHIOutputMode::Windowed);
                } else {
                    if(m_enteredSizeMove && m_doneSizeMove) {
                        m_enteredSizeMove = false;
                        m_doneSizeMove = false;
                        const auto w = LOWORD(lp);
                        const auto h = HIWORD(lp);
                        const auto new_size = IntVector2{w, h};
                        window->SetDimensions(new_size);
                    }
                }
                break;
            }
            case WindowResizeType::Minimized: {
                m_is_minimized = true;
                return false; //App must be able to respond.
            }
            }
            ResizeBuffers();
        }
        return false; //App must be able to respond.
    }
    }
    return false;
}

void Renderer::Initialize() noexcept {
    m_rhi_instance = RHIInstance::CreateInstance();
    m_rhi_device = m_rhi_instance->CreateDevice();

    WindowDesc windowDesc{};
    auto* config = ServiceLocator::get<IConfigService, NullConfigService>();
    if(config->HasKey("windowed")) {
        auto windowed = windowDesc.mode == RHIOutputMode::Windowed;
        config->GetValue("windowed", windowed);
        windowDesc.mode = windowed ? RHIOutputMode::Windowed : RHIOutputMode::Borderless_Fullscreen;
    }
    if(config->HasKey("width")) {
        auto& width = windowDesc.dimensions.x;
        config->GetValue("width", width);
    }
    if(config->HasKey("height")) {
        auto& height = windowDesc.dimensions.y;
        config->GetValue("height", height);
    }
    std::tie(m_rhi_output, m_rhi_context) = m_rhi_device->CreateOutputAndContext(windowDesc);
    
    GetOutput()->GetWindow()->SetDisplayMode(windowDesc.mode);

    LogAvailableDisplays();
    CreateWorkingVboAndIbo();
    CreateDefaultConstantBuffers();

    CreateAndRegisterDefaultDepthStencilStates();
    CreateAndRegisterDefaultSamplers();
    CreateAndRegisterDefaultRasterStates();
    CreateAndRegisterDefaultTextures();
    CreateAndRegisterDefaultShaderPrograms();
    CreateAndRegisterDefaultShaders();
    CreateAndRegisterDefaultMaterials();
    CreateAndRegisterDefaultFonts();

    SetDepthStencilState(GetDepthStencilState("__default"));
    SetRasterState(GetRasterState("__solid"));
    const auto dims = GetOutput()->GetDimensions();
    SetScissorAndViewport(0.0f, 0.0f, static_cast<float>(dims.x), static_cast<float>(dims.y));
    SetSampler(GetSampler("__default"));
    SetRenderTarget(m_current_target, m_current_depthstencil);
    m_current_material = nullptr; //User must explicitly set to avoid defaulting to full lighting material.
}

void Renderer::CreateDefaultConstantBuffers() noexcept {
    m_matrix_cb = CreateConstantBuffer(&m_matrix_data, sizeof(m_matrix_data));
    m_time_cb = CreateConstantBuffer(&m_time_data, sizeof(m_time_data));
    m_lighting_cb = CreateConstantBuffer(&m_lighting_data, sizeof(m_lighting_data));
}

void Renderer::CreateWorkingVboAndIbo() noexcept {
    VertexBuffer::buffer_t default_vbo(1024);
    IndexBuffer::buffer_t default_ibo(1024);
    VertexCircleBuffer::buffer_t default_circle_vbo(1024);

    m_temp_vbo = CreateVertexBuffer(default_vbo);
    m_temp_ibo = CreateIndexBuffer(default_ibo);
    m_circle_vbo = CreateVertexCircleBuffer(default_circle_vbo);

    m_current_vbo_size = default_vbo.size();
    m_current_vbco_size = default_circle_vbo.size();
    m_current_ibo_size = default_ibo.size();
}

void Renderer::LogAvailableDisplays() noexcept {
    std::ostringstream ss;
    ss << std::format("{:->80}", '\n');
    ss << "Available Display Dimensions:\n";
    ss << std::format("{:->80}", '\n');
    ss << std::format("{:<13}{:^10}{:>10}", '|', "Resolution", '|');
    ss << std::format("{:>26}{:>20}", "Refresh Rage (Hz)", "|\n");
    ss << std::format("{:->80}", '\n');
    auto refreshRateHzStr = std::string{};
    for(auto it = std::begin(m_rhi_device->displayModes); it != std::end(m_rhi_device->displayModes); ) {
        const auto& display = *it;
        const auto& next_it = std::next(it, 1);
        if(next_it == std::end(m_rhi_device->displayModes)) {
            if(std::distance(it, next_it) == 1) {
                refreshRateHzStr += std::format("{}", display.refreshRateHz);
            }
            const auto resolutionStr = std::format("{}x{}", display.width, display.height);
            ss << std::format("{:<13}{:>10}", '|', resolutionStr);
            ss << std::format("{:>10}{:>40}{:>6}", '|', refreshRateHzStr, "|\n");
            refreshRateHzStr.clear();
            break;
        }
        const auto& next_display = *next_it;
        if(display.width == next_display.width && display.height == next_display.height) {
            refreshRateHzStr += std::format("{} ", display.refreshRateHz);
        } else {
            if(std::distance(it, next_it) == 1) {
                refreshRateHzStr += std::format("{}", display.refreshRateHz);
            }
            if(!refreshRateHzStr.empty() && refreshRateHzStr.back() == ' ') {
                refreshRateHzStr.pop_back();
            }
            const auto resolutionStr = std::format("{}x{}", display.width, display.height);
            ss << std::format("{:<13}{:>10}", '|', resolutionStr);
            ss << std::format("{0:>10}{1:>40}{2:>6}", '|', refreshRateHzStr, "|\n");
            refreshRateHzStr.clear();
        }
        ++it;
    }
    ss << std::format("{:->80}", '\n');
    DebuggerPrintf(ss.str());
    ServiceLocator::get<IFileLoggerService, NullFileLoggerService>()->LogLineAndFlush(ss.str());
}

Vector2 Renderer::GetScreenCenter() const noexcept {
    RECT desktopRect;
    HWND desktopWindowHandle = ::GetDesktopWindow();
    if(::GetClientRect(desktopWindowHandle, &desktopRect)) {
        float center_x = desktopRect.left + (desktopRect.right - desktopRect.left) * 0.5f;
        float center_y = desktopRect.top + (desktopRect.bottom - desktopRect.top) * 0.5f;
        return Vector2{center_x, center_y};
    }
    return Vector2::Zero;
}

Vector2 Renderer::GetWindowCenter() const noexcept {
    const auto& window = *GetOutput()->GetWindow();
    return GetWindowCenter(window);
}

Vector2 Renderer::GetWindowCenter(const Window& window) const noexcept {
    RECT rect;
    HWND windowHandle = static_cast<HWND>(window.GetWindowHandle());
    if(::GetClientRect(windowHandle, &rect)) {
        float center_x = rect.left + (rect.right - rect.left) * 0.50f;
        float center_y = rect.top + (rect.bottom - rect.top) * 0.50f;
        return Vector2{center_x, center_y};
    }
    return Vector2::Zero;
}

void Renderer::UnbindWorkingVboAndIbo() noexcept {
    //Setting the current sizes to zero forces them to be recreated next time they are updated.
    m_current_ibo_size = 0;
    m_current_vbo_size = 0;
}

void Renderer::SetDepthComparison(ComparisonFunction cf) noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.DepthFunc = ComparisonFunctionToD3DComparisonFunction(cf);
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

ComparisonFunction Renderer::GetDepthComparison() const noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    return D3DComparisonFunctionToComparisonFunction(desc.DepthFunc);
}

void Renderer::SetStencilFrontComparison(ComparisonFunction cf) noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.FrontFace.StencilFunc = ComparisonFunctionToD3DComparisonFunction(cf);
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::SetStencilBackComparison(ComparisonFunction cf) noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.BackFace.StencilFunc = ComparisonFunctionToD3DComparisonFunction(cf);
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::EnableStencilWrite() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.StencilEnable = true;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::DisableStencilWrite() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.StencilEnable = false;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::BeginFrame() noexcept {
    UnbindAllShaderResources();
}

void Renderer::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    UpdateSystemTime(deltaSeconds);
}

void Renderer::UpdateGameTime(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_time_data.game_time += deltaSeconds.count();
    m_time_data.game_frame_time = deltaSeconds.count();
    m_time_cb->Update(*m_rhi_context, &m_time_data);
    SetConstantBuffer(GetTimeBufferIndex(), m_time_cb.get());
}

void Renderer::UpdateSystemTime(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_time_data.system_time += deltaSeconds.count();
    m_time_data.system_frame_time = deltaSeconds.count();
    m_time_cb->Update(*m_rhi_context, &m_time_data);
    SetConstantBuffer(GetTimeBufferIndex(), m_time_cb.get());
}

void Renderer::UpdateConstantBuffer(ConstantBuffer& buffer, void* const& data) noexcept {
    buffer.Update(*m_rhi_context, data);
}

void Renderer::Render() const noexcept {
    /* DO NOTHING */
}

void Renderer::EndFrame() noexcept {
    Present();
    FulfillScreenshotRequest();
}

void Renderer::BeginRender(Texture* color_target /*= nullptr*/, const Rgba& clear_color /*= Rgba::Black*/, Texture* depthstencil_target /*= nullptr*/) noexcept {
    ResetModelViewProjection();
    SetRenderTarget(color_target, depthstencil_target);
    ClearColor(clear_color);
    ClearDepthStencilBuffer();
    
    SetViewportAsPercent();
}

void Renderer::BeginRenderToBackbuffer(const Rgba& clear_color /*= Rgba::Black*/) noexcept {
    BeginRender(nullptr, clear_color);
}

void Renderer::BeginHUDRender(Camera2D& ui_camera, const Vector2& camera_position, float window_height) noexcept {
    const float ui_view_height = window_height;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = Vector2{0.0f, 1.0f};
    ui_camera.SetPosition(camera_position);
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    SetCamera(ui_camera);
}

TimeUtils::FPSeconds Renderer::GetGameFrameTime() const noexcept {
    return TimeUtils::FPSeconds{m_time_data.game_frame_time};
}

TimeUtils::FPSeconds Renderer::GetSystemFrameTime() const noexcept {
    return TimeUtils::FPSeconds{m_time_data.system_frame_time};
}

TimeUtils::FPSeconds Renderer::GetGameTime() const noexcept {
    return TimeUtils::FPSeconds{m_time_data.game_time};
}

TimeUtils::FPSeconds Renderer::GetSystemTime() const noexcept {
    return TimeUtils::FPSeconds{m_time_data.system_time};
}

std::unique_ptr<ConstantBuffer> Renderer::CreateConstantBuffer(void* const& buffer, const std::size_t& buffer_size) const noexcept {
    return m_rhi_device->CreateConstantBuffer(buffer, buffer_size, BufferUsage::Dynamic, BufferBindUsage::Constant_Buffer);
}

std::unique_ptr<VertexBuffer> Renderer::CreateVertexBuffer(const VertexBuffer::buffer_t& vbo) const noexcept {
    return m_rhi_device->CreateVertexBuffer<VertexBuffer>(vbo, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer);
}

std::unique_ptr<VertexCircleBuffer> Renderer::CreateVertexCircleBuffer(const VertexCircleBuffer::buffer_t& vbco) const noexcept {
    return m_rhi_device->CreateVertexBuffer<VertexCircleBuffer>(vbco, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer);
}

std::unique_ptr<VertexBufferInstanced> Renderer::CreateVertexBufferInstanced(const VertexBufferInstanced::buffer_t& vbio) const noexcept {
    return m_rhi_device->CreateVertexBufferInstanced(vbio, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer);
}

std::unique_ptr<IndexBuffer> Renderer::CreateIndexBuffer(const IndexBuffer::buffer_t& ibo) const noexcept {
    return m_rhi_device->CreateIndexBuffer(ibo, BufferUsage::Dynamic, BufferBindUsage::Index_Buffer);
}

std::unique_ptr<StructuredBuffer> Renderer::CreateStructuredBuffer(const StructuredBuffer::buffer_t& sbo, std::size_t element_size, std::size_t element_count) const noexcept {
    return m_rhi_device->CreateStructuredBuffer(sbo, element_size, element_count, BufferUsage::Static, BufferBindUsage::Shader_Resource);
}

bool Renderer::RegisterTexture(const std::string& name, std::unique_ptr<Texture> texture) noexcept {
    namespace FS = std::filesystem;
    FS::path p(name);
    if(!StringUtils::StartsWith(p.string(), "__")) {
        std::error_code ec{};
        p = FS::canonical(p, ec);
        if(ec) {
            std::cout << ec.message();
            return false;
        }
    }
    p.make_preferred();
    auto found_texture = std::find_if(std::cbegin(m_textures), std::cend(m_textures), [&p](const auto& t) { return t.first == p.string(); });
    if(found_texture == m_textures.end()) {
        m_textures.emplace_back(std::make_pair(name, std::move(texture)));
        return true;
    } else {
        return false;
    }
}

Texture* Renderer::GetTexture(const std::string& nameOrFile) noexcept {
    namespace FS = std::filesystem;
    FS::path p(nameOrFile);
    if(!StringUtils::StartsWith(p.string(), "__")) {
        p = FS::canonical(p);
    }
    p.make_preferred();
    auto found_texture = std::find_if(std::cbegin(m_textures), std::cend(m_textures), [&p](const auto& t) { return t.first == p.string(); });
    if(found_texture == m_textures.end()) {
        return nullptr;
    }
    return (*found_texture).second.get();
}

void Renderer::DrawPoint(const Vertex3D& point) noexcept {
    std::vector<Vertex3D> vbo = {point};
    std::vector<unsigned int> ibo = {0};
    DrawIndexed(PrimitiveType::Points, vbo, ibo);
}

void Renderer::DrawPoint(const Vector3& point, const Rgba& color /*= Rgba::WHITE*/, const Vector2& tex_coords /*= Vector2::ZERO*/) noexcept {
    DrawPoint(Vertex3D(point, color, tex_coords));
}

void Renderer::DrawFrustum(const Frustum& frustum, const Rgba& color /*= Rgba::YELLOW*/, const Vector2& tex_coords /*= Vector2::ZERO*/) noexcept {
    const Vector3& point1{frustum.GetNearBottomLeft()};
    const Vector3& point2{frustum.GetNearTopLeft()};
    const Vector3& point3{frustum.GetNearTopRight()};
    const Vector3& point4{frustum.GetNearBottomRight()};
    const Vector3& point5{frustum.GetFarBottomLeft()};
    const Vector3& point6{frustum.GetFarTopLeft()};
    const Vector3& point7{frustum.GetFarTopRight()};
    const Vector3& point8{frustum.GetFarBottomRight()};
    std::vector<Vertex3D> vbo{
    Vertex3D{point1, color, tex_coords}, //Near
    Vertex3D{point2, color, tex_coords},
    Vertex3D{point3, color, tex_coords},
    Vertex3D{point4, color, tex_coords},
    Vertex3D{point5, color, tex_coords}, //Far
    Vertex3D{point6, color, tex_coords},
    Vertex3D{point7, color, tex_coords},
    Vertex3D{point8, color, tex_coords},
    };
    std::vector<unsigned int> ibo{
    0, 1, 1, 2, 2, 3, 3, 0, //Near
    4, 5, 5, 6, 6, 7, 7, 4, //Far
    0, 4, 1, 5, 2, 6, 3, 7, //Edges
    };
    DrawIndexed(PrimitiveType::Lines, vbo, ibo);
}

void Renderer::DrawWorldGridXZ(float radius /*= 500.0f*/, float major_gridsize /*= 20.0f*/, float minor_gridsize /*= 5.0f*/, const Rgba& major_color /*= Rgba::WHITE*/, const Rgba& minor_color /*= Rgba::DARK_GRAY*/) noexcept {
    const float half_length = radius;
    const float length = radius * 2.0f;
    const float space_between_majors = std::floor(length * (major_gridsize / length));
    const float space_between_minors = std::floor(length * (minor_gridsize / length));

    Mesh::Builder mesh_builder{};
    //MAJOR LINES
    mesh_builder.Begin(PrimitiveType::Lines);
    mesh_builder.SetColor(major_color);
    for(float x = -half_length; x < half_length + 1.0f; x += space_between_majors) {
        mesh_builder.AddVertex(Vector3(x, 0.0f, -half_length));
        mesh_builder.AddVertex(Vector3(x, 0.0f, half_length));
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }

    for(float z = -half_length; z < half_length + 1.0f; z += space_between_majors) {
        mesh_builder.AddVertex(Vector3(-half_length, 0.0f, z));
        mesh_builder.AddVertex(Vector3(half_length, 0.0f, z));
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }

    const bool major_minor_are_same_size = MathUtils::IsEquivalent(major_gridsize, minor_gridsize);
    const bool has_minors = !major_minor_are_same_size;
    if(has_minors) {
        mesh_builder.SetColor(minor_color);
        //MINOR LINES
        for(float x = -half_length; x < half_length; x += space_between_minors) {
            if(MathUtils::IsEquivalent(std::fmod(x, space_between_majors), 0.0f)) {
                continue;
            }
            mesh_builder.AddVertex(Vector3(x, 0.0f, -half_length));
            mesh_builder.AddVertex(Vector3(x, 0.0f, half_length));
            mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
        }
        for(float z = -half_length; z < half_length; z += space_between_minors) {
            if(MathUtils::IsEquivalent(std::fmod(z, space_between_majors), 0.0f)) {
                continue;
            }
            mesh_builder.AddVertex(Vector3(-half_length, 0.0f, z));
            mesh_builder.AddVertex(Vector3(half_length,  0.0f, z));
            mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
        }
    }
    mesh_builder.End(GetMaterial("__unlit"));

    SetModelMatrix(Matrix4::I);
    Mesh::Render(mesh_builder);
}

void Renderer::DrawWorldGridXY(float radius /*= 500.0f*/, float major_gridsize /*= 20.0f*/, float minor_gridsize /*= 5.0f*/, const Rgba& major_color /*= Rgba::WHITE*/, const Rgba& minor_color /*= Rgba::DARK_GRAY*/) noexcept {
    const float half_length = radius;
    const float length = radius * 2.0f;
    const float space_between_majors = std::floor(length * (major_gridsize / length));
    const float space_between_minors = std::floor(length * (minor_gridsize / length));

    static Mesh::Builder mesh_builder{};
    mesh_builder.Clear();
    //MAJOR LINES
    mesh_builder.Begin(PrimitiveType::Lines);
    mesh_builder.SetColor(major_color);
    for(float x = -half_length; x < half_length + 1.0f; x += space_between_majors) {
        mesh_builder.AddVertex(Vector3(x, -half_length, 0.0f));
        mesh_builder.AddVertex(Vector3(x, half_length, 0.0f));
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }

    for(float y = -half_length; y < half_length + 1.0f; y += space_between_majors) {
        mesh_builder.AddVertex(Vector3(-half_length, y, 0.0f));
        mesh_builder.AddVertex(Vector3(half_length, y, 0.0f));
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }

    const bool major_minor_are_same_size = MathUtils::IsEquivalent(major_gridsize, minor_gridsize);
    const bool has_minors = !major_minor_are_same_size;
    if(has_minors) {
        mesh_builder.SetColor(minor_color);
        //MINOR LINES
        for(float x = -half_length; x < half_length; x += space_between_minors) {
            if(MathUtils::IsEquivalent(std::fmod(x, space_between_majors), 0.0f)) {
                continue;
            }
            mesh_builder.AddVertex(Vector3(x, -half_length, 0.0f));
            mesh_builder.AddVertex(Vector3(x, half_length, 0.0f));
            mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
        }
        for(float y = -half_length; y < half_length; y += space_between_minors) {
            if(MathUtils::IsEquivalent(std::fmod(y, space_between_majors), 0.0f)) {
                continue;
            }
            mesh_builder.AddVertex(Vector3(-half_length, y, 0.0f));
            mesh_builder.AddVertex(Vector3(half_length, y, 0.0f));
            mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
        }
    }
    mesh_builder.End(GetMaterial("__unlit"));

    SetModelMatrix(Matrix4::I);
    Mesh::Render(mesh_builder);
}

void Renderer::DrawWorldGrid2D(int width, int height, const Rgba& color /*= Rgba::White*/) noexcept {
    const auto y_start = 0;
    const auto y_end = height;
    const auto x_start = 0;
    const auto x_end = width;
    const auto y_first = 0;
    const auto y_last = height + 1;
    const auto x_first = 0;
    const auto x_last = width + 1;
    //const auto size = std::size_t{2u} + width + height;

    static Mesh::Builder mesh_builder{};
    mesh_builder.Clear();
    mesh_builder.Begin(PrimitiveType::Lines);
    mesh_builder.SetColor(color);
    for(int x = x_first; x < x_last; ++x) {
        mesh_builder.AddVertex(Vector3{static_cast<float>(x), static_cast<float>(y_start), 0.0f});
        mesh_builder.AddVertex(Vector3{static_cast<float>(x), static_cast<float>(y_end), 0.0f});
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }
    for(int y = y_first; y < y_last; ++y) {
        mesh_builder.AddVertex(Vector3{static_cast<float>(x_start), static_cast<float>(y), 0.0f});
        mesh_builder.AddVertex(Vector3{static_cast<float>(x_end), static_cast<float>(y), 0.0f});
        mesh_builder.AddIndicies(Mesh::Builder::Primitive::Line);
    }
    mesh_builder.End(GetMaterial("__2D"));
    Mesh::Render(mesh_builder);
}

void Renderer::DrawWorldGrid2D(const IntVector2& dimensions, const Rgba& color /*= Rgba::White*/) noexcept {
    DrawWorldGrid2D(dimensions.x, dimensions.y, color);
}

void Renderer::DrawAxes(float maxlength /*= 1000.0f*/, bool disable_unit_depth /*= true*/) noexcept {
    static std::vector<Vertex3D> vbo{
    Vertex3D{Vector3::Zero, Rgba::Red},
    Vertex3D{Vector3::Zero, Rgba::Green},
    Vertex3D{Vector3::Zero, Rgba::Blue},
    Vertex3D{Vector3::X_Axis * maxlength, Rgba::Red},
    Vertex3D{Vector3::Y_Axis * maxlength, Rgba::Green},
    Vertex3D{Vector3::Z_Axis * maxlength, Rgba::Blue},
    Vertex3D{Vector3::X_Axis, Rgba::Red},
    Vertex3D{Vector3::Y_Axis, Rgba::Green},
    Vertex3D{Vector3::Z_Axis, Rgba::Blue},
    };
    static std::vector<unsigned int> ibo{
    0, 3, 1, 4, 2, 5,
    0, 6, 1, 7, 2, 8};
    SetModelMatrix(Matrix4::I);
    SetMaterial(GetMaterial("__unlit"));
    DrawIndexed(PrimitiveType::Lines, vbo, ibo, 6, 0);
    if(disable_unit_depth) {
        DisableDepth();
    }
    DrawIndexed(PrimitiveType::Lines, vbo, ibo, 6, 6);
    if(disable_unit_depth) {
        EnableDepth();
    }
}

void Renderer::DrawDebugSphere(const Rgba& color) noexcept {
    SetMaterial(GetMaterial("__unlit"));

    float centerX = 0.0f;
    float centerY = 0.0f;
    int numSides = 65;
    auto num_sides_as_float = static_cast<float>(numSides);
    std::vector<Vector3> verts;
    verts.reserve(numSides);
    float anglePerVertex = 360.0f / num_sides_as_float;
    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        float radians = MathUtils::ConvertDegreesToRadians(degrees);
        float pX = std::cos(radians) + centerX;
        float pY = std::sin(radians) + centerY;
        verts.emplace_back(Vector2(pX, pY), 0.0f);
    }
    {
        float radians = MathUtils::ConvertDegreesToRadians(360.0f);
        float pX = std::cos(radians) + centerX;
        float pY = std::sin(radians) + centerY;
        verts.emplace_back(Vector2(pX, pY), 0.0f);
    }

    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        float radians = MathUtils::ConvertDegreesToRadians(degrees);
        float pX = std::cos(radians) + centerX;
        float pY = std::sin(radians) + centerY;
        verts.emplace_back(Vector2(pX, 0.0f), pY);
    }
    //{
    //    float radians = MathUtils::ConvertDegreesToRadians(360.0f);
    //    float pX = std::cos(radians) + centerX;
    //    float pY = std::sin(radians) + centerY;
    //    verts.emplace_back(Vector2(pX, 0.0f), pY);
    //}

    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        float radians = MathUtils::ConvertDegreesToRadians(degrees);
        float pX = std::cos(radians) + centerX;
        float pY = std::sin(radians) + centerY;
        verts.emplace_back(Vector2(0.0f, pX), pY);
    }
    //{
    //    float radians = MathUtils::ConvertDegreesToRadians(360.0f);
    //    float pX = std::cos(radians) + centerX;
    //    float pY = std::sin(radians) + centerY;
    //    verts.emplace_back(Vector2(0.0f, pX), pY);
    //}
    std::vector<Vertex3D> vbo;
    vbo.resize(verts.size());
    for(std::size_t i = 0; i < vbo.size(); ++i) {
        vbo[i].position = verts[i];
        const auto&& [r, g, b, a] = color.GetAsFloats();
        vbo[i].color = Vector4{r, g, b, a};
    }

    std::vector<unsigned int> ibo;
    ibo.resize(verts.size() * 2 - 2);
    unsigned int idx = 0;
    for(std::size_t i = 0; i < ibo.size(); i += 2) {
        ibo[i + 0] = idx + 0;
        ibo[i + 1] = idx + 1;
        ++idx;
    }
    //ibo[ibo.size() - 2] = idx;
    //ibo[ibo.size() - 1] = idx + 1;
    DrawIndexed(PrimitiveType::Lines, vbo, ibo);
}

void Renderer::Draw(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo) noexcept {
    UpdateVbo(vbo);
    Draw(topology, m_temp_vbo.get(), vbo.size());
}

void Renderer::Draw(const PrimitiveType& topology, const std::vector<VertexCircle2D>& vbo) noexcept {
    UpdateVbco(vbo);
    Draw(topology, m_circle_vbo.get(), vbo.size());
}

void Renderer::Draw(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, std::size_t vertex_count) noexcept {
    UpdateVbo(vbo);
    Draw(topology, m_temp_vbo.get(), vertex_count);
}

void Renderer::Draw(const PrimitiveType& topology, const std::vector<VertexCircle2D>& vbo, std::size_t vertex_count) noexcept {
    UpdateVbco(vbo);
    Draw(topology, m_circle_vbo.get(), vertex_count);
}

void Renderer::DrawIndexed(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<unsigned int>& ibo) noexcept {
    UpdateVbo(vbo);
    UpdateIbo(ibo);
    DrawIndexed(topology, m_temp_vbo.get(), m_temp_ibo.get(), ibo.size());
}

void Renderer::DrawIndexed(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<unsigned int>& ibo, std::size_t index_count, std::size_t startVertex /*= 0*/, std::size_t baseVertexLocation /*= 0*/) noexcept {
    UpdateVbo(vbo);
    UpdateIbo(ibo);
    DrawIndexed(topology, m_temp_vbo.get(), m_temp_ibo.get(), index_count, startVertex, baseVertexLocation);
}

void Renderer::DrawInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) noexcept {
    DrawInstanced(topology, vbo, vbio, instanceCount, vbo.size());
}

void Renderer::DrawInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount, std::size_t vertexCount) noexcept {
    UpdateVbo(vbo);
    UpdateVbio(vbio);
    DrawInstanced(topology, m_temp_vbo.get(), m_temp_vbio.get(), vertexCount, instanceCount, std::size_t{0u}, std::size_t{0u});
}

void Renderer::DrawIndexedInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, const std::vector<unsigned int>& ibo, std::size_t instanceCount) noexcept {
    DrawIndexedInstanced(topology, vbo, vbio, ibo, instanceCount, 0, 0, 0);
}

void Renderer::DrawIndexedInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, const std::vector<unsigned int>& ibo, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept {
    UpdateVbo(vbo);
    UpdateVbio(vbio);
    UpdateIbo(ibo);
    DrawIndexedInstanced(topology, m_temp_vbo.get(), m_temp_vbio.get(), m_temp_ibo.get(), ibo.size(), instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void Renderer::SetLightingEyePosition(const Vector3& position) noexcept {
    m_lighting_data.eye_position = Vector4(position, 1.0f);
    m_lighting_cb->Update(*m_rhi_context, &m_lighting_data);
    SetConstantBuffer(GetLightingBufferIndex(), m_lighting_cb.get());
}

void Renderer::SetAmbientLight(const Rgba& ambient) noexcept {
    float intensity = ambient.a / 255.0f;
    SetAmbientLight(ambient, intensity);
}

void Renderer::SetAmbientLight(const Rgba& color, float intensity) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    m_lighting_data.ambient = Vector4{r, g, b, intensity};
    m_lighting_cb->Update(*m_rhi_context, &m_lighting_data);
    SetConstantBuffer(GetLightingBufferIndex(), m_lighting_cb.get());
}

void Renderer::SetSpecGlossEmitFactors(Material* mat) noexcept {
    float spec = mat ? mat->GetSpecularIntensity() : 1.0f;
    float gloss = mat ? mat->GetGlossyFactor() : 8.0f;
    float emit = mat ? mat->GetEmissiveFactor() : 0.0f;
    m_lighting_data.specular_glossy_emissive_factors = Vector4(spec, gloss, emit, 1.0f);
    m_lighting_cb->Update(*m_rhi_context, &m_lighting_data);
    SetConstantBuffer(GetLightingBufferIndex(), m_lighting_cb.get());
}

void Renderer::SetUseVertexNormalsForLighting(bool value) noexcept {
    if(value) {
        m_lighting_data.useVertexNormals = 1;
    } else {
        m_lighting_data.useVertexNormals = 0;
    }
    m_lighting_cb->Update(*m_rhi_context, &m_lighting_data);
    SetConstantBuffer(GetLightingBufferIndex(), m_lighting_cb.get());
}

const light_t& Renderer::GetLight(unsigned int index) const noexcept {
    return m_lighting_data.lights[index];
}

void Renderer::SetPointLight(unsigned int index, const PointLightDesc& desc) noexcept {
    auto l = light_t{};
    l.attenuation = Vector4(desc.attenuation, 0.0f);
    l.specAttenuation = l.attenuation;
    l.position = Vector4(desc.position, 1.0f);
    const auto&& [r, g, b, _] = desc.color.GetAsFloats();
    l.color = Vector4{r, g, b, desc.intensity};
    SetPointLight(index, l);
}

void Renderer::SetDirectionalLight(unsigned int index, const DirectionalLightDesc& desc) noexcept {
    auto l = light_t{};
    l.direction = Vector4(desc.direction, 0.0f);
    l.attenuation = Vector4(desc.attenuation, 1.0f);
    l.specAttenuation = l.attenuation;
    const auto&& [r, g, b, _] = desc.color.GetAsFloats();
    l.color = Vector4{r, g, b, desc.intensity};
    SetDirectionalLight(index, l);
}

void Renderer::SetSpotlight(unsigned int index, const SpotLightDesc& desc) noexcept {
    auto l = light_t{};
    l.attenuation = Vector4(desc.attenuation, 0.0f);
    l.specAttenuation = l.attenuation;
    l.position = Vector4(desc.position, 1.0f);
    const auto&& [r, g, b, _] = desc.color.GetAsFloats();
    l.color = Vector4{r, g, b, desc.intensity};
    l.direction = Vector4(desc.direction, 0.0f);

    float inner_degrees = desc.inner_outer_anglesDegrees.x;
    float inner_radians = MathUtils::ConvertDegreesToRadians(inner_degrees);
    float inner_half_angle = inner_radians * 0.5f;
    float inner_dot_threshold = std::cos(inner_half_angle);

    float outer_degrees = desc.inner_outer_anglesDegrees.y;
    float outer_radians = MathUtils::ConvertDegreesToRadians(outer_degrees);
    float outer_half_angle = outer_radians * 0.5f;
    float outer_dot_threshold = std::cos(outer_half_angle);

    l.innerOuterDotThresholds = Vector4(Vector2(inner_dot_threshold, outer_dot_threshold), Vector2::Zero);

    SetSpotlight(index, l);
}

void Renderer::SetLightAtIndex(unsigned int index, const light_t& light) noexcept {
    m_lighting_data.lights[index] = light;
    m_lighting_cb->Update(*m_rhi_context, &m_lighting_data);
    SetConstantBuffer(GetLightingBufferIndex(), m_lighting_cb.get());
}

void Renderer::SetPointLight(unsigned int index, const light_t& light) noexcept {
    SetLightAtIndex(index, light);
}

void Renderer::SetDirectionalLight(unsigned int index, const light_t& light) noexcept {
    SetLightAtIndex(index, light);
}

void Renderer::SetSpotlight(unsigned int index, const light_t& light) noexcept {
    SetLightAtIndex(index, light);
}

std::unique_ptr<AnimatedSprite> Renderer::CreateAnimatedSprite(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(filepath.string().c_str());
    if(xml_result == tinyxml2::XML_SUCCESS) {
        auto* xml_root = doc.RootElement();
        return std::make_unique<AnimatedSprite>(*xml_root);
    }
    return nullptr;
}

std::unique_ptr<AnimatedSprite> Renderer::CreateAnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const XMLElement& elem) noexcept {
    return std::make_unique<AnimatedSprite>(sheet, elem);
}

std::unique_ptr<AnimatedSprite> Renderer::CreateAnimatedSprite(const XMLElement& elem) noexcept {
    return std::make_unique<AnimatedSprite>(elem);
}

std::unique_ptr<AnimatedSprite> Renderer::CreateAnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const IntVector2& startSpriteCoords /* = IntVector2::ZERO*/) noexcept {
    return std::make_unique<AnimatedSprite>(sheet, startSpriteCoords);
}

std::unique_ptr<AnimatedSprite> Renderer::CreateAnimatedSprite(const AnimatedSpriteDesc& desc) noexcept {
    return std::make_unique<AnimatedSprite>(desc);
}

std::shared_ptr<SpriteSheet> Renderer::CreateSpriteSheet(const XMLElement& elem) noexcept {
    return std::make_shared<SpriteSheet>(elem);
}

std::shared_ptr<SpriteSheet> Renderer::CreateSpriteSheet(Texture* texture, int tilesWide, int tilesHigh) noexcept {
    return std::shared_ptr<SpriteSheet>(new SpriteSheet(texture, tilesWide, tilesHigh));
}

std::shared_ptr<SpriteSheet> Renderer::CreateSpriteSheet(const std::filesystem::path& filepath, unsigned int width /*= 1*/, unsigned int height /*= 1*/) noexcept {
    namespace FS = std::filesystem;
    FS::path p(filepath);
    p = FS::canonical(p);
    p.make_preferred();
    if(!FS::exists(p)) {
        DebuggerPrintf((p.string() + " not found.\n").c_str());
        return nullptr;
    }
    tinyxml2::XMLDocument doc;
    auto xml_load = doc.LoadFile(p.string().c_str());
    if(xml_load == tinyxml2::XML_SUCCESS) {
        auto* xml_root = doc.RootElement();
        return CreateSpriteSheet(*xml_root);
    }
    return std::shared_ptr<SpriteSheet>(new SpriteSheet(p, width, height));
}

//TODO (Casey): Audit template usage
//void Renderer::Draw(const PrimitiveType& topology, VertexBuffer* vbo, std::size_t vertex_count) noexcept {
//    GUARANTEE_OR_DIE(_current_material, "Attempting to call Draw function without a material set!\n");
//    D3D11_PRIMITIVE_TOPOLOGY d3d_prim = PrimitiveTypeToD3dTopology(topology);
//    _rhi_context->GetDxContext()->IASetPrimitiveTopology(d3d_prim);
//    unsigned int stride = sizeof(VertexBuffer::arraybuffer_t);
//    unsigned int offsets = 0;
//    const auto dx_vbo_buffer = vbo->GetDxBuffer();
//    _rhi_context->GetDxContext()->IASetVertexBuffers(0, 1, dx_vbo_buffer.GetAddressOf(), &stride, &offsets);
//    _rhi_context->Draw(vertex_count);
//}

void Renderer::DrawInstanced(const PrimitiveType& topology, VertexBuffer* vbo, VertexBufferInstanced* vbio, std::size_t vertexPerInstanceCount, std::size_t instanceCount, std::size_t startVertexLocation, std::size_t startInstanceLocation) noexcept {
    GUARANTEE_OR_DIE(m_current_material, "Attempting to call Draw function without a material set!\n");
    D3D11_PRIMITIVE_TOPOLOGY d3d_prim = PrimitiveTypeToD3dTopology(topology);
    m_rhi_context->GetDxContext()->IASetPrimitiveTopology(d3d_prim);
    unsigned int stride = sizeof(VertexBuffer::arraybuffer_t);
    unsigned int offsets = 0;
    const auto dx_vbo_buffer = vbo->GetDxBuffer();
    const auto dx_vbio_buffer = vbio->GetDxBuffer();
    ID3D11Buffer* const dx_buffers[] = {vbo->GetDxBuffer().Get(), vbio->GetDxBuffer().Get()};
    m_rhi_context->GetDxContext()->IASetVertexBuffers(0, 2, dx_buffers, &stride, &offsets);
    m_rhi_context->DrawInstanced(vertexPerInstanceCount, instanceCount, startVertexLocation, startInstanceLocation);
}

//TODO (Casey): Audit template usage
//void Renderer::DrawIndexed(const PrimitiveType& topology, VertexBuffer* vbo, IndexBuffer* ibo, std::size_t index_count, std::size_t startVertex /*= 0*/, std::size_t baseVertexLocation /*= 0*/) noexcept {
//    GUARANTEE_OR_DIE(_current_material, "Attempting to call Draw function without a material set!\n");
//    D3D11_PRIMITIVE_TOPOLOGY d3d_prim = PrimitiveTypeToD3dTopology(topology);
//    _rhi_context->GetDxContext()->IASetPrimitiveTopology(d3d_prim);
//    unsigned int stride = sizeof(VertexBuffer::arraybuffer_t);
//    unsigned int offsets = 0;
//    const auto dx_vbo_buffer = vbo->GetDxBuffer();
//    auto dx_ibo_buffer = ibo->GetDxBuffer();
//    _rhi_context->GetDxContext()->IASetVertexBuffers(0, 1, dx_vbo_buffer.GetAddressOf(), &stride, &offsets);
//    _rhi_context->GetDxContext()->IASetIndexBuffer(dx_ibo_buffer.Get(), DXGI_FORMAT_R32_UINT, offsets);
//    _rhi_context->DrawIndexed(index_count, startVertex, baseVertexLocation);
//}

void Renderer::DrawIndexedInstanced(const PrimitiveType& topology, VertexBuffer* vbo, VertexBufferInstanced* vbio, IndexBuffer* ibo, std::size_t indexPerInstanceCount, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept {
    GUARANTEE_OR_DIE(m_current_material, "Attempting to call Draw function without a material set!\n");
    D3D11_PRIMITIVE_TOPOLOGY d3d_prim = PrimitiveTypeToD3dTopology(topology);
    m_rhi_context->GetDxContext()->IASetPrimitiveTopology(d3d_prim);
    unsigned int stride = sizeof(VertexBuffer::arraybuffer_t);
    unsigned int offsets = 0;
    const auto dx_vbo_buffer = vbo->GetDxBuffer();
    const auto dx_vbio_buffer = vbio->GetDxBuffer();
    ID3D11Buffer* const dx_buffers[] = {vbo->GetDxBuffer().Get(), vbio->GetDxBuffer().Get()};
    auto dx_ibo_buffer = ibo->GetDxBuffer();
    m_rhi_context->GetDxContext()->IASetVertexBuffers(0, 2, dx_buffers, &stride, &offsets);
    m_rhi_context->GetDxContext()->IASetIndexBuffer(dx_ibo_buffer.Get(), DXGI_FORMAT_R32_UINT, offsets);
    m_rhi_context->DrawIndexedInstanced(indexPerInstanceCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void Renderer::DrawPoint2D(float pointX, float pointY, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    std::vector<Vertex3D> vbo{};
    vbo.reserve(1);
    vbo.emplace_back(Vector3(pointX, pointY, 0.0f), color);
    std::vector<unsigned int> ibo{};
    ibo.reserve(1);
    ibo.push_back(0);
    DrawIndexed(PrimitiveType::Points, vbo, ibo);
}
void Renderer::DrawPoint2D(const Vector2& point, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    DrawPoint2D(point.x, point.y, color);
}

void Renderer::DrawLine2D(float startX, float startY, float endX, float endY, const Rgba& color /*= Rgba::WHITE*/, float thickness /*= 0.0f*/) noexcept {
    bool use_thickness = thickness > 0.0f;
    if(!use_thickness) {
        Vertex3D start = Vertex3D(Vector3(Vector2(startX, startY), 0.0f), color, Vector2::Zero);
        Vertex3D end = Vertex3D(Vector3(Vector2(endX, endY), 0.0f), color, Vector2::One);
        std::vector<Vertex3D> vbo = {
        start, end};
        std::vector<unsigned int> ibo = {
        0, 1};
        DrawIndexed(PrimitiveType::Lines, vbo, ibo);
        return;
    }
    Vector3 start = Vector3(Vector2(startX, startY), 0.0f);
    Vector3 end = Vector3(Vector2(endX, endY), 0.0f);
    Vector3 displacement = end - start;
    float length = displacement.CalcLength();
    if(length > 0.0f) {
        Vector3 direction = displacement.GetNormalize();
        Vector3 left_normal = Vector3(-direction.y, direction.x, 0.0f);
        Vector3 right_normal = Vector3(direction.y, -direction.x, 0.0f);
        Vector3 start_left = start + left_normal * thickness * 0.5f;
        Vector3 start_right = start + right_normal * thickness * 0.5f;
        Vector3 end_left = end + left_normal * thickness * 0.5f;
        Vector3 end_right = end + right_normal * thickness * 0.5f;
        DrawQuad2D(Vector2(start + direction * length * 0.5f), Vector2(displacement * 0.5f), color);
    }
}

void Renderer::DrawLine2D(const Vector2& start, const Vector2& end, const Rgba& color /*= Rgba::WHITE*/, float thickness /*= 0.0f*/) noexcept {
    DrawLine2D(start.x, start.y, end.x, end.y, color, thickness);
}

void Renderer::DrawQuad2D(float left, float bottom, float right, float top, const Rgba& color /*= Rgba::WHITE*/, const Vector4& texCoords /*= Vector4::ZW_AXIS*/) noexcept {
    Vector3 v_lb = Vector3(left, bottom, 0.0f);
    Vector3 v_rt = Vector3(right, top, 0.0f);
    Vector3 v_lt = Vector3(left, top, 0.0f);
    Vector3 v_rb = Vector3(right, bottom, 0.0f);
    Vector2 uv_lt = Vector2(texCoords.x, texCoords.y);
    Vector2 uv_lb = Vector2(texCoords.x, texCoords.w);
    Vector2 uv_rt = Vector2(texCoords.z, texCoords.y);
    Vector2 uv_rb = Vector2(texCoords.z, texCoords.w);
    std::vector<Vertex3D> vbo = {
    Vertex3D(v_lb, color, uv_lb), Vertex3D(v_lt, color, uv_lt), Vertex3D(v_rt, color, uv_rt), Vertex3D(v_rb, color, uv_rb)};
    std::vector<unsigned int> ibo = {
    0, 1, 2, 0, 2, 3};
    DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
}

void Renderer::DrawQuad2D(const Rgba& color) noexcept {
    DrawQuad2D(Vector2::Zero, Vector2(0.5f, 0.5f), color);
}

void Renderer::DrawQuad2D(const Vector2& position /*= Vector2::ZERO*/, const Vector2& halfExtents /*= Vector2(0.5f, 0.5f)*/, const Rgba& color /*= Rgba::WHITE*/, const Vector4& texCoords /*= Vector4::ZW_AXIS*/) noexcept {
    float left = position.x - halfExtents.x;
    float bottom = position.y + halfExtents.y;
    float right = position.x + halfExtents.x;
    float top = position.y - halfExtents.y;
    DrawQuad2D(left, bottom, right, top, color, texCoords);
}

void Renderer::DrawQuad2D(const Vector4& texCoords) noexcept {
    DrawQuad2D(Vector2::Zero, Vector2(0.5f, 0.5f), Rgba::White, texCoords);
}

void Renderer::DrawQuad2D(const Rgba& color, const Vector4& texCoords) noexcept {
    DrawQuad2D(Vector2::Zero, Vector2(0.5f, 0.5f), color, texCoords);
}

void Renderer::DrawQuad2D(const Matrix4& transform, const Rgba& color /*= Rgba::White*/, const Vector4& texCoords /*= Vector4::ZW_AXIS*/) noexcept {
    SetModelMatrix(transform);
    DrawQuad2D(Vector2::Zero, Vector2{0.5f, 0.5f}, color, texCoords);
}

void Renderer::DrawCircle2D(float centerX, float centerY, float radius, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    //TODO: Render using GPU. See https://www.youtube.com/watch?v=xf7Y988cPRk
    DrawPolygon2D(centerX, centerY, radius, 65, color);
}

void Renderer::DrawCircle2D(const Matrix4& transform, float thickness, const Rgba& color /*= Rgba::WHITE*/, float fade /*= 0.00025f*/) noexcept {
    if(auto mat = GetMaterial("__circle2d"); mat != GetMaterial("__invalid")) {
        SetMaterial(mat);
        SetModelMatrix(transform);
        std::vector<VertexCircle2D> vbo{
            VertexCircle2D{Vector3::Zero, color, Vector2{thickness, fade}}
        };
        Draw(PrimitiveType::Triangles, vbo);
    }
}

void Renderer::DrawCircle2D(const Vector2& center, float radius, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    DrawCircle2D(center.x, center.y, radius, color);
}

void Renderer::DrawCircle2D(const Disc2& circle, const Rgba& color /*= Rgba::White*/) noexcept {
    DrawCircle2D(circle.center, circle.radius, color);
}

void Renderer::DrawFilledCircle2D(const Disc2& circle, const Rgba& color /*= Rgba::White*/) noexcept {
    DrawFilledCircle2D(circle.center, circle.radius, color);
}

void Renderer::DrawFilledCircle2D(const Vector2& center, float radius, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    //TODO: Replace with shader

    const auto num_sides = std::size_t{65};
    const auto size = num_sides + 1u;
    std::vector<Vector3> verts{};
    verts.reserve(size);
    const auto anglePerVertex = 360.0f / static_cast<float>(num_sides);
    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        const auto radians = MathUtils::ConvertDegreesToRadians(degrees);
        const auto pX = radius * std::cos(radians) + center.x;
        const auto pY = radius * std::sin(radians) + center.y;
        verts.emplace_back(Vector2(pX, pY), 0.0f);
    }

    std::vector<Vertex3D> vbo;
    vbo.reserve(verts.size());
    for(const auto& vert : verts) {
        vbo.emplace_back(vert, color);
    }

    std::vector<unsigned int> ibo(num_sides * 3);
    unsigned int j = 1u;
    for(std::size_t i = 1; i < ibo.size(); i += 3) {
        ibo[i] = (j++);
        ibo[i + 1] = (j);
    }
    DrawIndexed(PrimitiveType::TriangleStrip, vbo, ibo);
}

void Renderer::DrawAABB2(const AABB2& bounds, const Rgba& edgeColor, const Rgba& fillColor, const Vector2& edgeHalfExtents /*= Vector2::ZERO*/) noexcept {
    const Vector2 lt_inner(bounds.mins.x, bounds.mins.y);
    const Vector2 lb_inner(bounds.mins.x, bounds.maxs.y);
    const Vector2 rt_inner(bounds.maxs.x, bounds.mins.y);
    const Vector2 rb_inner(bounds.maxs.x, bounds.maxs.y);
    const Vector2 lt_outer(bounds.mins.x - edgeHalfExtents.x, bounds.mins.y - edgeHalfExtents.y);
    const Vector2 lb_outer(bounds.mins.x - edgeHalfExtents.x, bounds.maxs.y + edgeHalfExtents.y);
    const Vector2 rt_outer(bounds.maxs.x + edgeHalfExtents.x, bounds.mins.y - edgeHalfExtents.y);
    const Vector2 rb_outer(bounds.maxs.x + edgeHalfExtents.x, bounds.maxs.y + edgeHalfExtents.y);

    const std::vector<Vertex3D> vbo = {
    Vertex3D(Vector3(rt_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lt_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lt_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rt_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rb_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(rb_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(lb_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lb_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rt_inner, 0.0f), fillColor),
    Vertex3D(Vector3(lt_inner, 0.0f), fillColor),
    Vertex3D(Vector3(lb_inner, 0.0f), fillColor),
    Vertex3D(Vector3(rb_inner, 0.0f), fillColor),
    };
    // clang-format off
    const std::vector<unsigned int> ibo{
    8, 9, 10, //Fill
    8, 10, 11,
    0, 1, 2, //Top
    0, 2, 3,
    4, 0, 3, //Right
    4, 3, 5,
    6, 4, 5, //Bottom
    6, 5, 7,
    1, 6, 7, //Left
    1, 7, 2,
    };
    // clang-format on

    if(edgeHalfExtents == Vector2::Zero) {
        DrawIndexed(PrimitiveType::Lines, vbo, ibo, ibo.size() - 6, 6);
    } else {
        DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
    }
}

void Renderer::DrawAABB2(const Rgba& edgeColor, const Rgba& fillColor) noexcept {
    AABB2 bounds;
    bounds.mins = Vector2(-0.5f, -0.5f);
    bounds.maxs = Vector2(0.5f, 0.5f);
    Vector2 edge_half_extents = Vector2::Zero;
    DrawAABB2(bounds, edgeColor, fillColor, edge_half_extents);
}

void Renderer::DrawOBB2(const OBB2& obb, const Rgba& edgeColor, const Rgba& fillColor /*= Rgba::NoAlpha*/, const Vector2& edgeHalfExtents /*= Vector2::ZERO*/) noexcept {
    Vector2 lt = obb.GetTopLeft();
    Vector2 lb = obb.GetBottomLeft();
    Vector2 rt = obb.GetTopRight();
    Vector2 rb = obb.GetBottomRight();
    Vector2 lt_inner(lt);
    Vector2 lb_inner(lb);
    Vector2 rt_inner(rt);
    Vector2 rb_inner(rb);
    Vector2 lt_outer(lt.x - edgeHalfExtents.x, lt.y - edgeHalfExtents.y);
    Vector2 lb_outer(lb.x - edgeHalfExtents.x, lb.y + edgeHalfExtents.y);
    Vector2 rt_outer(rt.x + edgeHalfExtents.x, rt.y - edgeHalfExtents.y);
    Vector2 rb_outer(rb.x + edgeHalfExtents.x, rb.y + edgeHalfExtents.y);
    const std::vector<Vertex3D> vbo = {
    Vertex3D(Vector3(rt_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lt_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lt_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rt_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rb_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(rb_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(lb_outer, 0.0f), edgeColor),
    Vertex3D(Vector3(lb_inner, 0.0f), edgeColor),
    Vertex3D(Vector3(rt_inner, 0.0f), fillColor),
    Vertex3D(Vector3(lt_inner, 0.0f), fillColor),
    Vertex3D(Vector3(lb_inner, 0.0f), fillColor),
    Vertex3D(Vector3(rb_inner, 0.0f), fillColor),
    };
    // clang-format off
    const std::vector<unsigned int> ibo = {
    8, 9, 10,  //Fill
    8, 10, 11,
    0, 1, 2, //Top
    0, 2, 3,
    4, 0, 3, //Right
    4, 3, 5,
    6, 4, 5, //Bottom
    6, 5, 7,
    1, 6, 7, //Left
    1, 7, 2,
    };
    // clang-format on
    if(edgeHalfExtents == Vector2::Zero) {
        DrawIndexed(PrimitiveType::Lines, vbo, ibo, ibo.size() - 6, 6);
    } else {
        DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
    }
}

void Renderer::DrawOBB2(float orientationDegrees, const Rgba& edgeColor, const Rgba& fillColor /*= Rgba::NoAlpha*/) noexcept {
    OBB2 obb;
    obb.half_extents = Vector2(0.5f, 0.5f);
    obb.orientationDegrees = orientationDegrees;
    auto edge_half_extents = Vector2::Zero;
    DrawOBB2(obb, edgeColor, fillColor, edge_half_extents);
}

void Renderer::DrawX2D(const Vector2& position /*= Vector2::ZERO*/, const Vector2& half_extents /*= Vector2(0.5f, 0.5f)*/, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    float left = position.x - half_extents.x;
    float top = position.y - half_extents.y;
    float right = position.x + half_extents.x;
    float bottom = position.y + half_extents.y;
    Vector3 lt = Vector3(left, top, 0.0f);
    Vector3 rt = Vector3(right, top, 0.0f);
    Vector3 lb = Vector3(left, bottom, 0.0f);
    Vector3 rb = Vector3(right, bottom, 0.0f);
    std::vector<Vertex3D> vbo = {
    Vertex3D(lt, color),
    Vertex3D(rb, color),
    Vertex3D(lb, color),
    Vertex3D(rt, color),
    };

    std::vector<unsigned int> ibo = {
    0, 1, 2, 3};

    DrawIndexed(PrimitiveType::Lines, vbo, ibo);
}

void Renderer::DrawX2D(const Rgba& color) noexcept {
    DrawX2D(Vector2::Zero, Vector2(0.5f, 0.5f), color);
}

void Renderer::DrawPolygon2D(float centerX, float centerY, float radius, std::size_t numSides /*= 3*/, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    auto num_sides_as_float = static_cast<float>(numSides);
    std::vector<Vector3> verts;
    verts.reserve(numSides);
    float anglePerVertex = 360.0f / num_sides_as_float;
    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        float radians = MathUtils::ConvertDegreesToRadians(degrees);
        float pX = radius * std::cos(radians) + centerX;
        float pY = radius * std::sin(radians) + centerY;
        verts.emplace_back(Vector2(pX, pY), 0.0f);
    }

    std::vector<Vertex3D> vbo;
    vbo.resize(verts.size());
    for(std::size_t i = 0; i < vbo.size(); ++i) {
        vbo[i] = Vertex3D(verts[i], color);
    }

    std::vector<unsigned int> ibo;
    ibo.resize(numSides + 1);
    for(std::size_t i = 0; i < ibo.size(); ++i) {
        ibo[i] = static_cast<unsigned int>(i % numSides);
    }
    DrawIndexed(PrimitiveType::LinesStrip, vbo, ibo);
}

void Renderer::DrawPolygon2D(const Vector2& center, float radius, std::size_t numSides /*= 3*/, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    DrawPolygon2D(center.x, center.y, radius, numSides, color);
}

void Renderer::DrawPolygon2D(const Polygon2& polygon, const Rgba& color /*= Rgba::White*/) noexcept {
    const std::vector<Vertex3D> vbo = [&polygon, &color]() {
        std::vector<Vertex3D> buffer;
        buffer.reserve(polygon.GetVerts().size());
        for(const auto& v : polygon.GetVerts()) {
            buffer.push_back(Vertex3D(Vector3(v, 0.0f), color));
        }
        return buffer;
    }();
    const std::vector<unsigned int> ibo = [this, &vbo]() {
        std::vector<unsigned int> buffer(vbo.size() + 1);
        for(unsigned int i = 0u; i < buffer.size(); ++i) {
            buffer[i] = i % vbo.size();
        }
        return buffer;
    }();
    DrawIndexed(PrimitiveType::LinesStrip, vbo, ibo);
}

void Renderer::DrawFilledPolygon2D(float centerX, float centerY, float radius, std::size_t numSides /*= 3*/, const Rgba& color /*= Rgba::White*/) noexcept {
    auto num_sides_as_float = static_cast<float>(numSides);
    std::vector<Vector3> verts;
    verts.reserve(numSides);
    float anglePerVertex = 360.0f / num_sides_as_float;
    for(float degrees = 0.0f; degrees < 360.0f; degrees += anglePerVertex) {
        float radians = MathUtils::ConvertDegreesToRadians(degrees);
        float pX = radius * std::cos(radians) + centerX;
        float pY = radius * std::sin(radians) + centerY;
        verts.emplace_back(Vector2(pX, pY), 0.0f);
    }

    std::vector<Vertex3D> vbo;
    vbo.resize(verts.size());
    for(std::size_t i = 0; i < vbo.size(); ++i) {
        vbo[i] = Vertex3D(verts[i], color);
    }

    std::vector<unsigned int> ibo;
    ibo.resize(numSides + 1);
    for(std::size_t i = 0; i < ibo.size(); ++i) {
        ibo[i] = static_cast<unsigned int>(i % numSides);
    }
    DrawIndexed(PrimitiveType::TriangleStrip, vbo, ibo);
}

void Renderer::DrawFilledPolygon2D(const Vector2& center, float radius, std::size_t numSides /*= 3*/, const Rgba& color /*= Rgba::White*/) noexcept {
    DrawFilledPolygon2D(center.x, center.y, radius, numSides, color);
}

void Renderer::DrawFilledPolygon2D(const Polygon2& polygon, const Rgba& color /*= Rgba::White*/) noexcept {
    const std::vector<Vertex3D> vbo = [&polygon, &color]() {
        std::vector<Vertex3D> buffer;
        buffer.reserve(polygon.GetVerts().size());
        for(const auto& v : polygon.GetVerts()) {
            buffer.push_back(Vertex3D(Vector3(v, 0.0f), color));
        }
        return buffer;
    }();
    const std::vector<unsigned int> ibo = [this, &vbo]() {
        std::vector<unsigned int> buffer(vbo.size() + 1);
        for(unsigned int i = 0u; i < buffer.size(); ++i) {
            buffer[i] = i % vbo.size();
        }
        return buffer;
    }();
    DrawIndexed(PrimitiveType::TriangleStrip, vbo, ibo);
}

void Renderer::DrawTextLine(const KerningFont* font, const std::string& text, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    if(font == nullptr) {
        return;
    }
    if(text.empty()) {
        return;
    }
    float cursor_x = 0.0f;
    float cursor_y = 0.0f;
    float line_top = cursor_y - font->GetCommonDef().base;
    auto texture_w = static_cast<float>(font->GetCommonDef().scale.x);
    auto texture_h = static_cast<float>(font->GetCommonDef().scale.y);
    std::size_t text_size = text.size();

    Mesh::Builder builder;
    builder.verticies.reserve(text_size * 4);
    builder.indicies.reserve(text_size * 6);

    builder.Begin(PrimitiveType::Triangles);
    
    builder.SetColor(color);

    for(auto text_iter = text.begin(); text_iter != text.end(); /* DO NOTHING */) {
        KerningFont::CharDef current_def = font->GetCharDef(*text_iter);
        float char_uvl = current_def.position.x / texture_w;
        float char_uvt = current_def.position.y / texture_h;
        float char_uvr = char_uvl + (current_def.dimensions.x / texture_w);
        float char_uvb = char_uvt + (current_def.dimensions.y / texture_h);

        float quad_top = line_top + current_def.offsets.y;
        float quad_bottom = quad_top + current_def.dimensions.y;
        float quad_left = cursor_x - current_def.offsets.x;
        float quad_right = quad_left + current_def.dimensions.x;

        builder.SetUV(Vector2{char_uvl, char_uvb});
        builder.AddVertex(Vector2(quad_left, quad_bottom));

        builder.SetUV(Vector2{char_uvl, char_uvt});
        builder.AddVertex(Vector2(quad_left, quad_top));

        builder.SetUV(Vector2{char_uvr, char_uvt});
        builder.AddVertex(Vector2(quad_right, quad_top));

        builder.SetUV(Vector2{char_uvr, char_uvb});
        builder.AddVertex(Vector2{quad_right, quad_bottom});

        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        std::string::const_iterator previous_char = text_iter;
        ++text_iter;
        if(text_iter != text.end()) {
            int kern_value = font->GetKerningValue(*previous_char, *text_iter);
            cursor_x += (current_def.xadvance + kern_value);
        }
    }

    builder.End(font->GetMaterial());
    if(const auto& cbs = font->GetMaterial()->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        auto& font_cb = cbs[0].get();
        Vector4 channel{1.0f, 1.0f, 1.0f, 1.0f};
        font_cb.Update(*m_rhi_context, &channel);
    }
    Mesh::Render(builder);
}

void Renderer::DrawTextLine(const Matrix4& transform, const KerningFont* font, const std::string& text, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    if(font == nullptr) {
        return;
    }
    if(text.empty()) {
        return;
    }
    float cursor_x = 0.0f;
    float cursor_y = 0.0f;
    float line_top = cursor_y - font->GetCommonDef().base;
    auto texture_w = static_cast<float>(font->GetCommonDef().scale.x);
    auto texture_h = static_cast<float>(font->GetCommonDef().scale.y);
    std::size_t text_size = text.size();

    Mesh::Builder builder;
    builder.verticies.reserve(text_size * 4);
    builder.indicies.reserve(text_size * 6);

    builder.Begin(PrimitiveType::Triangles);

    builder.SetColor(color);

    for(auto text_iter = text.begin(); text_iter != text.end(); /* DO NOTHING */) {
        KerningFont::CharDef current_def = font->GetCharDef(*text_iter);
        float char_uvl = current_def.position.x / texture_w;
        float char_uvt = current_def.position.y / texture_h;
        float char_uvr = char_uvl + (current_def.dimensions.x / texture_w);
        float char_uvb = char_uvt + (current_def.dimensions.y / texture_h);

        float quad_top = line_top + current_def.offsets.y;
        float quad_bottom = quad_top + current_def.dimensions.y;
        float quad_left = cursor_x - current_def.offsets.x;
        float quad_right = quad_left + current_def.dimensions.x;

        builder.SetUV(Vector2{char_uvl, char_uvb});
        builder.AddVertex(Vector2(quad_left, quad_bottom));

        builder.SetUV(Vector2{char_uvl, char_uvt});
        builder.AddVertex(Vector2(quad_left, quad_top));

        builder.SetUV(Vector2{char_uvr, char_uvt});
        builder.AddVertex(Vector2(quad_right, quad_top));

        builder.SetUV(Vector2{char_uvr, char_uvb});
        builder.AddVertex(Vector2{quad_right, quad_bottom});

        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        std::string::const_iterator previous_char = text_iter;
        ++text_iter;
        if(text_iter != text.end()) {
            int kern_value = font->GetKerningValue(*previous_char, *text_iter);
            cursor_x += (current_def.xadvance + kern_value);
        }
    }

    builder.End(font->GetMaterial());
    const auto& cbs = font->GetMaterial()->GetShader()->GetConstantBuffers();
    auto has_constant_buffers = !cbs.empty();
    if(has_constant_buffers) {
        auto& font_cb = cbs[0].get();
        Vector4 channel{1.0f, 1.0f, 1.0f, 1.0f};
        font_cb.Update(*m_rhi_context, &channel);
    }
    SetModelMatrix(transform);
    Mesh::Render(builder);
}

void Renderer::DrawMultilineText(KerningFont* font, const std::string& text, const Rgba& color /*= Rgba::WHITE*/) noexcept {
    float y = font->GetLineHeight();
    float draw_loc_y = 0.0f;
    float draw_loc_x = 0.0f;
    auto draw_loc = Vector2(draw_loc_x * 0.99f, draw_loc_y);

    std::vector<Vertex3D> vbo{};
    std::vector<unsigned int> ibo{};
    std::vector<std::string> lines = StringUtils::Split(text, '\n', false);
    for(auto& line : lines) {
        draw_loc.y += y;
        AppendMultiLineTextBuffer(font, line, draw_loc, color, vbo, ibo);
    }
    const auto& cbs = font->GetMaterial()->GetShader()->GetConstantBuffers();
    auto has_constant_buffers = !cbs.empty();
    if(has_constant_buffers) {
        auto& font_cb = cbs[0].get();
        Vector4 channel{1.0f, 1.0f, 1.0f, 1.0f};
        font_cb.Update(*m_rhi_context, &channel);
    }
    SetMaterial(font->GetMaterial());
    DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
}

void Renderer::AppendMultiLineTextBuffer(KerningFont* font, const std::string& text, const Vector2& start_position, const Rgba& color, std::vector<Vertex3D>& vbo, std::vector<unsigned int>& ibo) noexcept {
    if(font == nullptr) {
        return;
    }
    if(text.empty()) {
        return;
    }

    float cursor_x = start_position.x;
    float cursor_y = start_position.y;
    float line_top = cursor_y - font->GetCommonDef().base;
    auto texture_w = static_cast<float>(font->GetCommonDef().scale.x);
    auto texture_h = static_cast<float>(font->GetCommonDef().scale.y);
    std::size_t text_size = text.size();
    vbo.reserve(text_size * 4);
    ibo.reserve(text_size * 6);

    for(auto text_iter = text.begin(); text_iter != text.end(); /* DO NOTHING */) {
        KerningFont::CharDef current_def = font->GetCharDef(*text_iter);
        float char_uvl = current_def.position.x / texture_w;
        float char_uvt = current_def.position.y / texture_h;
        float char_uvr = char_uvl + (current_def.dimensions.x / texture_w);
        float char_uvb = char_uvt + (current_def.dimensions.y / texture_h);

        float quad_top = line_top + current_def.offsets.y;
        float quad_bottom = quad_top + current_def.dimensions.y;
        float quad_left = cursor_x - current_def.offsets.x;
        float quad_right = quad_left + current_def.dimensions.x;

        vbo.emplace_back(Vector3(quad_left, quad_bottom, 0.0f), color, Vector2(char_uvl, char_uvb));
        vbo.emplace_back(Vector3(quad_left, quad_top, 0.0f), color, Vector2(char_uvl, char_uvt));
        vbo.emplace_back(Vector3(quad_right, quad_top, 0.0f), color, Vector2(char_uvr, char_uvt));
        vbo.emplace_back(Vector3(quad_right, quad_bottom, 0.0f), color, Vector2(char_uvr, char_uvb));

        auto s = static_cast<unsigned int>(vbo.size());
        ibo.push_back(s - 4);
        ibo.push_back(s - 3);
        ibo.push_back(s - 2);
        ibo.push_back(s - 4);
        ibo.push_back(s - 2);
        ibo.push_back(s - 1);

        std::string::const_iterator previous_char = text_iter;
        ++text_iter;
        if(text_iter != text.end()) {
            int kern_value = font->GetKerningValue(*previous_char, *text_iter);
            cursor_x += (current_def.xadvance + kern_value);
        }
    }
}

std::vector<std::unique_ptr<ConstantBuffer>> Renderer::CreateConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* _shader_program) noexcept {
    auto vs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, _shader_program->GetVSByteCode());
    auto hs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, _shader_program->GetHSByteCode());
    auto ds_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, _shader_program->GetDSByteCode());
    auto gs_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, _shader_program->GetGSByteCode());
    auto ps_cbuffers = RHIDevice::CreateConstantBuffersFromByteCode(device, _shader_program->GetPSByteCode());
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

std::vector<std::unique_ptr<ConstantBuffer>> Renderer::CreateComputeConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept {
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

void Renderer::SetWinProc(const std::function<bool(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>& windowProcedure) noexcept {
    if(auto* output = GetOutput()) {
        if(auto* window = output->GetWindow()) {
            window->custom_message_handler = windowProcedure;
        }
    }
}

void Renderer::CopyTexture(const Texture* src, Texture* dst) const noexcept {
    if((src && dst) && src != dst) {
        auto* dc = GetDeviceContext();
        auto* dx_dc = dc->GetDxContext();
        dx_dc->CopyResource(dst->GetDxResource(), src->GetDxResource());
    }
}

void Renderer::ResizeBuffers() noexcept {
    GetOutput()->ResetBackbuffer();
}

void Renderer::ClearState() noexcept {
    m_current_material = nullptr;
    m_rhi_context->GetDxContext()->OMSetRenderTargets(0, nullptr, nullptr);
    m_rhi_context->ClearState();
    m_rhi_context->Flush();
}

void Renderer::RequestScreenShot(std::filesystem::path saveLocation) {
    namespace FS = std::filesystem;
    const auto folderLocation = saveLocation.parent_path();
    if(!FS::exists(folderLocation)) {
        const auto err_str = folderLocation.string() + " does not exist.\n";
        DebuggerPrintf(err_str.c_str());
    }
    m_screenshot = saveLocation;
    m_last_screenshot_location = saveLocation;
}

void Renderer::RequestScreenShot() {
    namespace FS = std::filesystem;
    if(m_last_screenshot_location.empty()) {
        const auto folder = screenshot_job_t{FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineData) / std::filesystem::path{"Screenshots"}};
        TimeUtils::DateTimeStampOptions options{};
        options.include_milliseconds = true;
        options.is_filename = true;
        options.use_24_hour_clock = true;
        const auto screenshot_tag = TimeUtils::GetDateTimeStampFromNow(options);
        const auto filepath = folder / FS::path{"Screenshot_" + screenshot_tag + ".png"};
        m_last_screenshot_location = filepath;
        m_screenshot = m_last_screenshot_location;
    }
    RequestScreenShot(m_last_screenshot_location);
}

constexpr unsigned int Renderer::GetMatrixBufferIndex() const noexcept {
    return 0;
}

constexpr unsigned int Renderer::GetTimeBufferIndex() const noexcept {
    return 1;
}

constexpr unsigned int Renderer::GetLightingBufferIndex() const noexcept {
    return 2;
}

constexpr unsigned int Renderer::GetConstantBufferStartIndex() const noexcept {
    return 3;
}

constexpr unsigned int Renderer::GetStructuredBufferStartIndex() const noexcept {
    return 64;
}

constexpr unsigned int Renderer::GetMaxLightCount() const noexcept {
    return max_light_count;
}

Image Renderer::GetBackbufferAsImage() const noexcept {
    std::vector<Rgba> data{};
    const auto* bb = GetOutput()->GetBackBuffer();
    return Image{bb, this};
}

void Renderer::FulfillScreenshotRequest() noexcept {
    if(m_screenshot && !m_last_screenshot_location.empty()) {
        //TODO: Make this a job so game doesn't lag
        //const auto cb = [this](void*) {
        //TODO: Replace with FrameBuffer object
        //auto img = GetFullscreenTextureAsImage();
        //if(!img.Export(_screenshot)) {
        //    const auto err = "Could not export to \"" + _screenshot.operator std::string() + "\".\n";
        //    ServiceLocator::get<IFileLoggerService>()->LogAndFlush(err);
        //}
        //_screenshot.clear();
        //};
        //_jobSystem.Run(JobType::Generic, cb, nullptr);
    }
}

void Renderer::DispatchComputeJob(const ComputeJob& job) noexcept {
    SetComputeShader(job.computeShader);
    auto* dc = GetDeviceContext();
    auto* dx_dc = dc->GetDxContext();
    for(auto i = 0u; i < job.uavCount; ++i) {
        dc->SetUnorderedAccessView(i, job.uavTextures[i]);
    }
    dx_dc->Dispatch(job.threadGroupCountX, job.threadGroupCountY, job.threadGroupCountZ);
}

Texture* Renderer::GetDefaultDepthStencil() const noexcept {
    return m_default_depthstencil;
}

void Renderer::SetFullscreen(bool isFullscreen) noexcept {
    if(isFullscreen) {
        SetFullscreenMode();
    } else {
        SetWindowedMode();
    }
}

void Renderer::SetFullscreenMode() noexcept {
    if(auto* output = GetOutput()) {
        if(auto* window = output->GetWindow()) {
            window->SetDisplayMode(RHIOutputMode::Borderless_Fullscreen);
        }
    }
}

void Renderer::SetWindowedMode() noexcept {
    if(auto* output = GetOutput()) {
        if(auto* window = output->GetWindow()) {
            window->SetDisplayMode(RHIOutputMode::Windowed);
        }
    }
}

void Renderer::CreateAndRegisterDefaultEngineFonts() noexcept {
    std::filesystem::path p = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineData) / std::filesystem::path{"Fonts"};
    FileUtils::CreateFolders(p); //If the directory wasn't created, they either already exist or the install was corrupted.
    RegisterFontsFromFolder(p);
}

void Renderer::CreateAndRegisterDefaultShaderPrograms() noexcept {
    auto default_sp = CreateDefaultShaderProgram();
    std::string name = default_sp->GetName();
    RegisterShaderProgram(name, std::move(default_sp));

    auto unlit_sp = CreateDefaultUnlitShaderProgram();
    name = unlit_sp->GetName();
    RegisterShaderProgram(name, std::move(unlit_sp));

    auto normal_sp = CreateDefaultNormalShaderProgram();
    name = normal_sp->GetName();
    RegisterShaderProgram(name, std::move(normal_sp));

    auto normalmap_sp = CreateDefaultNormalMapShaderProgram();
    name = normalmap_sp->GetName();
    RegisterShaderProgram(name, std::move(normalmap_sp));

    auto font_sp = CreateDefaultFontShaderProgram();
    name = font_sp->GetName();
    RegisterShaderProgram(name, std::move(font_sp));

    auto circle2d_sp = CreateDefaultCircle2DShaderProgram();
    name = circle2d_sp->GetName();
    RegisterShaderProgram(name, std::move(circle2d_sp));

    auto webp_sp = CreateDefaultWebPShaderProgram();
    name = webp_sp->GetName();
    RegisterShaderProgram(name, std::move(webp_sp));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultShaderProgram() noexcept {
#if 0
    std::string program =
    R"(

static const int MAX_LIGHT_COUNT = 16;
static const float PI = 3.141592653589793238;

float3 NormalAsColor(float3 n) {
    return ((n + 1.0f) * 0.5f);
}

float3 ColorAsNormal(float3 color) {
    return ((color * 2.0f) - 1.0f);
}

float RangeMap(float valueToMap, float minInputRange, float maxInputRange, float minOutputRange, float maxOutputRange) {
    return (valueToMap - minInputRange) * (maxOutputRange - minOutputRange) / (maxInputRange - minInputRange) + minOutputRange;
}

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

struct light {
    float4 position;
    float4 color;
    float4 attenuation;
    float4 specAttenuation;
    float4 innerOuterDotThresholds;
    float4 direction;
};

cbuffer lighting_cb : register(b2) {
    light g_Lights[16];
    float4 g_lightAmbient;
    float4 g_lightSpecGlossEmitFactors;
    float4 g_lightEyePosition;
    int g_lightUseVertexNormals;
    float3 g_lightPadding;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
    float3 world_position : WORLD;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 normal = input_vertex.normal;
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;
    output.normal = normal;
    output.world_position = world.xyz;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {

    float2 uv = input_pixel.uv;
    float4 albedo = tDiffuse.Sample(sSampler, uv);
    float4 tinted_color = albedo * input_pixel.color;
    
    float use_vertex_normals = (float)g_lightUseVertexNormals;
    float use_normal_map = 1.0f - (float)g_lightUseVertexNormals;
    
    float3 normal_as_color = use_normal_map * tNormal.Sample(sSampler, uv).rgb + use_vertex_normals * input_pixel.normal.rgb;
    float3 local_normal = ColorAsNormal(normal_as_color);
    local_normal = normalize(local_normal);
    float3 world_position = input_pixel.world_position;
    float3 world_normal = mul(float4(local_normal, 0.0f), g_MODEL).xyz;

    float3 vector_to_eye = g_lightEyePosition.xyz - world_position;
    float3 direction_from_eye = -normalize(vector_to_eye);

    float3 ambient_occlusion_map_factor = tOcclusion.Sample(sSampler, uv).rgb;
    float3 ambient_light = g_lightAmbient.rgb * g_lightAmbient.a * ambient_occlusion_map_factor;

    float3 total_light_color = float3(0.0f, 0.0f, 0.0f);
    float3 total_specular_color = float3(0.0f, 0.0f, 0.0f);

    float3 reflected_eye_direction = reflect(direction_from_eye, world_normal);
    float3 debugColor = float3( 0, 0, 0 );

    [unroll]
    for(int light_index = 0; light_index < 16; ++light_index) {
        float4 light_pos = g_Lights[light_index].position;
        float4 light_color_intensity = g_Lights[light_index].color;
        float4 light_att = g_Lights[light_index].attenuation;
        float4 light_specAtt = g_Lights[light_index].specAttenuation;
        float innerDotThreshold = g_Lights[light_index].innerOuterDotThresholds.x;
        float outerDotThreshold = g_Lights[light_index].innerOuterDotThresholds.y;
        float3 light_forward = normalize(g_Lights[light_index].direction.xyz);

        float3 vector_to_light = light_pos.xyz - world_position.xyz;
        float distance_to_light = length(vector_to_light);
        float3 direction_to_light = vector_to_light / distance_to_light;

        float useDirection = light_att.w;
        float useCalcDirection = 1.0f - light_att.w;
        direction_to_light = useCalcDirection * (direction_to_light) + useDirection * (-light_forward);

        //Calculate spotlight penumbra
        float penumbra_dot = dot(-light_forward, direction_to_light);
        float penumbra_factor = saturate(RangeMap(penumbra_dot, innerDotThreshold, outerDotThreshold, 1.0f, 0.0f));
        debugColor += NormalAsColor(direction_to_light);

        //Calculate dot3
        float light_impact_factor = saturate(dot(direction_to_light, world_normal));

        float intensity_factor = light_color_intensity.a;
        float attenuation_factor = 1.0f / (light_att.x +
            distance_to_light * light_att.y +
            distance_to_light * distance_to_light * light_att.z);
        attenuation_factor = saturate(attenuation_factor);

        float3 light_color = light_color_intensity.rgb;
        total_light_color += light_color * (intensity_factor * light_impact_factor * attenuation_factor * penumbra_factor);

        float spec_attenuation_factor = 1.0f / (light_specAtt.x + distance_to_light * light_specAtt.y + distance_to_light * distance_to_light * light_specAtt.z);
        float spec_dot3 = saturate(dot(reflected_eye_direction, direction_to_light));
        float spec_factor = g_lightSpecGlossEmitFactors.x * pow(spec_dot3, g_lightSpecGlossEmitFactors.y);
        float3 spec_color = light_color * (spec_attenuation_factor * intensity_factor * spec_factor);
        total_specular_color += spec_color;
    }

    float3 diffuse_light_color = saturate(ambient_light + total_light_color);
    float3 emissive_color = tEmissive.Sample(sSampler, uv).rgb;
    float3 specular_map_color = tSpecular.Sample(sSampler, uv).rgb;

    float3 final_color = (diffuse_light_color * tinted_color.rgb) + (total_specular_color * specular_map_color) + emissive_color;
    float final_alpha = tinted_color.a;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1556>  g_VertexFunction{68, 88, 66, 67, 180, 142, 65, 18, 203, 129, 160, 1, 80, 73, 136, 88, 162, 78, 9, 248, 1, 0, 0, 0, 20, 6, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 168, 1, 0, 0, 52, 2, 0, 0, 224, 2, 0, 0, 120, 5, 0, 0, 82, 68, 69, 70, 108, 1, 0, 0, 1, 0, 0, 0, 104, 0, 0, 0, 1, 0, 0, 0, 60, 0, 0, 0, 0, 5, 254, 255, 16, 129, 4, 0, 68, 1, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 171, 171, 92, 0, 0, 0, 3, 0, 0, 0, 128, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 48, 1, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 55, 1, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 132, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 104, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 119, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 15, 0, 0, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 171, 171, 171, 79, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 12, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 0, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 8, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 83, 72, 69, 88, 144, 2, 0, 0, 80, 0, 1, 0, 164, 0, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 12, 0, 0, 0, 95, 0, 0, 3, 114, 16, 16, 0, 0, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 95, 0, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 3, 0, 0, 0, 103, 0, 0, 4, 242, 32, 16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 1, 0, 0, 0, 101, 0, 0, 3, 50, 32, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 3, 0, 0, 0, 101, 0, 0, 3, 114, 32, 16, 0, 4, 0, 0, 0, 104, 0, 0, 2, 2, 0, 0, 0, 54, 0, 0, 5, 114, 0, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 17, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 4, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 5, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 6, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 7, 0, 0, 0, 54, 0, 0, 5, 114, 32, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 17, 0, 0, 8, 18, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 8, 0, 0, 0, 17, 0, 0, 8, 34, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 8, 66, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 10, 0, 0, 0, 17, 0, 0, 8, 130, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 11, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 50, 32, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 3, 0, 0, 0, 70, 30, 16, 0, 3, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 19, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 21140> g_PixelFunction{68, 88, 66, 67, 9, 37, 155, 244, 8, 225, 192, 112, 36, 126, 194, 237, 105, 139, 211, 245, 1, 0, 0, 0, 148, 82, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 168, 5, 0, 0, 84, 6, 0, 0, 136, 6, 0, 0, 248, 81, 0, 0, 82, 68, 69, 70, 108, 5, 0, 0, 2, 0, 0, 0, 140, 1, 0, 0, 8, 0, 0, 0, 60, 0, 0, 0, 0, 5, 255, 255, 16, 129, 4, 0, 68, 5, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 60, 1, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 69, 1, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 78, 1, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 1, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 86, 1, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 3, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 96, 1, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 4, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 107, 1, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 5, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 117, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 127, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 115, 83, 97, 109, 112, 108, 101, 114, 0, 116, 68, 105, 102, 102, 117, 115, 101, 0, 116, 78, 111, 114, 109, 97, 108, 0, 116, 83, 112, 101, 99, 117, 108, 97, 114, 0, 116, 79, 99, 99, 108, 117, 115, 105, 111, 110, 0, 116, 69, 109, 105, 115, 115, 105, 118, 101, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 108, 105, 103, 104, 116, 105, 110, 103, 95, 99, 98, 0, 171, 117, 1, 0, 0, 3, 0, 0, 0, 188, 1, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 1, 0, 0, 6, 0, 0, 0, 128, 2, 0, 0, 64, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 52, 2, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 72, 2, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 108, 2, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 72, 2, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 115, 2, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 72, 2, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 2, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 112, 3, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 2, 0, 0, 0, 64, 4, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 100, 4, 0, 0, 0, 6, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 116, 4, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 152, 4, 0, 0, 16, 6, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 116, 4, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 180, 4, 0, 0, 32, 6, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 116, 4, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 199, 4, 0, 0, 48, 6, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 228, 4, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 8, 5, 0, 0, 52, 6, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 32, 5, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 76, 105, 103, 104, 116, 115, 0, 108, 105, 103, 104, 116, 0, 112, 111, 115, 105, 116, 105, 111, 110, 0, 102, 108, 111, 97, 116, 52, 0, 171, 1, 0, 3, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 136, 3, 0, 0, 99, 111, 108, 111, 114, 0, 97, 116, 116, 101, 110, 117, 97, 116, 105, 111, 110, 0, 115, 112, 101, 99, 65, 116, 116, 101, 110, 117, 97, 116, 105, 111, 110, 0, 105, 110, 110, 101, 114, 79, 117, 116, 101, 114, 68, 111, 116, 84, 104, 114, 101, 115, 104, 111, 108, 100, 115, 0, 100, 105, 114, 101, 99, 116, 105, 111, 110, 0, 127, 3, 0, 0, 144, 3, 0, 0, 0, 0, 0, 0, 180, 3, 0, 0, 144, 3, 0, 0, 16, 0, 0, 0, 186, 3, 0, 0, 144, 3, 0, 0, 32, 0, 0, 0, 198, 3, 0, 0, 144, 3, 0, 0, 48, 0, 0, 0, 214, 3, 0, 0, 144, 3, 0, 0, 64, 0, 0, 0, 238, 3, 0, 0, 144, 3, 0, 0, 80, 0, 0, 0, 5, 0, 0, 0, 1, 0, 24, 0, 16, 0, 6, 0, 248, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 121, 3, 0, 0, 103, 95, 108, 105, 103, 104, 116, 65, 109, 98, 105, 101, 110, 116, 0, 171, 1, 0, 3, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 136, 3, 0, 0, 103, 95, 108, 105, 103, 104, 116, 83, 112, 101, 99, 71, 108, 111, 115, 115, 69, 109, 105, 116, 70, 97, 99, 116, 111, 114, 115, 0, 103, 95, 108, 105, 103, 104, 116, 69, 121, 101, 80, 111, 115, 105, 116, 105, 111, 110, 0, 103, 95, 108, 105, 103, 104, 116, 85, 115, 101, 86, 101, 114, 116, 101, 120, 78, 111, 114, 109, 97, 108, 115, 0, 105, 110, 116, 0, 171, 0, 0, 2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 223, 4, 0, 0, 103, 95, 108, 105, 103, 104, 116, 80, 97, 100, 100, 105, 110, 103, 0, 102, 108, 111, 97, 116, 51, 0, 171, 171, 1, 0, 3, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 5, 0, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 7, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 7, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 79, 83, 71, 78, 44, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 84, 97, 114, 103, 101, 116, 0, 171, 171, 83, 72, 69, 88, 104, 75, 0, 0, 80, 0, 0, 0, 218, 18, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 89, 0, 0, 4, 70, 142, 32, 0, 2, 0, 0, 0, 100, 0, 0, 0, 90, 0, 0, 3, 0, 96, 16, 0, 0, 0, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 0, 0, 0, 0, 85, 85, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 1, 0, 0, 0, 85, 85, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 3, 0, 0, 0, 85, 85, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 4, 0, 0, 0, 85, 85, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 5, 0, 0, 0, 85, 85, 0, 0, 98, 16, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 98, 16, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 98, 16, 0, 3, 114, 16, 16, 0, 3, 0, 0, 0, 98, 16, 0, 3, 114, 16, 16, 0, 4, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 0, 0, 0, 0, 104, 0, 0, 2, 22, 0, 0, 0, 0, 0, 0, 10, 18, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 10, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 10, 0, 0, 0, 16, 0, 0, 9, 34, 0, 16, 0, 0, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 11, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 11, 0, 0, 0, 68, 0, 0, 5, 34, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 226, 0, 16, 0, 0, 0, 0, 0, 86, 5, 16, 0, 0, 0, 0, 0, 6, 137, 32, 0, 2, 0, 0, 0, 11, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 1, 0, 0, 0, 150, 7, 16, 128, 65, 0, 0, 0, 0, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 1, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 8, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 2, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 6, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 75, 0, 0, 5, 18, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 6, 0, 16, 0, 3, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 1, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 150, 7, 16, 128, 65, 0, 0, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 0, 0, 0, 9, 34, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 10, 0, 0, 0, 14, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 128, 65, 0, 0, 0, 0, 0, 0, 0, 10, 0, 16, 0, 0, 0, 0, 0, 0, 32, 0, 7, 18, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 34, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 3, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 8, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 8, 0, 0, 0, 50, 0, 0, 11, 66, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 3, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 9, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 9, 0, 0, 0, 50, 0, 0, 10, 66, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 9, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 50, 0, 0, 10, 34, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 8, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 14, 0, 0, 10, 34, 0, 16, 0, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 26, 0, 16, 0, 0, 0, 0, 0, 54, 32, 0, 5, 34, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 14, 0, 0, 10, 66, 0, 16, 0, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 7, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 114, 0, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 1, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 43, 0, 0, 6, 130, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 99, 0, 0, 0, 0, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 128, 65, 0, 0, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 56, 0, 0, 7, 114, 0, 16, 0, 3, 0, 0, 0, 246, 15, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 3, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 2, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 50, 0, 0, 15, 114, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 191, 0, 0, 128, 191, 0, 0, 128, 191, 0, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 68, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 7, 114, 0, 16, 0, 2, 0, 0, 0, 246, 15, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 8, 34, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 16, 0, 0, 8, 66, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 0, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 7, 0, 0, 0, 56, 0, 0, 7, 34, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 178, 0, 16, 0, 0, 0, 0, 0, 6, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 4, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 5, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 5, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 2, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 5, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 4, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 5, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 246, 15, 16, 0, 4, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 4, 0, 0, 0, 246, 15, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 4, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 70, 2, 16, 0, 4, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 2, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 2, 0, 0, 0, 50, 0, 0, 11, 34, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 3, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 3, 0, 0, 0, 50, 0, 0, 10, 34, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 3, 0, 0, 0, 26, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 34, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 26, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 34, 0, 16, 0, 2, 0, 0, 0, 26, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 2, 0, 0, 0, 42, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 1, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 42, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 1, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 16, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 17, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 17, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 210, 0, 16, 0, 2, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 6, 137, 32, 0, 2, 0, 0, 0, 17, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 5, 0, 0, 0, 134, 3, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 3, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 14, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 6, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 12, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 6, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 6, 0, 0, 0, 246, 15, 16, 0, 5, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 5, 0, 0, 0, 246, 15, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 134, 3, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 16, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 14, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 14, 0, 0, 0, 50, 0, 0, 11, 66, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 15, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 15, 0, 0, 0, 50, 0, 0, 10, 66, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 15, 0, 0, 0, 42, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 14, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 66, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 42, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 2, 0, 0, 0, 42, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 13, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 13, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 13, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 22, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 22, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 23, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 23, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 23, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 7, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 20, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 8, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 18, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 246, 15, 16, 0, 3, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 7, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 7, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 7, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 22, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 20, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 20, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 21, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 21, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 21, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 20, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 19, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 7, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 19, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 19, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 28, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 28, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 29, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 29, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 29, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 8, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 26, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 9, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 24, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 246, 15, 16, 0, 4, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 8, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 28, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 26, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 26, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 27, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 27, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 27, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 26, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 3, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 4, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 3, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 25, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 25, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 25, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 34, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 34, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 35, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 35, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 35, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 9, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 32, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 10, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 30, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 246, 15, 16, 0, 5, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 9, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 34, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 32, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 32, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 33, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 33, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 33, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 32, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 4, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 5, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 4, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 31, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 31, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 31, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 40, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 40, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 41, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 41, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 41, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 10, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 38, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 38, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 11, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 36, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 5, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 11, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 246, 15, 16, 0, 6, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 10, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 40, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 38, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 38, 0, 0, 0, 50, 0, 0, 11, 18, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 39, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 39, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 39, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 38, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 5, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 5, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 37, 0, 0, 0, 16, 32, 0, 7, 18, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 18, 0, 16, 0, 6, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 37, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 37, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 46, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 46, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 47, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 47, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 47, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 11, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 44, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 44, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 12, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 42, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 12, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 246, 15, 16, 0, 7, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 11, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 46, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 44, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 44, 0, 0, 0, 50, 0, 0, 11, 18, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 45, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 45, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 45, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 44, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 6, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 8, 18, 0, 16, 0, 6, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 43, 0, 0, 0, 16, 32, 0, 7, 34, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 34, 0, 16, 0, 6, 0, 0, 0, 26, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 43, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 26, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 43, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 52, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 52, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 53, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 53, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 226, 0, 16, 0, 6, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 6, 137, 32, 0, 2, 0, 0, 0, 53, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 12, 0, 0, 0, 150, 7, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 50, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 13, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 48, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 7, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 13, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 246, 15, 16, 0, 8, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 12, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 150, 7, 16, 128, 65, 0, 0, 0, 6, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 52, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 50, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 50, 0, 0, 0, 50, 0, 0, 11, 34, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 51, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 51, 0, 0, 0, 50, 0, 0, 10, 34, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 51, 0, 0, 0, 26, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 50, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 34, 0, 16, 0, 6, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 26, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 8, 34, 0, 16, 0, 6, 0, 0, 0, 26, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 49, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 6, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 49, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 49, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 58, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 59, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 59, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 59, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 14, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 56, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 56, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 15, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 54, 0, 0, 0, 16, 0, 0, 7, 66, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 6, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 15, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 246, 15, 16, 0, 6, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 14, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 70, 2, 16, 0, 14, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 14, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 56, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 56, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 57, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 57, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 6, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 57, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 56, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 66, 0, 16, 0, 6, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 6, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 55, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 14, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 55, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 55, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 64, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 64, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 65, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 65, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 65, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 15, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 62, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 62, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 16, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 60, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 6, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 16, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 246, 15, 16, 0, 7, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 15, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 64, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 62, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 62, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 63, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 63, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 63, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 62, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 6, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 7, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 6, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 61, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 7, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 61, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 61, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 70, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 70, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 71, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 71, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 71, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 16, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 68, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 68, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 17, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 66, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 7, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 17, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 246, 15, 16, 0, 8, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 16, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 70, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 68, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 68, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 69, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 69, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 69, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 68, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 7, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 8, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 7, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 67, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 67, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 67, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 76, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 76, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 77, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 77, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 77, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 17, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 74, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 74, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 18, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 72, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 8, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 9, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 18, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 246, 15, 16, 0, 9, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 17, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 76, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 74, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 74, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 9, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 75, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 75, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 9, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 75, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 74, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 8, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 9, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 8, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 73, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 9, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 73, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 73, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 82, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 82, 0, 0, 0, 16, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 83, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 83, 0, 0, 0, 68, 0, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 83, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 18, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 80, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 19, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 78, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 9, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 19, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 246, 15, 16, 0, 10, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 18, 0, 0, 0, 6, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 16, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 0, 0, 0, 9, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 82, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 128, 65, 0, 0, 0, 2, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 50, 0, 0, 11, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 80, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 80, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 81, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 81, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 81, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 80, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 9, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 10, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 9, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 79, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 79, 0, 0, 0, 56, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 79, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 16, 0, 0, 9, 130, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 89, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 89, 0, 0, 0, 68, 0, 0, 5, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 89, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 19, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 86, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 1, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 86, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 20, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 84, 0, 0, 0, 16, 0, 0, 7, 18, 0, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 10, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 20, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 246, 15, 16, 0, 10, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 19, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 85, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 86, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 86, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 87, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 87, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 10, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 87, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 50, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 86, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 10, 0, 16, 0, 2, 0, 0, 0, 54, 32, 0, 5, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 14, 0, 0, 10, 18, 0, 16, 0, 2, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 10, 0, 0, 0, 56, 0, 0, 8, 18, 0, 16, 0, 2, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 85, 0, 0, 0, 16, 0, 0, 8, 130, 0, 16, 0, 10, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 88, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 11, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 88, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 88, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 128, 65, 0, 0, 0, 10, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 85, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 16, 0, 0, 9, 130, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 95, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 95, 0, 0, 0, 68, 0, 0, 5, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 13, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 95, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 20, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 1, 0, 0, 0, 58, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 92, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 0, 0, 0, 9, 114, 0, 16, 0, 21, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 90, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 10, 0, 0, 0, 70, 2, 16, 0, 21, 0, 0, 0, 70, 2, 16, 0, 21, 0, 0, 0, 75, 0, 0, 5, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 14, 0, 0, 7, 114, 0, 16, 0, 21, 0, 0, 0, 70, 2, 16, 0, 21, 0, 0, 0, 246, 15, 16, 0, 11, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 20, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 21, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 91, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 12, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 92, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 92, 0, 0, 0, 50, 0, 0, 11, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 93, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 93, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 93, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 50, 0, 0, 10, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 42, 128, 32, 0, 2, 0, 0, 0, 92, 0, 0, 0, 58, 0, 16, 0, 12, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 10, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 10, 0, 0, 0, 54, 32, 0, 5, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 14, 0, 0, 10, 130, 0, 16, 0, 10, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 58, 0, 16, 0, 11, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 10, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 58, 128, 32, 0, 2, 0, 0, 0, 91, 0, 0, 0, 16, 0, 0, 8, 130, 0, 16, 0, 11, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 0, 0, 0, 9, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 94, 0, 0, 0, 0, 0, 0, 10, 130, 0, 16, 0, 12, 0, 0, 0, 10, 128, 32, 128, 65, 0, 0, 0, 2, 0, 0, 0, 94, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 94, 0, 0, 0, 14, 0, 0, 8, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 128, 65, 0, 0, 0, 11, 0, 0, 0, 58, 0, 16, 0, 12, 0, 0, 0, 0, 32, 0, 7, 130, 0, 16, 0, 11, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 11, 0, 0, 0, 50, 0, 0, 10, 178, 0, 16, 0, 0, 0, 0, 0, 70, 136, 32, 0, 2, 0, 0, 0, 91, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 114, 0, 16, 0, 13, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 4, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 56, 0, 0, 9, 114, 0, 16, 0, 21, 0, 0, 0, 246, 143, 32, 0, 2, 0, 0, 0, 96, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 96, 0, 0, 0, 50, 32, 0, 9, 178, 0, 16, 0, 0, 0, 0, 0, 70, 8, 16, 0, 21, 0, 0, 0, 70, 8, 16, 0, 13, 0, 0, 0, 70, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 9, 114, 0, 16, 0, 13, 0, 0, 0, 70, 18, 16, 128, 65, 0, 0, 0, 4, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 98, 0, 0, 0, 16, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 68, 0, 0, 5, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 7, 114, 0, 16, 0, 13, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 13, 0, 0, 0, 16, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 0, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 50, 0, 0, 11, 114, 0, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 246, 15, 16, 128, 65, 0, 0, 0, 1, 0, 0, 0, 70, 2, 16, 128, 65, 0, 0, 0, 13, 0, 0, 0, 16, 32, 0, 7, 18, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 47, 0, 0, 5, 18, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 1, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 18, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 10, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 114, 0, 16, 0, 1, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 7, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 4, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 1, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 5, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 13, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 7, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 19, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 8, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 3, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 25, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 9, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 4, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 31, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 10, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 5, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 37, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 11, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 43, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 12, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 49, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 14, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 55, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 15, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 6, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 61, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 16, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 7, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 67, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 17, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 8, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 73, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 18, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 9, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 79, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 16, 32, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 19, 0, 0, 0, 16, 32, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 3, 0, 0, 0, 70, 2, 16, 0, 20, 0, 0, 0, 47, 0, 0, 5, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 56, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 130, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 1, 0, 0, 0, 58, 0, 16, 0, 10, 0, 0, 0, 47, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 26, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 25, 0, 0, 5, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 56, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 128, 32, 0, 2, 0, 0, 0, 97, 0, 0, 0, 56, 0, 0, 7, 66, 0, 16, 0, 0, 0, 0, 0, 42, 0, 16, 0, 0, 0, 0, 0, 10, 0, 16, 0, 2, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 85, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 50, 0, 0, 10, 114, 0, 16, 0, 1, 0, 0, 0, 70, 130, 32, 0, 2, 0, 0, 0, 91, 0, 0, 0, 246, 15, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 114, 0, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 3, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 56, 0, 0, 7, 114, 0, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 242, 0, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 0, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 56, 0, 0, 7, 242, 0, 16, 0, 2, 0, 0, 0, 70, 14, 16, 0, 2, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 50, 0, 0, 9, 114, 0, 16, 0, 0, 0, 0, 0, 70, 3, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 2, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 130, 32, 16, 0, 0, 0, 0, 0, 58, 0, 16, 0, 2, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 114, 0, 16, 0, 1, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 5, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 0, 0, 0, 7, 114, 32, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 79, 2, 0, 0, 22, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 55, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__default";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = RHIDevice::CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::move(std::make_unique<ShaderProgram>(std::move(desc)));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultUnlitShaderProgram() noexcept {
#if 0
    std::string program =
    R"(

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);


ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {
    float4 albedo = tDiffuse.Sample(sSampler, input_pixel.uv);
    return albedo * input_pixel.color;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1388> g_VertexFunction{68, 88, 66, 67, 154, 212, 131, 86, 33, 141, 207, 3, 214, 139, 162, 196, 200, 132, 245, 217, 1, 0, 0, 0, 108, 5, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 168, 1, 0, 0, 20, 2, 0, 0, 132, 2, 0, 0, 208, 4, 0, 0, 82, 68, 69, 70, 108, 1, 0, 0, 1, 0, 0, 0, 104, 0, 0, 0, 1, 0, 0, 0, 60, 0, 0, 0, 0, 5, 254, 255, 16, 129, 4, 0, 68, 1, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 171, 171, 92, 0, 0, 0, 3, 0, 0, 0, 128, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 48, 1, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 55, 1, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 100, 0, 0, 0, 3, 0, 0, 0, 8, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 89, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 171, 171, 79, 83, 71, 78, 104, 0, 0, 0, 3, 0, 0, 0, 8, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 98, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 12, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 171, 171, 171, 83, 72, 69, 88, 68, 2, 0, 0, 80, 0, 1, 0, 145, 0, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 12, 0, 0, 0, 95, 0, 0, 3, 114, 16, 16, 0, 0, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 95, 0, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 103, 0, 0, 4, 242, 32, 16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 1, 0, 0, 0, 101, 0, 0, 3, 50, 32, 16, 0, 2, 0, 0, 0, 104, 0, 0, 2, 2, 0, 0, 0, 54, 0, 0, 5, 114, 0, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 17, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 4, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 5, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 6, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 7, 0, 0, 0, 17, 0, 0, 8, 18, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 8, 0, 0, 0, 17, 0, 0, 8, 34, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 8, 66, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 10, 0, 0, 0, 17, 0, 0, 8, 130, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 11, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 50, 32, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 17, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 732> g_PixelFunction{68, 88, 66, 67, 159, 89, 228, 8, 27, 100, 31, 188, 127, 130, 159, 32, 197, 80, 105, 3, 1, 0, 0, 0, 220, 2, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 244, 0, 0, 0, 100, 1, 0, 0, 152, 1, 0, 0, 64, 2, 0, 0, 82, 68, 69, 70, 184, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 60, 0, 0, 0, 0, 5, 255, 255, 16, 129, 4, 0, 142, 0, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 124, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 133, 0, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 115, 83, 97, 109, 112, 108, 101, 114, 0, 116, 68, 105, 102, 102, 117, 115, 101, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 171, 171, 73, 83, 71, 78, 104, 0, 0, 0, 3, 0, 0, 0, 8, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 98, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 171, 171, 171, 79, 83, 71, 78, 44, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 84, 97, 114, 103, 101, 116, 0, 171, 171, 83, 72, 69, 88, 160, 0, 0, 0, 80, 0, 0, 0, 40, 0, 0, 0, 106, 8, 0, 1, 90, 0, 0, 3, 0, 96, 16, 0, 0, 0, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 0, 0, 0, 0, 85, 85, 0, 0, 98, 16, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 98, 16, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 0, 0, 0, 0, 104, 0, 0, 2, 1, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 242, 0, 16, 0, 0, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 0, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 56, 0, 0, 7, 242, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__unlit";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultNormalShaderProgram() noexcept {
#if 0
    std::string program =
    R"(

float3 NormalAsColor(float3 n) {
    return ((n + 1.0f) * 0.5f);
}

float3 ColorAsNormal(float3 color) {
    return ((color * 2.0f) - 1.0f);
}

float RangeMap(float valueToMap, float minInputRange, float maxInputRange, float minOutputRange, float maxOutputRange) {
    return (valueToMap - minInputRange) * (maxOutputRange - minOutputRange) / (maxInputRange - minInputRange) + minOutputRange;
}

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

struct light {
    float4 position;
    float4 color;
    float4 attenuation;
    float4 specAttenuation;
    float4 innerOuterDotThresholds;
    float4 direction;
};

cbuffer lighting_cb : register(b2) {
    light g_Lights[16];
    float4 g_lightAmbient;
    float4 g_lightSpecGlossEmitFactors;
    float4 g_lightEyePosition;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
    float3 world_position : WORLD;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 normal = input_vertex.normal;
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;
    output.normal = normal;
    output.world_position = world.xyz;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {

    float2 uv = input_pixel.uv;
    float4 albedo = tDiffuse.Sample(sSampler, uv);
    float4 tinted_color = albedo * input_pixel.color;

    float3 normal_as_color = NormalAsColor(input_pixel.normal.xyz);

    float3 final_color = normal_as_color;
    float final_alpha = 1.0f;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1556> g_VertexFunction{uint8_t{68}, 88, 66, 67, 180, 142, 65, 18, 203, 129, 160, 1, 80, 73, 136, 88, 162, 78, 9, 248, 1, 0, 0, 0, 20, 6, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 168, 1, 0, 0, 52, 2, 0, 0, 224, 2, 0, 0, 120, 5, 0, 0, 82, 68, 69, 70, 108, 1, 0, 0, 1, 0, 0, 0, 104, 0, 0, 0, 1, 0, 0, 0, 60, 0, 0, 0, 0, 5, 254, 255, 16, 129, 4, 0, 68, 1, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 171, 171, 92, 0, 0, 0, 3, 0, 0, 0, 128, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 48, 1, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 55, 1, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 132, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 104, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 119, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 15, 0, 0, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 171, 171, 171, 79, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 12, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 0, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 8, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 83, 72, 69, 88, 144, 2, 0, 0, 80, 0, 1, 0, 164, 0, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 12, 0, 0, 0, 95, 0, 0, 3, 114, 16, 16, 0, 0, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 95, 0, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 3, 0, 0, 0, 103, 0, 0, 4, 242, 32, 16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 1, 0, 0, 0, 101, 0, 0, 3, 50, 32, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 3, 0, 0, 0, 101, 0, 0, 3, 114, 32, 16, 0, 4, 0, 0, 0, 104, 0, 0, 2, 2, 0, 0, 0, 54, 0, 0, 5, 114, 0, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 17, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 4, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 5, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 6, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 7, 0, 0, 0, 54, 0, 0, 5, 114, 32, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 17, 0, 0, 8, 18, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 8, 0, 0, 0, 17, 0, 0, 8, 34, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 8, 66, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 10, 0, 0, 0, 17, 0, 0, 8, 130, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 11, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 50, 32, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 3, 0, 0, 0, 70, 30, 16, 0, 3, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 19, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 668> g_PixelFunction{68, 88, 66, 67, 119, 155, 61, 161, 30, 214, 151, 236, 255, 45, 21, 134, 144, 143, 18, 3, 1, 0, 0, 0, 156, 2, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 160, 0, 0, 0, 76, 1, 0, 0, 128, 1, 0, 0, 0, 2, 0, 0, 82, 68, 69, 70, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 0, 0, 0, 0, 5, 255, 255, 16, 129, 4, 0, 60, 0, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 7, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 0, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 79, 83, 71, 78, 44, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 84, 97, 114, 103, 101, 116, 0, 171, 171, 83, 72, 69, 88, 120, 0, 0, 0, 80, 0, 0, 0, 30, 0, 0, 0, 106, 8, 0, 1, 98, 16, 0, 3, 114, 16, 16, 0, 3, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 0, 0, 0, 0, 50, 0, 0, 15, 114, 32, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 3, 0, 0, 0, 2, 64, 0, 0, 0, 0, 0, 63, 0, 0, 0, 63, 0, 0, 0, 63, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 0, 63, 0, 0, 0, 63, 0, 0, 0, 63, 0, 0, 0, 0, 54, 0, 0, 5, 130, 32, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__normal";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultNormalMapShaderProgram() noexcept {
#if 0
std::string program =
    R"(

float3 NormalAsColor(float3 n) {
    return ((n + 1.0f) * 0.5f);
}

float3 ColorAsNormal(float3 color) {
    return ((color * 2.0f) - 1.0f);
}

float RangeMap(float valueToMap, float minInputRange, float maxInputRange, float minOutputRange, float maxOutputRange) {
    return (valueToMap - minInputRange) * (maxOutputRange - minOutputRange) / (maxInputRange - minInputRange) + minOutputRange;
}

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

struct light {
    float4 position;
    float4 color;
    float4 attenuation;
    float4 specAttenuation;
    float4 innerOuterDotThresholds;
    float4 direction;
};

cbuffer lighting_cb : register(b2) {
    light g_Lights[16];
    float4 g_lightAmbient;
    float4 g_lightSpecGlossEmitFactors;
    float4 g_lightEyePosition;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 normal : NORMAL;
    float3 world_position : WORLD;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 normal = input_vertex.normal;
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;
    output.normal = normal;
    output.world_position = world.xyz;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {

    float2 uv = input_pixel.uv;
    float4 albedo = tDiffuse.Sample(sSampler, uv);
    float4 tinted_color = albedo * input_pixel.color;

    float3 normal_as_color = tNormal.Sample(sSampler, uv).rgb;

    float3 final_color = normal_as_color;
    float final_alpha = 1.0f;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1556> g_VertexFunction{68, 88, 66, 67, 180, 142, 65, 18, 203, 129, 160, 1, 80, 73, 136, 88, 162, 78, 9, 248, 1, 0, 0, 0, 20, 6, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 168, 1, 0, 0, 52, 2, 0, 0, 224, 2, 0, 0, 120, 5, 0, 0, 82, 68, 69, 70, 108, 1, 0, 0, 1, 0, 0, 0, 104, 0, 0, 0, 1, 0, 0, 0, 60, 0, 0, 0, 0, 5, 254, 255, 16, 129, 4, 0, 68, 1, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 171, 171, 92, 0, 0, 0, 3, 0, 0, 0, 128, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 48, 1, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 55, 1, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 12, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 132, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 104, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 119, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 15, 0, 0, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 171, 171, 171, 79, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 12, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 0, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 8, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 83, 72, 69, 88, 144, 2, 0, 0, 80, 0, 1, 0, 164, 0, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 12, 0, 0, 0, 95, 0, 0, 3, 114, 16, 16, 0, 0, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 95, 0, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 3, 0, 0, 0, 103, 0, 0, 4, 242, 32, 16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 1, 0, 0, 0, 101, 0, 0, 3, 50, 32, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 3, 0, 0, 0, 101, 0, 0, 3, 114, 32, 16, 0, 4, 0, 0, 0, 104, 0, 0, 2, 2, 0, 0, 0, 54, 0, 0, 5, 114, 0, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 17, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 4, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 5, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 6, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 7, 0, 0, 0, 54, 0, 0, 5, 114, 32, 16, 0, 4, 0, 0, 0, 70, 2, 16, 0, 1, 0, 0, 0, 17, 0, 0, 8, 18, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 8, 0, 0, 0, 17, 0, 0, 8, 34, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 8, 66, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 10, 0, 0, 0, 17, 0, 0, 8, 130, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 11, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 50, 32, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 3, 0, 0, 0, 70, 30, 16, 0, 3, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 19, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 792> g_PixelFunction{68, 88, 66, 67, 62, 134, 9, 188, 80, 172, 86, 87, 207, 97, 24, 49, 200, 104, 254, 54, 1, 0, 0, 0, 24, 3, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 244, 0, 0, 0, 160, 1, 0, 0, 212, 1, 0, 0, 124, 2, 0, 0, 82, 68, 69, 70, 184, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 60, 0, 0, 0, 0, 5, 255, 255, 16, 129, 4, 0, 141, 0, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 124, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 133, 0, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 1, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 115, 83, 97, 109, 112, 108, 101, 114, 0, 116, 78, 111, 114, 109, 97, 108, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 171, 171, 171, 73, 83, 71, 78, 164, 0, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 146, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 0, 0, 0, 156, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7, 0, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 78, 79, 82, 77, 65, 76, 0, 87, 79, 82, 76, 68, 0, 171, 171, 79, 83, 71, 78, 44, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 84, 97, 114, 103, 101, 116, 0, 171, 171, 83, 72, 69, 88, 160, 0, 0, 0, 80, 0, 0, 0, 40, 0, 0, 0, 106, 8, 0, 1, 90, 0, 0, 3, 0, 96, 16, 0, 0, 0, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 1, 0, 0, 0, 85, 85, 0, 0, 98, 16, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 0, 0, 0, 0, 104, 0, 0, 2, 1, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 114, 0, 16, 0, 0, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 1, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 114, 32, 16, 0, 0, 0, 0, 0, 70, 2, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 32, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__normalmap";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultFontShaderProgram() noexcept {
#if 0
    std::string program =
    R"(

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

cbuffer font_cb : register(b3) {
    float4 g_font_channel;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    float4 channel_id : CHANNEL_ID;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;
    output.channel_id = g_font_channel;
    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {

    float2 uv = input_pixel.uv;
    float4 chnl = input_pixel.channel_id;
    float4 albedo = tDiffuse.Sample(sSampler, uv);
    if(dot(float4(1.0f, 1.0f, 1.0f, 1.0f), chnl)) {
        float val = dot(albedo, chnl);
        albedo.rgb = val > 0.5 ? 2 * val - 1 : 0;
        albedo.a = val > 0.5 ? 1 : 2 * val;
    }
    float3 tinted_color = albedo.rgb * input_pixel.color.rgb;
    float tinted_alpha = albedo.a * input_pixel.color.a;
    float3 final_color = tinted_color;
    float final_alpha = tinted_alpha;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    std::array<uint8_t, 1636> g_VertexFunction{68, 88, 66, 67, 183, 113, 13, 44, 238, 130, 203, 8, 92, 76, 41, 48, 178, 116, 211, 32, 1, 0, 0, 0, 100, 6, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 76, 2, 0, 0, 184, 2, 0, 0, 72, 3, 0, 0, 200, 5, 0, 0, 82, 68, 69, 70, 16, 2, 0, 0, 2, 0, 0, 0, 144, 0, 0, 0, 2, 0, 0, 0, 60, 0, 0, 0, 0, 5, 254, 255, 16, 129, 4, 0, 232, 1, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 124, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 134, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 109, 97, 116, 114, 105, 120, 95, 99, 98, 0, 102, 111, 110, 116, 95, 99, 98, 0, 171, 171, 124, 0, 0, 0, 3, 0, 0, 0, 192, 0, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 134, 0, 0, 0, 1, 0, 0, 0, 132, 1, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 56, 1, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 76, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 112, 1, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 76, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 119, 1, 0, 0, 128, 0, 0, 0, 64, 0, 0, 0, 2, 0, 0, 0, 76, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 77, 79, 68, 69, 76, 0, 102, 108, 111, 97, 116, 52, 120, 52, 0, 171, 171, 171, 3, 0, 3, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 1, 0, 0, 103, 95, 86, 73, 69, 87, 0, 103, 95, 80, 82, 79, 74, 69, 67, 84, 73, 79, 78, 0, 172, 1, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 196, 1, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 103, 95, 102, 111, 110, 116, 95, 99, 104, 97, 110, 110, 101, 108, 0, 102, 108, 111, 97, 116, 52, 0, 171, 171, 1, 0, 3, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 187, 1, 0, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 73, 83, 71, 78, 100, 0, 0, 0, 3, 0, 0, 0, 8, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 89, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 171, 171, 79, 83, 71, 78, 136, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 104, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 116, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 12, 0, 0, 125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 67, 72, 65, 78, 78, 69, 76, 95, 73, 68, 0, 83, 72, 69, 88, 120, 2, 0, 0, 80, 0, 1, 0, 158, 0, 0, 0, 106, 8, 0, 1, 89, 0, 0, 4, 70, 142, 32, 0, 0, 0, 0, 0, 12, 0, 0, 0, 89, 0, 0, 4, 70, 142, 32, 0, 3, 0, 0, 0, 1, 0, 0, 0, 95, 0, 0, 3, 114, 16, 16, 0, 0, 0, 0, 0, 95, 0, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 95, 0, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 103, 0, 0, 4, 242, 32, 16, 0, 0, 0, 0, 0, 1, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 1, 0, 0, 0, 101, 0, 0, 3, 50, 32, 16, 0, 2, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 3, 0, 0, 0, 104, 0, 0, 2, 2, 0, 0, 0, 54, 0, 0, 5, 114, 0, 16, 0, 0, 0, 0, 0, 70, 18, 16, 0, 0, 0, 0, 0, 54, 0, 0, 5, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 17, 0, 0, 8, 18, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 1, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 2, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 1, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 3, 0, 0, 0, 17, 0, 0, 8, 18, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 4, 0, 0, 0, 17, 0, 0, 8, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 5, 0, 0, 0, 17, 0, 0, 8, 66, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 6, 0, 0, 0, 17, 0, 0, 8, 130, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 7, 0, 0, 0, 17, 0, 0, 8, 18, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 8, 0, 0, 0, 17, 0, 0, 8, 34, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 8, 66, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 10, 0, 0, 0, 17, 0, 0, 8, 130, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 142, 32, 0, 0, 0, 0, 0, 11, 0, 0, 0, 54, 0, 0, 5, 242, 32, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 54, 0, 0, 5, 50, 32, 16, 0, 2, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 54, 0, 0, 6, 242, 32, 16, 0, 3, 0, 0, 0, 70, 142, 32, 0, 3, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 18, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    std::array<uint8_t, 1076> g_PixelFunction{68, 88, 66, 67, 52, 78, 224, 141, 56, 184, 43, 47, 169, 244, 92, 237, 221, 143, 62, 44, 1, 0, 0, 0, 52, 4, 0, 0, 5, 0, 0, 0, 52, 0, 0, 0, 244, 0, 0, 0, 132, 1, 0, 0, 184, 1, 0, 0, 152, 3, 0, 0, 82, 68, 69, 70, 184, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 60, 0, 0, 0, 0, 5, 255, 255, 16, 129, 4, 0, 142, 0, 0, 0, 82, 68, 49, 49, 60, 0, 0, 0, 24, 0, 0, 0, 32, 0, 0, 0, 40, 0, 0, 0, 36, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 124, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 133, 0, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0, 115, 83, 97, 109, 112, 108, 101, 114, 0, 116, 68, 105, 102, 102, 117, 115, 101, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 171, 171, 73, 83, 71, 78, 136, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 104, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 116, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 15, 15, 0, 0, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3, 0, 0, 125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 15, 15, 0, 0, 83, 86, 95, 80, 79, 83, 73, 84, 73, 79, 78, 0, 67, 79, 76, 79, 82, 0, 85, 86, 0, 67, 72, 65, 78, 78, 69, 76, 95, 73, 68, 0, 79, 83, 71, 78, 44, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 83, 86, 95, 84, 97, 114, 103, 101, 116, 0, 171, 171, 83, 72, 69, 88, 216, 1, 0, 0, 80, 0, 0, 0, 118, 0, 0, 0, 106, 8, 0, 1, 90, 0, 0, 3, 0, 96, 16, 0, 0, 0, 0, 0, 88, 24, 0, 4, 0, 112, 16, 0, 0, 0, 0, 0, 85, 85, 0, 0, 98, 16, 0, 3, 242, 16, 16, 0, 1, 0, 0, 0, 98, 16, 0, 3, 50, 16, 16, 0, 2, 0, 0, 0, 98, 16, 0, 3, 242, 16, 16, 0, 3, 0, 0, 0, 101, 0, 0, 3, 242, 32, 16, 0, 0, 0, 0, 0, 104, 0, 0, 2, 3, 0, 0, 0, 17, 0, 0, 10, 18, 0, 16, 0, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 128, 63, 70, 30, 16, 0, 3, 0, 0, 0, 57, 0, 0, 10, 18, 0, 16, 0, 0, 0, 0, 0, 2, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 16, 0, 0, 0, 0, 0, 69, 0, 0, 139, 194, 0, 0, 128, 67, 85, 21, 0, 242, 0, 16, 0, 1, 0, 0, 0, 70, 16, 16, 0, 2, 0, 0, 0, 70, 126, 16, 0, 0, 0, 0, 0, 0, 96, 16, 0, 0, 0, 0, 0, 17, 0, 0, 7, 34, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 70, 30, 16, 0, 3, 0, 0, 0, 50, 0, 0, 9, 66, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 0, 64, 1, 64, 0, 0, 0, 0, 128, 191, 49, 0, 0, 7, 130, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 0, 63, 26, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 7, 34, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 26, 0, 16, 0, 0, 0, 0, 0, 55, 0, 0, 9, 130, 0, 16, 0, 2, 0, 0, 0, 58, 0, 16, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 128, 63, 26, 0, 16, 0, 0, 0, 0, 0, 1, 0, 0, 7, 114, 0, 16, 0, 2, 0, 0, 0, 166, 10, 16, 0, 0, 0, 0, 0, 246, 15, 16, 0, 0, 0, 0, 0, 55, 0, 0, 9, 242, 0, 16, 0, 0, 0, 0, 0, 6, 0, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 2, 0, 0, 0, 70, 14, 16, 0, 1, 0, 0, 0, 56, 0, 0, 7, 242, 32, 16, 0, 0, 0, 0, 0, 70, 14, 16, 0, 0, 0, 0, 0, 70, 30, 16, 0, 1, 0, 0, 0, 62, 0, 0, 1, 83, 84, 65, 84, 148, 0, 0, 0, 12, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__font";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultCircle2DShaderProgram() noexcept {
#if 0
    std::string program =
        R"(

cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 thickness_fade : UV;
};

struct ps_in_t {
    float4 local_position : SV_POSITION;
    float4 world_position : WORLD_POSITION;
    float4 color : COLOR;
    float2 thickness_fade : UV;
};

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.local_position = local;
    output.world_position = world;
    output.color = input_vertex.color;
    output.thickness_fade = input_vertex.thickness_fade;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0{

    float4 lp = input_pixel.local_position;
    float4 wp = input_pixel.world_position;
    float4 color = input_pixel.color;

    float thickness = input_pixel.thickness_fade.x;
    float fade = input_pixel.thickness_fade.y;
    float distance = 1.0f - length(float2(lp.x, lp.y));

    float alpha = smoothstep(0.0f, fade, distance);
    alpha = smoothstep(thickness + fade, thickness, distance);

    float3 tinted_color = color.rgb;
    float tinted_alpha = color.a * alpha;
    float3 final_color = tinted_color;
    float final_alpha = tinted_alpha;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}

)";
#endif

#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1200> g_VertexFunction{0x44, 0x58, 0x42, 0x43, 0xE8, 0x46, 0xF1, 0xCC, 0xAD, 0x98, 0xF2, 0xA9, 0x83, 0xFB, 0x09, 0x11, 0xE2, 0x4F, 0xB8, 0x83, 0x01, 0x00, 0x00, 0x00, 0xB0, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0xA8, 0x01, 0x00, 0x00, 0x14, 0x02, 0x00, 0x00, 0xA8, 0x02, 0x00, 0x00, 0x14, 0x04, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x6C, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFE, 0xFF, 0x12, 0x81, 0x04, 0x00, 0x44, 0x01, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x5F, 0x63, 0x62, 0x00, 0xAB, 0xAB, 0x5C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x37, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x67, 0x5F, 0x4D, 0x4F, 0x44, 0x45, 0x4C, 0x00, 0x66, 0x6C, 0x6F, 0x61, 0x74, 0x34, 0x78, 0x34, 0x00, 0xAB, 0xAB, 0xAB, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x67, 0x5F, 0x56, 0x49, 0x45, 0x57, 0x00, 0x67, 0x5F, 0x50, 0x52, 0x4F, 0x4A, 0x45, 0x43, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4E, 0x64, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0xAB, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x8C, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x57, 0x4F, 0x52, 0x4C, 0x44, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0x53, 0x48, 0x45, 0x58, 0x64, 0x01, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00, 0x59, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x59, 0x00, 0x00, 0x04, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x72, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x04, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0x32, 0x20, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x12, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x82, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x12, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x22, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x42, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x82, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x32, 0x20, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 980> g_PixelFunction{0x44, 0x58, 0x42, 0x43, 0x49, 0xC0, 0x90, 0x78, 0x3D, 0x2A, 0xFF, 0x59, 0xA9, 0x36, 0x34, 0x39, 0xBF, 0x99, 0x8B, 0xEC, 0x01, 0x00, 0x00, 0x00, 0xD4, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x34, 0x01, 0x00, 0x00, 0x68, 0x01, 0x00, 0x00, 0x38, 0x03, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF, 0x12, 0x81, 0x04, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4E, 0x8C, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x03, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x57, 0x4F, 0x52, 0x4C, 0x44, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0xC8, 0x01, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x64, 0x20, 0x00, 0x04, 0x32, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x07, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4B, 0x00, 0x00, 0x05, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x80, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x07, 0x22, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0A, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x10, 0x80, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x22, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x10, 0x80, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x0A, 0x22, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x1A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x20, 0x00, 0x07, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x09, 0x22, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x38, 0x00, 0x00, 0x07, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x07, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x07, 0x82, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x12, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__circle2d";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

std::unique_ptr<ShaderProgram> Renderer::CreateDefaultWebPShaderProgram() noexcept {
#pragma region g_VertexFunction Byte Code
    static constexpr const std::array<uint8_t, 1528> g_VertexFunction{0x44, 0x58, 0x42, 0x43, 0x8E, 0x0E, 0xEB, 0x4C, 0x4C, 0x79, 0x6E, 0x2E, 0x5E, 0x9E, 0xAD, 0x19, 0x01, 0x32, 0x5D, 0x9D, 0x01, 0x00, 0x00, 0x00, 0xF8, 0x05, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0xA8, 0x01, 0x00, 0x00, 0x54, 0x02, 0x00, 0x00, 0xE4, 0x02, 0x00, 0x00, 0x5C, 0x05, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x6C, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFE, 0xFF, 0x12, 0x81, 0x04, 0x00, 0x44, 0x01, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x5F, 0x63, 0x62, 0x00, 0xAB, 0xAB, 0x5C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x37, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x67, 0x5F, 0x4D, 0x4F, 0x44, 0x45, 0x4C, 0x00, 0x66, 0x6C, 0x6F, 0x61, 0x74, 0x34, 0x78, 0x34, 0x00, 0xAB, 0xAB, 0xAB, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x67, 0x5F, 0x56, 0x49, 0x45, 0x57, 0x00, 0x67, 0x5F, 0x50, 0x52, 0x4F, 0x4A, 0x45, 0x43, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4E, 0xA4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0x4E, 0x4F, 0x52, 0x4D, 0x41, 0x4C, 0x00, 0x54, 0x41, 0x4E, 0x47, 0x45, 0x4E, 0x54, 0x00, 0xAB, 0xAB, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x88, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x00, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0x54, 0x41, 0x4E, 0x47, 0x45, 0x4E, 0x54, 0x00, 0xAB, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x70, 0x02, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00, 0x9C, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x59, 0x00, 0x00, 0x04, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x72, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x72, 0x10, 0x10, 0x00, 0x04, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x04, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0x32, 0x20, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0x72, 0x20, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x12, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x82, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x11, 0x00, 0x00, 0x08, 0x12, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x22, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x42, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x82, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x22, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x42, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x82, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x12, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x22, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x42, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x82, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x32, 0x20, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72, 0x20, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x46, 0x12, 0x10, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#pragma endregion
#pragma region g_PixelFunction Byte Code
    static constexpr const std::array<uint8_t, 804> g_PixelFunction{0x44, 0x58, 0x42, 0x43, 0xA5, 0xFB, 0xEF, 0x80, 0x73, 0x8D, 0xB7, 0x25, 0x92, 0x66, 0x1E, 0xDC, 0x46, 0x9B, 0xDB, 0x2E, 0x01, 0x00, 0x00, 0x00, 0x24, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00, 0x84, 0x01, 0x00, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x88, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF, 0x12, 0x81, 0x04, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x73, 0x53, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x72, 0x00, 0x74, 0x44, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB, 0xAB, 0x49, 0x53, 0x47, 0x4E, 0x88, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x55, 0x56, 0x00, 0x54, 0x41, 0x4E, 0x47, 0x45, 0x4E, 0x54, 0x00, 0xAB, 0xAB, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0xC8, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x5A, 0x00, 0x00, 0x03, 0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x40, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x32, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x42, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x48, 0x42, 0x45, 0x00, 0x00, 0x8B, 0x02, 0x02, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00, 0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x7E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x07, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#pragma endregion

    ShaderProgramDesc desc{};
    desc.name = "__webp";
    {
        ID3D11VertexShader* vs = nullptr;
        m_rhi_device->GetDxDevice()->CreateVertexShader(g_VertexFunction.data(), g_VertexFunction.size(), nullptr, &vs);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_VertexFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_VertexFunction.data(), g_VertexFunction.size());
        desc.vs = vs;
        desc.vs_bytecode = blob;
        desc.input_layout = m_rhi_device->CreateInputLayoutFromByteCode(*GetDevice(), blob);
    }
    {
        ID3D11PixelShader* ps = nullptr;
        m_rhi_device->GetDxDevice()->CreatePixelShader(g_PixelFunction.data(), g_PixelFunction.size(), nullptr, &ps);
        ID3DBlob* blob = nullptr;
        ::D3DCreateBlob(g_PixelFunction.size(), &blob);
        std::memcpy(blob->GetBufferPointer(), g_PixelFunction.data(), g_PixelFunction.size());
        desc.ps = ps;
        desc.ps_bytecode = blob;
    }
    return std::make_unique<ShaderProgram>(std::move(desc));
}

void Renderer::CreateAndRegisterDefaultMaterials() noexcept {
    auto default_mat = CreateDefaultMaterial();
    auto name = default_mat->GetName();
    RegisterMaterial(name, std::move(default_mat));

    auto unlit_mat = CreateDefaultUnlitMaterial();
    name = unlit_mat->GetName();
    RegisterMaterial(name, std::move(unlit_mat));

    auto mat_2d = CreateDefault2DMaterial();
    name = mat_2d->GetName();
    RegisterMaterial(name, std::move(mat_2d));

    auto mat_norm = CreateDefaultNormalMaterial();
    name = mat_norm->GetName();
    RegisterMaterial(name, std::move(mat_norm));

    auto mat_normmap = CreateDefaultNormalMapMaterial();
    name = mat_normmap->GetName();
    RegisterMaterial(name, std::move(mat_normmap));

    auto mat_invalid = CreateDefaultInvalidMaterial();
    name = mat_invalid->GetName();
    RegisterMaterial(name, std::move(mat_invalid));

    auto mat_circle2d = CreateDefaultCircle2DMaterial();
    name = mat_circle2d->GetName();
    RegisterMaterial(name, std::move(mat_circle2d));

    auto mat_webp = CreateDefaultWebPMaterial();
    name = mat_webp->GetName();
    RegisterMaterial(name, std::move(mat_webp));
}

std::unique_ptr<Material> Renderer::CreateDefaultMaterial() noexcept {
    std::string material =
    R"(
<material name="__default">
    <shader src="__default" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefaultUnlitMaterial() noexcept {
    std::string material =
    R"(
<material name="__unlit">
    <shader src="__unlit" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefault2DMaterial() noexcept {
    std::string material =
    R"(
<material name="__2D">
    <shader src="__2D" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefaultNormalMaterial() noexcept {
    std::string material =
    R"(
<material name="__normal">
    <shader src="__normal" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefaultNormalMapMaterial() noexcept {
    std::string material =
    R"(
<material name="__normalmap">
    <shader src="__normalmap" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefaultInvalidMaterial() noexcept {
    std::string material =
    R"(
<material name="__invalid">
    <shader src="__invalid" />
    <textures>
        <diffuse src="__invalid" />
    </textures>
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

std::unique_ptr<Material> Renderer::CreateDefaultCircle2DMaterial() noexcept {
    std::string material =
    R"(
<material name="__circle2d">
    <shader src="__circle2d" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    auto mat = std::make_unique<Material>(*doc.RootElement());
    mat->ClearAllTextureSlots();
    return mat;
}

[[nodiscard]] std::unique_ptr<Material> Renderer::CreateDefaultWebPMaterial() noexcept {
    std::string material =
    R"(
<material name="__webp">
    <shader src="__webp" />
</material>
)";

    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(material.c_str(), material.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    auto mat = std::make_unique<Material>(*doc.RootElement());
    mat->ClearAllTextureSlots();
    return mat;
}

std::unique_ptr<Material> Renderer::CreateMaterialFromFont(KerningFont* font) noexcept {
    if(font == nullptr) {
        return nullptr;
    }
    namespace FS = std::filesystem;
    FS::path folderpath = font->GetFilePath();
    folderpath = folderpath.parent_path();
    std::string name = font->GetName();
    std::string shader = "__font";
    std::ostringstream material_stream;
    material_stream << "<material name=\"Font_" << name << "\">\n";
    material_stream << "\t<shader src=\"" << shader << "\" />\n";
    std::size_t image_count = font->GetImagePaths().size();
    bool has_textures = image_count > 0;
    if(has_textures) {
        material_stream << "\t<textures>\n";
    }
    bool has_lots_of_textures = has_textures && image_count > 6;
    for(std::size_t i = 0; i < image_count; ++i) {
        FS::path image_path = font->GetImagePaths()[i];
        auto fullpath = folderpath / image_path;
        fullpath = FS::canonical(fullpath);
        fullpath.make_preferred();
        switch(i) {
        case 0: material_stream << "\t\t<diffuse src=" << fullpath << " />\n"; break;
        case 1: material_stream << "\t\t<normal src=" << fullpath << " />\n"; break;
        case 2: material_stream << "\t\t<lighting src=" << fullpath << " />\n"; break;
        case 3: material_stream << "\t\t<specular src=" << fullpath << " />\n"; break;
        case 4: material_stream << "\t\t<occlusion src=" << fullpath << " />\n"; break;
        case 5: material_stream << "\t\t<emissive src=" << fullpath << " />\n"; break;
        default: break;
        }
        if(i >= 6 && has_lots_of_textures) {
            material_stream << "\t\t<texture index=\"" << (i - 6) << "\" src=" << fullpath << " />\n";
        }
    }
    if(has_textures) {
        material_stream << "\t</textures>\n";
    }
    material_stream << "</material>\n";
    tinyxml2::XMLDocument doc;
    std::string material_string = material_stream.str();
    auto result = doc.Parse(material_string.c_str(), material_string.size());
    if(result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Material>(*doc.RootElement());
}

void Renderer::CreateAndRegisterDefaultSamplers() noexcept {
    auto default_sampler = CreateDefaultSampler();
    auto name = "__default";
    default_sampler->SetDebugName("__default_sampler");
    RegisterSampler(name, std::move(default_sampler));

    auto linear_sampler = CreateLinearSampler();
    name = "__linear";
    linear_sampler->SetDebugName("__linear_sampler");
    RegisterSampler(name, std::move(linear_sampler));

    auto point_sampler = CreatePointSampler();
    name = "__point";
    point_sampler->SetDebugName("__point_sampler");
    RegisterSampler(name, std::move(point_sampler));

    auto invalid_sampler = CreateInvalidSampler();
    name = "__invalid";
    invalid_sampler->SetDebugName("__invalid_sampler");
    RegisterSampler(name, std::move(invalid_sampler));
}

std::unique_ptr<Sampler> Renderer::CreateDefaultSampler() noexcept {
    return std::make_unique<Sampler>(m_rhi_device.get(), SamplerDesc{});
}

std::unique_ptr<Sampler> Renderer::CreateLinearSampler() noexcept {
    SamplerDesc desc{};
    desc.mag_filter = FilterMode::Linear;
    desc.min_filter = FilterMode::Linear;
    desc.mip_filter = FilterMode::Linear;
    return std::make_unique<Sampler>(m_rhi_device.get(), desc);
}

std::unique_ptr<Sampler> Renderer::CreatePointSampler() noexcept {
    SamplerDesc desc{};
    desc.mag_filter = FilterMode::Point;
    desc.min_filter = FilterMode::Point;
    desc.mip_filter = FilterMode::Point;
    return std::make_unique<Sampler>(m_rhi_device.get(), desc);
}

std::unique_ptr<Sampler> Renderer::CreateInvalidSampler() noexcept {
    SamplerDesc desc{};
    desc.mag_filter = FilterMode::Point;
    desc.min_filter = FilterMode::Point;
    desc.mip_filter = FilterMode::Point;
    desc.UaddressMode = TextureAddressMode::Wrap;
    desc.VaddressMode = TextureAddressMode::Wrap;
    desc.WaddressMode = TextureAddressMode::Wrap;
    return std::make_unique<Sampler>(m_rhi_device.get(), desc);
}

void Renderer::CreateAndRegisterDefaultRasterStates() noexcept {
    auto default_raster = CreateDefaultRaster();
    auto name = "__default";
    default_raster->SetDebugName("__default_raster");
    RegisterRasterState(name, std::move(default_raster));

    auto scissorenable_raster = CreateScissorEnableRaster();
    name = "__scissorenable";
    scissorenable_raster->SetDebugName("__scissorenable");
    RegisterRasterState(name, std::move(scissorenable_raster));

    auto scissordisable_raster = CreateScissorDisableRaster();
    name = "__scissordisable";
    scissordisable_raster->SetDebugName("__scissordisable");
    RegisterRasterState(name, std::move(scissordisable_raster));

    auto wireframe_raster = CreateWireframeRaster();
    name = "__wireframe";
    wireframe_raster->SetDebugName("__wireframe");
    RegisterRasterState(name, std::move(wireframe_raster));

    auto solid_raster = CreateSolidRaster();
    name = "__solid";
    solid_raster->SetDebugName("__solid");
    RegisterRasterState(name, std::move(solid_raster));

    auto wireframenc_raster = CreateWireframeNoCullingRaster();
    name = "__wireframenc";
    wireframenc_raster->SetDebugName("__wireframenc");
    RegisterRasterState(name, std::move(wireframenc_raster));

    auto solidnc_raster = CreateSolidNoCullingRaster();
    name = "__solidnc";
    solidnc_raster->SetDebugName("__solidnc");
    RegisterRasterState(name, std::move(solidnc_raster));

    auto wireframefc_raster = CreateWireframeFrontCullingRaster();
    name = "__wireframefc";
    wireframefc_raster->SetDebugName("__wireframefc");
    RegisterRasterState(name, std::move(wireframefc_raster));

    auto solidfc_raster = CreateSolidFrontCullingRaster();
    name = "__solidfc";
    solidfc_raster->SetDebugName("__solidfc");
    RegisterRasterState(name, std::move(solidfc_raster));
}

std::unique_ptr<RasterState> Renderer::CreateDefaultRaster() noexcept {
    RasterDesc default_raster{};
    return std::make_unique<RasterState>(m_rhi_device.get(), default_raster);
}

std::unique_ptr<RasterState> Renderer::CreateScissorEnableRaster() noexcept {
    RasterDesc scissorenable{};
    scissorenable.scissorEnable = true;
    return std::make_unique<RasterState>(m_rhi_device.get(), scissorenable);
}

std::unique_ptr<RasterState> Renderer::CreateScissorDisableRaster() noexcept {
    RasterDesc scissordisable{};
    scissordisable.scissorEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), scissordisable);
}

std::unique_ptr<RasterState> Renderer::CreateWireframeRaster() noexcept {
    RasterDesc wireframe{};
    wireframe.fillmode = FillMode::Wireframe;
    wireframe.cullmode = CullMode::Back;
    wireframe.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), wireframe);
}

std::unique_ptr<RasterState> Renderer::CreateSolidRaster() noexcept {
    RasterDesc solid{};
    solid.fillmode = FillMode::Solid;
    solid.cullmode = CullMode::Back;
    solid.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), solid);
}

std::unique_ptr<RasterState> Renderer::CreateWireframeNoCullingRaster() noexcept {
    RasterDesc wireframe{};
    wireframe.fillmode = FillMode::Wireframe;
    wireframe.cullmode = CullMode::None;
    wireframe.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), wireframe);
}

std::unique_ptr<RasterState> Renderer::CreateSolidNoCullingRaster() noexcept {
    RasterDesc solid{};
    solid.fillmode = FillMode::Solid;
    solid.cullmode = CullMode::None;
    solid.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), solid);
}

std::unique_ptr<RasterState> Renderer::CreateWireframeFrontCullingRaster() noexcept {
    RasterDesc wireframe{};
    wireframe.fillmode = FillMode::Wireframe;
    wireframe.cullmode = CullMode::Front;
    wireframe.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), wireframe);
}

std::unique_ptr<RasterState> Renderer::CreateSolidFrontCullingRaster() noexcept {
    RasterDesc solid{};
    solid.fillmode = FillMode::Solid;
    solid.cullmode = CullMode::Front;
    solid.antialiasedLineEnable = false;
    return std::make_unique<RasterState>(m_rhi_device.get(), solid);
}

void Renderer::CreateAndRegisterDefaultDepthStencilStates() noexcept {
    auto default_state = CreateDefaultDepthStencilState();
    auto name = "__default";
    default_state->SetDebugName("__default_depthstencilstate");
    RegisterDepthStencilState(name, std::move(default_state));

    auto depth_disabled = CreateDisabledDepth();
    name = "__depthdisabled";
    depth_disabled->SetDebugName(name);
    RegisterDepthStencilState(name, std::move(depth_disabled));

    auto depth_enabled = CreateEnabledDepth();
    name = "__depthenabled";
    depth_enabled->SetDebugName(name);
    RegisterDepthStencilState(name, std::move(depth_enabled));

    auto stencil_disabled = CreateDisabledStencil();
    name = "__stencildisabled";
    stencil_disabled->SetDebugName(name);
    RegisterDepthStencilState(name, std::move(stencil_disabled));

    auto stencil_enabled = CreateEnabledStencil();
    name = "__stencilenabled";
    stencil_enabled->SetDebugName(name);
    RegisterDepthStencilState(name, std::move(stencil_enabled));
}

std::unique_ptr<DepthStencilState> Renderer::CreateDefaultDepthStencilState() noexcept {
    DepthStencilDesc desc{};
    return std::make_unique<DepthStencilState>(m_rhi_device.get(), desc);
}

std::unique_ptr<DepthStencilState> Renderer::CreateDisabledDepth() noexcept {
    DepthStencilDesc desc{};
    desc.depth_enabled = false;
    desc.depth_comparison = ComparisonFunction::Always;
    return std::make_unique<DepthStencilState>(m_rhi_device.get(), desc);
}

std::unique_ptr<DepthStencilState> Renderer::CreateEnabledDepth() noexcept {
    DepthStencilDesc desc{};
    desc.depth_enabled = true;
    desc.depth_comparison = ComparisonFunction::Less;
    return std::make_unique<DepthStencilState>(m_rhi_device.get(), desc);
}

std::unique_ptr<DepthStencilState> Renderer::CreateDisabledStencil() noexcept {
    DepthStencilDesc desc{};
    desc.stencil_enabled = false;
    desc.stencil_read = false;
    desc.stencil_write = false;
    return std::make_unique<DepthStencilState>(m_rhi_device.get(), desc);
}

std::unique_ptr<DepthStencilState> Renderer::CreateEnabledStencil() noexcept {
    DepthStencilDesc desc{};
    desc.stencil_enabled = true;
    desc.stencil_read = true;
    desc.stencil_write = true;
    return std::make_unique<DepthStencilState>(m_rhi_device.get(), desc);
}

void Renderer::CreateAndRegisterDefaultFonts() noexcept {
    auto font_system32 = CreateDefaultSystem32Font();
    const std::string name = font_system32->GetName();
    RegisterFont(name, std::move(font_system32));

    CreateAndRegisterDefaultEngineFonts();
}

std::unique_ptr<KerningFont> Renderer::CreateDefaultSystem32Font() noexcept {
#pragma region system32_font_data
    //TURN OFF WORD WRAP AND DO NOT SCROLL TO THE RIGHT!
    const std::vector<unsigned char> raw_system32_font = {0x42, 0x4d, 0x46, 0x03, 0x01, 0x15, 0x00, 0x00, 0x00, 0x20, 0x00, 0xc0, 0x00, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x00, 0x02, 0x0f, 0x00, 0x00, 0x00, 0x20, 0x00, 0x1a, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x03, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x33, 0x32, 0x5f, 0x30, 0x2e, 0x70, 0x6e, 0x67, 0x00, 0x04, 0x78, 0x0f, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x52, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x20, 0x00, 0x00, 0x00, 0x98, 0x00, 0xb2, 0x00, 0x18, 0x00, 0x01, 0x00, 0xf8, 0xff, 0x1f, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x21, 0x00, 0x00, 0x00, 0x08, 0x00, 0x9c, 0x00, 0x04, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x22, 0x00, 0x00, 0x00, 0xb4, 0x00, 0xa8, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x00, 0x0f, 0x23, 0x00, 0x00, 0x00, 0x30, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x24, 0x00, 0x00, 0x00, 0x32, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x25, 0x00, 0x00, 0x00, 0xea, 0x00, 0x34, 0x00, 0x16, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x0f, 0x26, 0x00, 0x00, 0x00, 0x48, 0x00, 0x72, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x27, 0x00, 0x00, 0x00, 0xd2, 0x00, 0xa8, 0x00, 0x04, 0x00, 0x08, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x28, 0x00, 0x00, 0x00, 0xce, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x29, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x2a, 0x00, 0x00, 0x00, 0x92, 0x00, 0xa8, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x00, 0x0f, 0x2b, 0x00, 0x00, 0x00, 0x18, 0x00, 0xae, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x2c, 0x00, 0x00, 0x00, 0x44, 0x00, 0xba, 0x00, 0x06, 0x00, 0x06, 0x00, 0x00, 0x00, 0x16, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x2d, 0x00, 0x00, 0x00, 0x10, 0x00, 0x34, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x2e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xa6, 0x00, 0x04, 0x00, 0x04, 0x00, 0x02, 0x00, 0x16, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x2f, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x30, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x70, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x31, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x84, 0x00, 0x08, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x32, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x70, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x33, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x70, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x34, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x88, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x35, 0x00, 0x00, 0x00, 0x18, 0x00, 0x88, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x36, 0x00, 0x00, 0x00, 0x24, 0x00, 0x88, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x37, 0x00, 0x00, 0x00, 0x48, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x38, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x39, 0x00, 0x00, 0x00, 0x54, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x3a, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x10, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x3b, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x9a, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x3c, 0x00, 0x00, 0x00, 0x28, 0x00, 0x9c, 0x00, 0x0c, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x3d, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xa8, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x3e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x9c, 0x00, 0x0c, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x3f, 0x00, 0x00, 0x00, 0x60, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x40, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x18, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x0f, 0x41, 0x00, 0x00, 0x00, 0x10, 0x00, 0x60, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x43, 0x00, 0x00, 0x00, 0x10, 0x00, 0x74, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x44, 0x00, 0x00, 0x00, 0x70, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x45, 0x00, 0x00, 0x00, 0x56, 0x00, 0x72, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x46, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x48, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x47, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x5c, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x48, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x5c, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x49, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x9c, 0x00, 0x04, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x4a, 0x00, 0x00, 0x00, 0x84, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0x4b, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x5c, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x4c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x74, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x4d, 0x00, 0x00, 0x00, 0x82, 0x00, 0x4a, 0x00, 0x14, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x18, 0x00, 0x00, 0x0f, 0x4e, 0x00, 0x00, 0x00, 0x80, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x4f, 0x00, 0x00, 0x00, 0x60, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x50, 0x00, 0x00, 0x00, 0x40, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x51, 0x00, 0x00, 0x00, 0x90, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x52, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x4a, 0x00, 0x12, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x53, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x74, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x54, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x55, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x56, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x57, 0x00, 0x00, 0x00, 0xce, 0x00, 0x34, 0x00, 0x1c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x00, 0x0f, 0x58, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x48, 0x00, 0x12, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x59, 0x00, 0x00, 0x00, 0x96, 0x00, 0x4a, 0x00, 0x14, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x5a, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x4a, 0x00, 0x12, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x5b, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x5c, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x5d, 0x00, 0x00, 0x00, 0xda, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x5e, 0x00, 0x00, 0x00, 0xe2, 0x00, 0xa6, 0x00, 0x0a, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x5f, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x32, 0x00, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x60, 0x00, 0x00, 0x00, 0xec, 0x00, 0xa6, 0x00, 0x08, 0x00, 0x06, 0x00, 0x02, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x61, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xb0, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x62, 0x00, 0x00, 0x00, 0x90, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0x64, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x65, 0x00, 0x00, 0x00, 0x24, 0x00, 0xae, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x66, 0x00, 0x00, 0x00, 0xea, 0x00, 0x84, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x67, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x68, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x69, 0x00, 0x00, 0x00, 0x04, 0x00, 0x9c, 0x00, 0x04, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x6a, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x6b, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x04, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x6d, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x9a, 0x00, 0x14, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x00, 0x0f, 0x6e, 0x00, 0x00, 0x00, 0xee, 0x00, 0x98, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x6f, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x98, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x70, 0x00, 0x00, 0x00, 0x64, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x71, 0x00, 0x00, 0x00, 0x70, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x72, 0x00, 0x00, 0x00, 0x50, 0x00, 0xac, 0x00, 0x08, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x73, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x9a, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x74, 0x00, 0x00, 0x00, 0x34, 0x00, 0x9c, 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x75, 0x00, 0x00, 0x00, 0xd6, 0x00, 0x9a, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x76, 0x00, 0x00, 0x00, 0xae, 0x00, 0x9a, 0x00, 0x10, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x77, 0x00, 0x00, 0x00, 0x62, 0x00, 0x9a, 0x00, 0x14, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x00, 0x0f, 0x78, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x9a, 0x00, 0x10, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x7a, 0x00, 0x00, 0x00, 0xca, 0x00, 0x9a, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x7b, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x7c, 0x00, 0x00, 0x00, 0xec, 0x00, 0x1a, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x7d, 0x00, 0x00, 0x00, 0xba, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x7e, 0x00, 0x00, 0x00, 0x54, 0x00, 0xba, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0x7f, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x81, 0x00, 0x00, 0x00, 0x56, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x8d, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x8f, 0x00, 0x00, 0x00, 0x42, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x90, 0x00, 0x00, 0x00, 0x46, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0x9d, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x9a, 0x00, 0x04, 0x00, 0x12, 0x00, 0x02, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x62, 0x00, 0xb6, 0x00, 0x36, 0x00, 0x01, 0x00, 0xee, 0xff, 0x1f, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xa1, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x84, 0x00, 0x04, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xa2, 0x00, 0x00, 0x00, 0x10, 0x00, 0x9c, 0x00, 0x0c, 0x00, 0x12, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xa3, 0x00, 0x00, 0x00, 0x88, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xa4, 0x00, 0x00, 0x00, 0x5e, 0x00, 0xaa, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xa5, 0x00, 0x00, 0x00, 0x50, 0x00, 0x5e, 0x00, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xa6, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xa7, 0x00, 0x00, 0x00, 0x94, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xa8, 0x00, 0x00, 0x00, 0x4a, 0x00, 0xba, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xa9, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x4a, 0x00, 0x14, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xaa, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x98, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xab, 0x00, 0x00, 0x00, 0x6a, 0x00, 0xa8, 0x00, 0x0e, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0xac, 0x00, 0x00, 0x00, 0xd6, 0x00, 0xa8, 0x00, 0x0c, 0x00, 0x06, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xad, 0x00, 0x00, 0x00, 0x30, 0x00, 0x72, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xae, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x4a, 0x00, 0x14, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xaf, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x4a, 0x00, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xb0, 0x00, 0x00, 0x00, 0xcc, 0x00, 0xa8, 0x00, 0x06, 0x00, 0x08, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xb1, 0x00, 0x00, 0x00, 0x30, 0x00, 0xae, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xb2, 0x00, 0x00, 0x00, 0x9e, 0x00, 0xa8, 0x00, 0x08, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xb3, 0x00, 0x00, 0x00, 0xa6, 0x00, 0xa8, 0x00, 0x08, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xb4, 0x00, 0x00, 0x00, 0xf4, 0x00, 0xa6, 0x00, 0x08, 0x00, 0x06, 0x00, 0x02, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xb5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x0e, 0x00, 0x16, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xb6, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x84, 0x00, 0x0a, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0xb7, 0x00, 0x00, 0x00, 0x5e, 0x00, 0xb6, 0x00, 0x04, 0x00, 0x04, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xb8, 0x00, 0x00, 0x00, 0x3c, 0x00, 0xba, 0x00, 0x08, 0x00, 0x06, 0x00, 0x02, 0x00, 0x1a, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xb9, 0x00, 0x00, 0x00, 0xae, 0x00, 0xa8, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xba, 0x00, 0x00, 0x00, 0x58, 0x00, 0xac, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x0f, 0xbb, 0x00, 0x00, 0x00, 0x78, 0x00, 0xa8, 0x00, 0x0e, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0xbc, 0x00, 0x00, 0x00, 0x18, 0x00, 0x4c, 0x00, 0x16, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x0f, 0xbd, 0x00, 0x00, 0x00, 0x44, 0x00, 0x4a, 0x00, 0x16, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x0f, 0xbe, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x4a, 0x00, 0x16, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x0f, 0xbf, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc1, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc2, 0x00, 0x00, 0x00, 0x40, 0x00, 0x1a, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc3, 0x00, 0x00, 0x00, 0x30, 0x00, 0x1a, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc4, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc5, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x18, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x00, 0x0f, 0xc7, 0x00, 0x00, 0x00, 0x50, 0x00, 0x1a, 0x00, 0x0e, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xc8, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x1a, 0x00, 0x0e, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xc9, 0x00, 0x00, 0x00, 0x88, 0x00, 0x1a, 0x00, 0x0e, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xca, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x1a, 0x00, 0x0e, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xcb, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x1a, 0x00, 0x0e, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xcc, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xcd, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xce, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xcf, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x1a, 0x00, 0x08, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xd0, 0x00, 0x00, 0x00, 0xce, 0x00, 0x48, 0x00, 0x12, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd1, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd2, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd3, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd4, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd5, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd6, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd7, 0x00, 0x00, 0x00, 0x86, 0x00, 0xa8, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xd8, 0x00, 0x00, 0x00, 0x20, 0x00, 0x60, 0x00, 0x10, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xd9, 0x00, 0x00, 0x00, 0x20, 0x00, 0x1a, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xda, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xdb, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xdc, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xdd, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x14, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x0f, 0xde, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x72, 0x00, 0x0e, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x12, 0x00, 0x00, 0x0f, 0xdf, 0x00, 0x00, 0x00, 0xac, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe1, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe2, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x36, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe3, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe4, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x72, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe5, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe6, 0x00, 0x00, 0x00, 0x76, 0x00, 0x9a, 0x00, 0x14, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x00, 0x0f, 0xe7, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x70, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0xe8, 0x00, 0x00, 0x00, 0x62, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xe9, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xea, 0x00, 0x00, 0x00, 0x56, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xeb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xec, 0x00, 0x00, 0x00, 0xb6, 0x00, 0x34, 0x00, 0x08, 0x00, 0x16, 0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xed, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x34, 0x00, 0x08, 0x00, 0x16, 0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xee, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x34, 0x00, 0x08, 0x00, 0x16, 0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xef, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x84, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x30, 0x00, 0x88, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf1, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf2, 0x00, 0x00, 0x00, 0x86, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf3, 0x00, 0x00, 0x00, 0x92, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf4, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf5, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf6, 0x00, 0x00, 0x00, 0x78, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf7, 0x00, 0x00, 0x00, 0x48, 0x00, 0xac, 0x00, 0x08, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x0c, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x00, 0x3c, 0x00, 0xac, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xf9, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfa, 0x00, 0x00, 0x00, 0x26, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfb, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x34, 0x00, 0x0c, 0x00, 0x16, 0x00, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x86, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x96, 0x00, 0x1a, 0x00, 0x0c, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x0f};
#pragma endregion Do not hover.There be dragons here !

#pragma region system32_font_image
    //TURN OFF WORD WRAP AND DO NOT SCROLL TO THE RIGHT!
    const std::vector<unsigned char> raw_system32_image = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x08, 0x06, 0x00, 0x00, 0x00, 0xeb, 0xed, 0xbd, 0x66, 0x00, 0x00, 0x0e, 0x03, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x9d, 0xd9, 0x72, 0xe3, 0xba, 0x0e, 0x45, 0x9d, 0x5b, 0xf9, 0xff, 0x5f, 0xce, 0x7d, 0xc8, 0x71, 0x55, 0xcc, 0x16, 0x4d, 0x0c, 0x1b, 0x24, 0x65, 0xae, 0xf5, 0xd2, 0xd5, 0x12, 0x27, 0x0d, 0x11, 0x36, 0x01, 0x90, 0xfe, 0xfa, 0xf9, 0xf9, 0x79, 0x74, 0xe8, 0x9d, 0xf8, 0xea, 0x55, 0x70, 0xb6, 0x13, 0x6d, 0xcf, 0xda, 0xbe, 0x6a, 0x9c, 0xa3, 0x76, 0xaa, 0xfa, 0xf7, 0xf6, 0xdb, 0x96, 0x9f, 0x7d, 0xbe, 0xe5, 0x59, 0xfe, 0xa7, 0xf9, 0x7f, 0x0f, 0xef, 0xf5, 0x8f, 0xea, 0x8f, 0xda, 0xc9, 0x8e, 0x3f, 0x7b, 0xff, 0x46, 0xe3, 0xa8, 0x7a, 0x7f, 0x5e, 0xca, 0xfd, 0xcf, 0xd9, 0x09, 0x00, 0x7c, 0x10, 0xdf, 0x8e, 0xb2, 0xd6, 0x2f, 0x92, 0xf5, 0x8b, 0xf6, 0xd3, 0xfc, 0xeb, 0xed, 0xa7, 0x47, 0xb6, 0xbe, 0xb7, 0x9d, 0xd6, 0x52, 0x78, 0xe9, 0xd5, 0xb3, 0x5a, 0xce, 0xd5, 0xf4, 0xae, 0xdf, 0x7a, 0x3f, 0xaa, 0xae, 0x3f, 0xfb, 0xfc, 0xa2, 0xcf, 0xd3, 0xcb, 0xd2, 0xf7, 0x07, 0x05, 0x00, 0x70, 0x30, 0x5f, 0x17, 0x3e, 0x80, 0xa8, 0x45, 0x56, 0xcf, 0xe1, 0xaa, 0xe7, 0x8c, 0xea, 0x76, 0xb2, 0xed, 0xa9, 0xc7, 0xb1, 0x3b, 0x55, 0x73, 0xe8, 0xbb, 0xdf, 0x37, 0xd5, 0x75, 0xf7, 0xce, 0xe3, 0x03, 0x00, 0x80, 0x5f, 0xae, 0x14, 0x40, 0x94, 0xa8, 0xd7, 0xb8, 0x37, 0x07, 0x1a, 0x1d, 0x57, 0x8d, 0xcb, 0x5b, 0x2e, 0xdb, 0x8f, 0x1a, 0x6f, 0x94, 0xa5, 0x3a, 0xda, 0x30, 0xea, 0x7f, 0xd4, 0x7e, 0x75, 0xfd, 0xec, 0xf5, 0x8f, 0xda, 0x1f, 0xd5, 0x57, 0xdf, 0x3f, 0x2f, 0x28, 0x00, 0x00, 0xf8, 0xe5, 0xfb, 0x51, 0xff, 0x45, 0x1c, 0x59, 0xf6, 0xd6, 0x5b, 0xd9, 0x8b, 0x0e, 0x7c, 0x3a, 0x55, 0x96, 0xb9, 0x2a, 0x9a, 0x90, 0xed, 0x7f, 0x75, 0xfd, 0xb6, 0xdc, 0x6c, 0xdf, 0x41, 0x74, 0xfc, 0xd2, 0xbf, 0x57, 0x14, 0x00, 0xc0, 0xc1, 0x78, 0xf2, 0x00, 0x56, 0x31, 0x52, 0x0c, 0x3d, 0xb2, 0x99, 0x58, 0xd1, 0x7e, 0xbc, 0xf5, 0xbd, 0x58, 0xaf, 0xab, 0x4a, 0x51, 0xa9, 0xfb, 0x5f, 0x5d, 0xff, 0xd1, 0x9c, 0x1f, 0x3d, 0x2f, 0xeb, 0xfb, 0x98, 0xf5, 0x69, 0x4d, 0x51, 0xc4, 0x28, 0x00, 0x80, 0x83, 0xf9, 0x7e, 0xe8, 0xbf, 0x68, 0xa3, 0xf2, 0x51, 0xb2, 0xe3, 0xdc, 0x95, 0xbb, 0x67, 0x02, 0xc2, 0xbd, 0x78, 0x79, 0x9f, 0x50, 0x00, 0x00, 0x07, 0xb3, 0xc2, 0x07, 0x90, 0xb5, 0xd0, 0xd9, 0xb9, 0xe1, 0xa8, 0xdd, 0x27, 0xde, 0x39, 0xd7, 0xea, 0xfa, 0x10, 0x23, 0xea, 0x63, 0xaa, 0x7a, 0x0f, 0xab, 0x21, 0x0f, 0x00, 0x00, 0x7e, 0xf9, 0xab, 0x00, 0xee, 0xf6, 0x45, 0xcb, 0x66, 0x0a, 0x7a, 0x57, 0xab, 0x45, 0x2d, 0x74, 0x55, 0x7d, 0x80, 0x34, 0x28, 0x00, 0x80, 0x83, 0x51, 0xf8, 0x00, 0xbc, 0x99, 0x7d, 0x77, 0x53, 0x1a, 0x70, 0x06, 0xd1, 0xb8, 0xbb, 0x6a, 0xcd, 0xca, 0x12, 0x50, 0x00, 0x00, 0x07, 0x73, 0xa5, 0x00, 0xd4, 0x5f, 0xb4, 0x51, 0xa6, 0xd3, 0x2e, 0x64, 0xbd, 0xee, 0xab, 0xeb, 0x03, 0xb8, 0x41, 0x01, 0x00, 0x1c, 0xcc, 0xbb, 0xfd, 0x00, 0xd4, 0xde, 0xf5, 0xaa, 0x5d, 0x51, 0xab, 0x77, 0x12, 0xda, 0x6d, 0xbd, 0xbb, 0x55, 0xa1, 0x71, 0xfe, 0xfa, 0x7c, 0x76, 0x8d, 0x88, 0x2a, 0xf3, 0x74, 0x8b, 0xeb, 0x47, 0x01, 0x00, 0x1c, 0x8c, 0x72, 0x4f, 0xc0, 0x1e, 0x5e, 0x6f, 0x6a, 0xb6, 0xfd, 0x53, 0x14, 0x40, 0x55, 0xfb, 0x9f, 0x5e, 0x5f, 0xf5, 0xbe, 0xa8, 0xfe, 0x4e, 0x96, 0x5e, 0x3f, 0x0a, 0x00, 0xe0, 0x60, 0xfe, 0x2a, 0x80, 0xec, 0x17, 0x2d, 0x3b, 0x67, 0xe9, 0x95, 0xb3, 0x8e, 0xc7, 0xfa, 0x25, 0x54, 0xef, 0x27, 0x10, 0x1d, 0x87, 0xaa, 0xdf, 0xac, 0xcf, 0xa5, 0xea, 0x39, 0x55, 0xbd, 0x4f, 0xa3, 0xe3, 0x56, 0x85, 0xe4, 0xbd, 0xae, 0x5e, 0x7d, 0xef, 0x7b, 0x57, 0xd5, 0x4e, 0x5b, 0xae, 0x07, 0x0a, 0x00, 0x00, 0x7e, 0xb9, 0x5a, 0x0b, 0xf0, 0x64, 0x55, 0x86, 0x9e, 0xf5, 0x4b, 0xe7, 0xad, 0xef, 0xcd, 0xbd, 0xef, 0x1d, 0xcf, 0x5a, 0x56, 0xaf, 0xa5, 0x1a, 0xf5, 0x9b, 0xbd, 0x5f, 0x2a, 0x54, 0xef, 0x45, 0xd4, 0xc2, 0x5b, 0xf3, 0x4d, 0x46, 0x99, 0xa9, 0xbd, 0x7e, 0xad, 0xc7, 0xbd, 0xfd, 0xf4, 0x60, 0x47, 0x20, 0x00, 0xa8, 0x45, 0xf9, 0xdb, 0x80, 0xd9, 0xf3, 0xbd, 0xf2, 0xd6, 0x75, 0xda, 0xed, 0xf1, 0x2a, 0x1f, 0x86, 0xd5, 0x72, 0xb4, 0x58, 0x95, 0x89, 0x57, 0x39, 0xf4, 0xda, 0xef, 0x95, 0xcf, 0x3e, 0xa7, 0x55, 0x39, 0xef, 0xd5, 0x19, 0xa5, 0xab, 0x33, 0x53, 0xbd, 0x0a, 0x46, 0x02, 0x0a, 0x00, 0xe0, 0x60, 0xae, 0x14, 0x40, 0x55, 0x7c, 0x33, 0x3a, 0x77, 0x56, 0x79, 0xa9, 0xbd, 0x96, 0xaf, 0xca, 0xd2, 0x8d, 0xe6, 0x82, 0xa3, 0x2f, 0xbc, 0xd7, 0x02, 0x54, 0xfb, 0x06, 0xb2, 0xbe, 0x95, 0x55, 0xd1, 0x26, 0x6f, 0xbb, 0xd5, 0x58, 0x7d, 0x0d, 0x52, 0x50, 0x00, 0x00, 0x07, 0xa3, 0xf8, 0x65, 0xa0, 0xd5, 0x73, 0xa7, 0x4f, 0xa1, 0xda, 0x52, 0xed, 0x56, 0xcf, 0xea, 0xdb, 0x80, 0xf7, 0x58, 0x7d, 0x3f, 0x97, 0xcf, 0x03, 0x05, 0x00, 0x70, 0x30, 0x15, 0xbb, 0x02, 0x47, 0xe3, 0xd9, 0xbb, 0xf4, 0xbf, 0x7a, 0xfc, 0x00, 0xd3, 0x40, 0x01, 0x00, 0x1c, 0xcc, 0x1d, 0x7e, 0x1b, 0x10, 0xce, 0x60, 0x96, 0x6f, 0x69, 0x4a, 0x86, 0xdd, 0x5d, 0x40, 0x01, 0x00, 0x1c, 0x4c, 0xa5, 0x02, 0x28, 0xcd, 0x60, 0x9a, 0xd0, 0x7f, 0xd5, 0xf8, 0xa3, 0xf1, 0xff, 0xe8, 0xea, 0xb5, 0x5d, 0xea, 0x8d, 0xf0, 0x66, 0x7e, 0x46, 0x59, 0x12, 0x6f, 0xdf, 0x15, 0x14, 0x00, 0xc0, 0xc1, 0x28, 0x15, 0x80, 0x37, 0x23, 0xac, 0xfa, 0xcb, 0xee, 0xed, 0x3f, 0x5a, 0x3f, 0x1a, 0x87, 0x6d, 0xc9, 0x5a, 0xce, 0xdd, 0xeb, 0x41, 0x2d, 0x56, 0xdf, 0xc6, 0xcb, 0xf3, 0x43, 0x01, 0x00, 0x1c, 0x0c, 0x51, 0x00, 0x1d, 0xde, 0x2f, 0xf0, 0x69, 0x73, 0xce, 0x55, 0xfb, 0x4b, 0xc0, 0x1b, 0x50, 0x00, 0x00, 0x07, 0x53, 0xa9, 0x00, 0x56, 0x5b, 0xb8, 0x6c, 0xff, 0xea, 0x1d, 0x89, 0x4e, 0xb7, 0x74, 0xa7, 0x5f, 0xff, 0x96, 0xa0, 0x00, 0x00, 0x0e, 0x46, 0xa1, 0x00, 0xa2, 0xab, 0x09, 0x7b, 0x3b, 0xeb, 0xb8, 0x56, 0x33, 0x09, 0xfa, 0x6f, 0x89, 0x8e, 0xbf, 0xd7, 0x0e, 0xab, 0xde, 0x60, 0x5b, 0x50, 0x00, 0x00, 0x07, 0xf3, 0xfd, 0xd0, 0x5b, 0xe0, 0x28, 0xa1, 0x38, 0xe6, 0x86, 0x64, 0x95, 0x85, 0xaa, 0xbf, 0xdd, 0xea, 0x59, 0xa9, 0x52, 0x4e, 0xea, 0x9d, 0xae, 0x54, 0x51, 0x9f, 0xec, 0xb8, 0x52, 0x3b, 0x44, 0xa1, 0x00, 0x00, 0x0e, 0xc6, 0xf2, 0xdb, 0x80, 0xff, 0xd4, 0x49, 0x96, 0x1f, 0xd5, 0xab, 0x2a, 0xaf, 0xda, 0xe9, 0x48, 0x65, 0xe1, 0x57, 0xb5, 0xef, 0xed, 0x47, 0xad, 0x68, 0xb2, 0xcf, 0x4b, 0x3d, 0x6e, 0xaf, 0xe5, 0xce, 0xfa, 0x9e, 0xb2, 0xe5, 0xb3, 0xca, 0x18, 0x05, 0x00, 0x00, 0xbf, 0x58, 0xf6, 0x04, 0xb4, 0x5a, 0xf8, 0xec, 0x5c, 0x46, 0x55, 0xde, 0xbb, 0xa3, 0x8f, 0xb7, 0xbc, 0xd7, 0xf7, 0xb1, 0x5b, 0xfb, 0x3d, 0xbc, 0xbf, 0x67, 0xe0, 0x45, 0xf5, 0xbc, 0xac, 0xe3, 0x89, 0x8e, 0x3b, 0xbb, 0x6b, 0x73, 0x15, 0x23, 0x45, 0x30, 0x7a, 0xde, 0xec, 0x09, 0x08, 0x00, 0xaf, 0x7c, 0xfd, 0xfc, 0xeb, 0x04, 0xf0, 0xee, 0xb3, 0x9e, 0xf5, 0x19, 0x54, 0xcd, 0x9d, 0x67, 0x8d, 0x33, 0x3a, 0x47, 0x54, 0x5d, 0x57, 0x55, 0xb9, 0x1e, 0xb3, 0xae, 0xab, 0xfa, 0xba, 0x5b, 0xd4, 0xf7, 0x41, 0x5d, 0xbe, 0x45, 0xf2, 0x9e, 0xa1, 0x00, 0x00, 0x0e, 0x26, 0xa3, 0x00, 0x54, 0x5f, 0x70, 0x15, 0xbb, 0x8d, 0x07, 0xce, 0x60, 0x96, 0xf2, 0x54, 0x1d, 0x7f, 0x01, 0x05, 0x00, 0x70, 0x30, 0x99, 0xb5, 0x00, 0x23, 0x0b, 0x9a, 0x9d, 0x8b, 0x65, 0xbf, 0xac, 0xa3, 0xf1, 0xa8, 0x94, 0x8f, 0xb7, 0x1f, 0xca, 0xc7, 0xca, 0xaf, 0x52, 0xaa, 0xbb, 0x28, 0xc7, 0x51, 0xe6, 0x61, 0x28, 0x1a, 0x87, 0x02, 0x00, 0x38, 0x98, 0x8a, 0xd5, 0x80, 0x2d, 0xde, 0x2f, 0x71, 0x34, 0xce, 0x5a, 0xbd, 0x96, 0x20, 0x3b, 0x7e, 0xca, 0x6b, 0xcb, 0xef, 0x82, 0x35, 0x1e, 0x3f, 0x52, 0x16, 0x56, 0xcb, 0x9d, 0x8d, 0x46, 0xbd, 0x9c, 0x47, 0x01, 0x00, 0x1c, 0x4c, 0x46, 0x01, 0x78, 0x2d, 0x62, 0x76, 0x0e, 0x6f, 0x65, 0xf4, 0x85, 0xcc, 0x2a, 0x8c, 0xb6, 0x3d, 0x2b, 0xea, 0xf2, 0xb3, 0xc7, 0x03, 0x3e, 0xb2, 0x99, 0x9c, 0xd1, 0x7e, 0xac, 0xd1, 0x81, 0x9f, 0xc7, 0x03, 0x05, 0x00, 0x70, 0x34, 0xca, 0x28, 0x40, 0xf4, 0x8b, 0xa7, 0xca, 0x35, 0x9f, 0x85, 0x2a, 0x3a, 0x60, 0x6d, 0x77, 0x97, 0xf1, 0xc0, 0x7b, 0xbc, 0x5e, 0xf8, 0xec, 0x1a, 0x1a, 0x49, 0x74, 0x02, 0x05, 0x00, 0x70, 0x30, 0x57, 0x0a, 0x60, 0xf4, 0xe5, 0x6f, 0xe7, 0xf4, 0x56, 0x0b, 0x54, 0x1d, 0x4f, 0xbd, 0x7b, 0xfb, 0x5e, 0x76, 0x1b, 0x0f, 0xcc, 0x41, 0xfa, 0xdc, 0x51, 0x00, 0x00, 0x07, 0x73, 0xa5, 0x00, 0xa6, 0xce, 0x41, 0x84, 0x78, 0xf3, 0x00, 0x56, 0xcf, 0x71, 0x77, 0xf3, 0xda, 0xaf, 0xbe, 0x1f, 0xf0, 0x1e, 0xef, 0xdf, 0x9b, 0x29, 0x73, 0x10, 0x05, 0x00, 0x70, 0x30, 0x33, 0x7f, 0x1b, 0xd0, 0x6a, 0xa1, 0xa3, 0xe5, 0x7b, 0xf5, 0xda, 0xe3, 0xb3, 0xc6, 0x13, 0xcd, 0x4c, 0xac, 0xca, 0x98, 0xdb, 0x65, 0x3c, 0x9f, 0x96, 0x21, 0x68, 0xf5, 0x99, 0x3d, 0x89, 0x2a, 0x65, 0x75, 0x1e, 0x0d, 0x79, 0x00, 0x00, 0xa7, 0x63, 0xd9, 0x0f, 0x60, 0xc4, 0xac, 0x2f, 0xf1, 0xec, 0x38, 0xfa, 0xec, 0x76, 0x47, 0xa8, 0xe3, 0xf6, 0xea, 0x55, 0x6f, 0xd9, 0xbc, 0x03, 0x6f, 0x7b, 0xb3, 0xa9, 0xda, 0x89, 0x49, 0xf5, 0x7e, 0xb5, 0x98, 0xda, 0x41, 0x01, 0x00, 0x1c, 0xcc, 0xdf, 0x5f, 0x06, 0x6a, 0x51, 0x7d, 0x81, 0xab, 0x2d, 0x6a, 0x2f, 0x2f, 0xa1, 0xb7, 0x06, 0xc1, 0x3a, 0x8e, 0xe8, 0xf5, 0xab, 0x15, 0x85, 0x77, 0x4d, 0x45, 0xb6, 0xbd, 0x5d, 0x51, 0xad, 0xe2, 0xf4, 0xb6, 0xab, 0xba, 0x5f, 0xa3, 0xf7, 0x33, 0xfb, 0xde, 0x86, 0xde, 0x57, 0x14, 0x00, 0xc0, 0xc1, 0x54, 0xee, 0x08, 0xd4, 0x52, 0x65, 0x51, 0xbd, 0x73, 0xa9, 0x6c, 0xae, 0x76, 0x8f, 0xa8, 0xe5, 0x1f, 0x79, 0xbf, 0xb3, 0xde, 0xfb, 0x68, 0x7b, 0x56, 0x8b, 0x67, 0xcd, 0xb7, 0x88, 0xb6, 0xa7, 0xde, 0xbf, 0x61, 0x37, 0xdf, 0xc2, 0x13, 0xab, 0x42, 0x68, 0xcb, 0xa7, 0x40, 0x01, 0x00, 0x1c, 0x8c, 0x65, 0x2d, 0x80, 0xea, 0xcb, 0xa9, 0xb6, 0xa8, 0x6d, 0xb9, 0x25, 0x5f, 0x50, 0x61, 0xbb, 0x3d, 0x0b, 0x1e, 0x55, 0x5a, 0xaa, 0xf6, 0xb2, 0x58, 0x95, 0x40, 0xb5, 0xe5, 0x57, 0x33, 0x52, 0x4e, 0xd1, 0xfe, 0x4d, 0x19, 0x7c, 0x06, 0x4c, 0xbe, 0x0f, 0x14, 0x00, 0xc0, 0xc1, 0xcc, 0xcc, 0x04, 0xb4, 0x92, 0xcd, 0x79, 0xb6, 0x1e, 0xcf, 0xb2, 0xdb, 0x5a, 0x88, 0x11, 0x59, 0x8b, 0xaa, 0xca, 0x63, 0xf0, 0xfa, 0x32, 0xac, 0xcc, 0x7e, 0x1e, 0xd5, 0xef, 0x57, 0x16, 0xf2, 0x00, 0x00, 0xe0, 0x3d, 0xca, 0x3d, 0x01, 0x5b, 0xb2, 0x51, 0x82, 0xe8, 0xdc, 0xc9, 0xda, 0x9e, 0x37, 0x1a, 0x30, 0xaa, 0xa7, 0xce, 0xa4, 0xdb, 0xc5, 0x92, 0xa8, 0x51, 0x5b, 0xce, 0x59, 0x99, 0xac, 0xb3, 0x9e, 0xc7, 0xe8, 0xfd, 0x92, 0xfa, 0x72, 0x50, 0x00, 0x00, 0x07, 0xf3, 0x57, 0x01, 0x44, 0xe7, 0xde, 0x5e, 0xaa, 0x2c, 0x75, 0xb4, 0x3d, 0x2b, 0xea, 0x4c, 0xc4, 0x96, 0xea, 0x38, 0x75, 0x76, 0x3c, 0xd6, 0x78, 0xff, 0x2c, 0xaa, 0xa2, 0x2f, 0xbd, 0x7e, 0xaa, 0xa3, 0x28, 0xde, 0x7c, 0x17, 0xc9, 0x78, 0x50, 0x00, 0x00, 0x07, 0xb3, 0x32, 0x0a, 0xa0, 0xb6, 0xd4, 0xea, 0x4c, 0x3c, 0x6b, 0xbb, 0x6a, 0x8b, 0xd0, 0x53, 0x16, 0xd1, 0xfa, 0xed, 0xf1, 0xbb, 0x73, 0xd7, 0xeb, 0x52, 0xf9, 0x8e, 0xac, 0xca, 0xcc, 0x04, 0x0a, 0x00, 0xe0, 0x60, 0xde, 0x29, 0x80, 0xea, 0xb9, 0xe8, 0x93, 0xac, 0xa5, 0xae, 0xca, 0x94, 0x52, 0xfb, 0x1e, 0xbc, 0x73, 0x38, 0xab, 0x85, 0xe8, 0xb5, 0xa7, 0x5a, 0x4d, 0xb6, 0x8b, 0xa2, 0x58, 0xdd, 0xbf, 0x15, 0x75, 0x74, 0xac, 0x14, 0x14, 0x00, 0xc0, 0xc1, 0x7c, 0x3f, 0xe2, 0xab, 0xe8, 0xbc, 0xa8, 0x2d, 0x75, 0x36, 0x6a, 0xa1, 0x6a, 0xd7, 0xbb, 0x9a, 0x2e, 0x3a, 0x87, 0xb3, 0x2a, 0x85, 0xdd, 0xe2, 0xda, 0x55, 0xcc, 0x8a, 0x3a, 0x44, 0x95, 0x5b, 0x8b, 0xca, 0x7b, 0xef, 0x55, 0x84, 0x3d, 0x58, 0x0b, 0x00, 0x70, 0x3a, 0x9e, 0x3d, 0x01, 0x55, 0x5f, 0xdc, 0xea, 0x0c, 0x39, 0xaf, 0xb7, 0x35, 0x7a, 0x5d, 0xd1, 0xb9, 0x71, 0x36, 0xfa, 0xa1, 0xca, 0xc9, 0xb7, 0x8e, 0xa7, 0x65, 0xf5, 0xf8, 0x47, 0x44, 0x15, 0x5b, 0x55, 0xfd, 0x59, 0x79, 0x2e, 0xa1, 0xbf, 0x63, 0x14, 0x00, 0xc0, 0xc1, 0x44, 0x7e, 0x1b, 0xf0, 0x49, 0xd6, 0x52, 0x58, 0xeb, 0xa9, 0xda, 0xb5, 0xe2, 0xcd, 0x0c, 0x53, 0x11, 0xf5, 0xf6, 0xb7, 0x64, 0xe7, 0xa0, 0xd1, 0x39, 0xea, 0xac, 0x68, 0xc5, 0xa8, 0xdc, 0x2e, 0xcc, 0x8a, 0x5a, 0xa4, 0xf2, 0x08, 0x50, 0x00, 0x00, 0x07, 0x13, 0xf9, 0x6d, 0xc0, 0xaa, 0xdc, 0xe8, 0xec, 0x5c, 0xc9, 0xea, 0xd5, 0x1f, 0x1d, 0xf7, 0x62, 0xf5, 0x12, 0x67, 0xe7, 0x6e, 0xd9, 0x0c, 0x30, 0x6f, 0x7d, 0xeb, 0xf3, 0xcd, 0x8e, 0x7f, 0x75, 0x34, 0x42, 0xfd, 0xfc, 0xee, 0x92, 0xaf, 0x40, 0x14, 0x00, 0xe0, 0x74, 0x2a, 0xd6, 0x02, 0x78, 0xe3, 0xe2, 0x59, 0x5f, 0x41, 0x3b, 0x97, 0x9c, 0x15, 0x05, 0x00, 0x78, 0x87, 0x2a, 0xce, 0x5f, 0x0a, 0x0a, 0x00, 0xe0, 0x60, 0x76, 0xdc, 0x13, 0xb0, 0x65, 0x77, 0x0b, 0x1d, 0xf5, 0x31, 0xec, 0x7a, 0x5d, 0xbb, 0xce, 0xd5, 0xef, 0x46, 0x76, 0x35, 0xe7, 0xa8, 0x5d, 0xef, 0x38, 0x2e, 0x41, 0x01, 0x00, 0x1c, 0x4c, 0x65, 0x1e, 0x40, 0xd5, 0xfa, 0x7e, 0x6f, 0x0e, 0xbf, 0x2a, 0xb7, 0xbf, 0x45, 0xe5, 0x1d, 0xdf, 0x55, 0x09, 0x9c, 0x86, 0xd5, 0x27, 0x14, 0x5d, 0xaf, 0xbf, 0x4a, 0x41, 0xbd, 0xfd, 0x3b, 0x44, 0x01, 0x00, 0x1c, 0x8c, 0x32, 0x0f, 0xc0, 0x6a, 0xc9, 0x56, 0x7f, 0x11, 0x47, 0x64, 0x33, 0xe0, 0xb2, 0xe5, 0x76, 0x41, 0xba, 0xf3, 0xcc, 0x0d, 0xc9, 0x5e, 0xef, 0x6e, 0x4a, 0xe0, 0x12, 0x14, 0x00, 0xc0, 0xc1, 0xec, 0x18, 0x05, 0x50, 0xaf, 0xc7, 0xef, 0x1d, 0x57, 0x7d, 0x89, 0x3f, 0x35, 0x97, 0x5d, 0xc5, 0xa7, 0x5f, 0xdf, 0x88, 0x6a, 0x25, 0x90, 0x6a, 0x1f, 0x05, 0x00, 0x70, 0x30, 0x95, 0x0a, 0x40, 0x35, 0x87, 0xb2, 0x96, 0x53, 0x45, 0x01, 0x66, 0xa3, 0xce, 0x45, 0xef, 0x11, 0xad, 0xef, 0xdd, 0x33, 0xd0, 0x9b, 0xa1, 0x99, 0x25, 0x7b, 0xff, 0x46, 0xed, 0x8d, 0xca, 0x5b, 0xa9, 0xf6, 0xa9, 0x58, 0xdb, 0x67, 0x35, 0x20, 0x00, 0xfc, 0xa2, 0x50, 0x00, 0xd1, 0x39, 0xbb, 0x37, 0x8a, 0xb0, 0xbb, 0x25, 0xcf, 0xe2, 0xfd, 0x82, 0xf7, 0xc8, 0xc6, 0xb1, 0x47, 0xf5, 0xbc, 0x4a, 0xa0, 0xc7, 0xac, 0xb9, 0x70, 0xaf, 0x9c, 0x0a, 0x6f, 0xbf, 0xd6, 0x7c, 0x16, 0xef, 0x6a, 0xd7, 0x50, 0xfb, 0x28, 0x00, 0x80, 0x83, 0xb9, 0xda, 0x13, 0xd0, 0x5c, 0xf7, 0xbf, 0x7f, 0xab, 0x32, 0xed, 0x46, 0xfd, 0x5a, 0xdb, 0x53, 0xcf, 0xe9, 0xac, 0x96, 0xa5, 0xda, 0xfb, 0x1d, 0xed, 0x2f, 0x9a, 0xf1, 0xe6, 0xb5, 0xa8, 0x51, 0x25, 0xa2, 0x8a, 0x9e, 0xa8, 0x7d, 0x1c, 0xde, 0x7e, 0x6f, 0x51, 0x1e, 0x05, 0x00, 0x70, 0x30, 0x15, 0x3b, 0x02, 0xcd, 0xb2, 0xc8, 0xb3, 0xe3, 0xcb, 0x23, 0x2f, 0xfa, 0x68, 0x4e, 0xeb, 0xdd, 0xe7, 0xa0, 0x57, 0xcf, 0x3b, 0xc7, 0x6d, 0xcb, 0xa9, 0xda, 0xb7, 0xfa, 0x72, 0x46, 0xed, 0x67, 0x9f, 0xbb, 0x75, 0xfc, 0xd6, 0x1d, 0x7c, 0x56, 0x93, 0x55, 0x92, 0xae, 0xfb, 0x89, 0x02, 0x00, 0x38, 0x18, 0x65, 0x1e, 0x40, 0xd6, 0xb2, 0xa8, 0xfa, 0xb1, 0xf6, 0x57, 0xe5, 0x2d, 0x3f, 0x05, 0xef, 0xde, 0x7f, 0xa3, 0x7a, 0xd6, 0xf3, 0x5e, 0xda, 0xf6, 0x54, 0xf1, 0xfc, 0x27, 0xb3, 0x7d, 0x3d, 0x52, 0x50, 0x00, 0x00, 0x07, 0xf3, 0xfd, 0xd0, 0x67, 0xa2, 0x79, 0x33, 0xac, 0x56, 0x61, 0x55, 0x0e, 0x28, 0x81, 0xb9, 0x54, 0xe5, 0x7b, 0x54, 0x7b, 0xd7, 0x67, 0x93, 0x8d, 0x96, 0x90, 0x07, 0x00, 0x70, 0x3a, 0x2b, 0x56, 0x03, 0xce, 0x52, 0x02, 0xea, 0xb9, 0x5a, 0x76, 0x6e, 0xbb, 0x8b, 0x45, 0x89, 0x66, 0xcc, 0x59, 0x95, 0x51, 0x36, 0x5a, 0x12, 0x25, 0xaa, 0x50, 0xb3, 0xf7, 0x41, 0x8d, 0x75, 0xbc, 0xbd, 0xf3, 0x2d, 0x6f, 0xc7, 0x8f, 0x02, 0x00, 0x38, 0x98, 0xbf, 0x0a, 0x60, 0x55, 0x2e, 0xb5, 0x97, 0xac, 0x25, 0x8e, 0x92, 0xbd, 0x1f, 0xab, 0xef, 0xdb, 0x13, 0x55, 0xb4, 0xa6, 0x67, 0xc9, 0xad, 0x79, 0x02, 0xde, 0xfc, 0x81, 0x11, 0x51, 0x5f, 0x56, 0xaf, 0x1d, 0x6b, 0x79, 0x2f, 0x59, 0x05, 0xd6, 0x3b, 0x1f, 0x02, 0x05, 0x00, 0x70, 0x30, 0x3b, 0xee, 0x08, 0x64, 0xc5, 0x9b, 0x01, 0x16, 0xc5, 0x3b, 0xa7, 0x55, 0x5b, 0x8c, 0xd5, 0x73, 0xd3, 0xa8, 0x85, 0xb6, 0x2a, 0xb5, 0x55, 0x0a, 0xce, 0xba, 0x6a, 0xce, 0x5a, 0xde, 0x8b, 0xd7, 0x47, 0xd1, 0x3b, 0x9f, 0x02, 0x05, 0x00, 0x70, 0x30, 0xef, 0xd6, 0x02, 0xcc, 0xfa, 0x32, 0xab, 0xf0, 0x5a, 0x44, 0xd5, 0x2a, 0xb1, 0x6a, 0xaf, 0x70, 0xd6, 0xc7, 0xb0, 0x4b, 0xde, 0x45, 0x94, 0x59, 0x99, 0xa3, 0xd1, 0xf2, 0x59, 0xd4, 0x51, 0x12, 0xd7, 0xf8, 0x51, 0x00, 0x00, 0x07, 0xb3, 0xc2, 0x07, 0x30, 0xfb, 0x8b, 0xde, 0xa3, 0xea, 0xcb, 0xbe, 0x5b, 0xc6, 0x63, 0x16, 0xef, 0x1c, 0xb5, 0x65, 0x76, 0x1c, 0x7d, 0x36, 0x2a, 0xaf, 0x7e, 0xaf, 0x5c, 0x8b, 0x57, 0xb9, 0x92, 0x09, 0x08, 0x00, 0xd7, 0x7c, 0x5d, 0x6c, 0x08, 0x14, 0xb5, 0xb0, 0x6a, 0x4b, 0x11, 0xed, 0x47, 0xdd, 0x9f, 0xb5, 0xbd, 0x2a, 0xaa, 0xa3, 0x18, 0xa3, 0x7e, 0xad, 0xd7, 0x3d, 0x6b, 0x67, 0x9b, 0x6c, 0xc6, 0xdf, 0x88, 0xec, 0x75, 0xf7, 0xc8, 0xde, 0x8f, 0xa8, 0x82, 0x40, 0x01, 0x00, 0xc0, 0x35, 0x7f, 0x15, 0x80, 0x6a, 0x6e, 0xad, 0xb6, 0x2c, 0xd9, 0xfa, 0x2a, 0x4b, 0x63, 0xc5, 0x6b, 0x61, 0x9f, 0xa8, 0xc6, 0x65, 0x8d, 0x5e, 0xf4, 0xfa, 0x5d, 0xbd, 0xa6, 0x41, 0x65, 0x29, 0xb3, 0xcf, 0xaf, 0x87, 0xfa, 0x7e, 0x59, 0xb1, 0x2a, 0x20, 0x14, 0x00, 0x00, 0xd8, 0xf8, 0x7e, 0xcc, 0xcf, 0x44, 0xca, 0x5a, 0x30, 0x6b, 0xe6, 0x5d, 0xf4, 0x7c, 0xf6, 0x0b, 0x3f, 0x6b, 0xf5, 0x99, 0x75, 0xfc, 0xa7, 0xa1, 0xf2, 0x05, 0xcc, 0x26, 0x3b, 0xce, 0xde, 0x7b, 0x4a, 0x1e, 0x00, 0x00, 0x5c, 0x53, 0xb9, 0x27, 0x60, 0x8f, 0xec, 0x1c, 0xcb, 0xba, 0xda, 0xcb, 0x9a, 0x51, 0xa5, 0xce, 0xb8, 0xaa, 0xb6, 0x30, 0xbd, 0xd5, 0x77, 0xaa, 0xf6, 0x76, 0x23, 0xba, 0xde, 0xfd, 0xd3, 0x88, 0x2a, 0xe2, 0xb7, 0xc7, 0x51, 0x00, 0x00, 0x07, 0xf3, 0x77, 0x4f, 0xc0, 0x1e, 0xb3, 0xbc, 0x9c, 0x56, 0xbc, 0x5e, 0xe0, 0xe8, 0x6a, 0xbd, 0x59, 0x96, 0xb1, 0x3a, 0xda, 0x12, 0x2d, 0x1f, 0x25, 0x1b, 0x9f, 0x1e, 0xb5, 0x57, 0x6d, 0xf1, 0xa3, 0x19, 0x7a, 0xa3, 0xfa, 0x59, 0x4a, 0xd6, 0xe8, 0xa0, 0x00, 0x00, 0x0e, 0xe6, 0xaf, 0x0f, 0x60, 0x96, 0x85, 0x50, 0xa3, 0x9e, 0x03, 0x5b, 0x7d, 0x0c, 0xaa, 0xfe, 0x7b, 0x64, 0x33, 0xde, 0xa2, 0x51, 0x86, 0xea, 0xfb, 0x69, 0x2d, 0xbf, 0x8a, 0xe8, 0xda, 0x85, 0x5b, 0x46, 0x1f, 0x50, 0x00, 0x00, 0x07, 0xe3, 0xc9, 0x03, 0x88, 0x7a, 0x1b, 0x5b, 0xbc, 0x73, 0x2c, 0x95, 0xe5, 0x98, 0xf5, 0x05, 0xce, 0xee, 0x13, 0x30, 0x4b, 0x61, 0x80, 0x06, 0xeb, 0xf3, 0xda, 0x52, 0x11, 0xa0, 0x00, 0x00, 0x0e, 0xe6, 0x2a, 0x0f, 0x60, 0x76, 0x1c, 0x5b, 0x95, 0x19, 0x37, 0x2a, 0x3f, 0x9b, 0x4f, 0xdd, 0xa1, 0x67, 0x77, 0x66, 0xed, 0xd4, 0xd4, 0xeb, 0xf7, 0x56, 0xa0, 0x00, 0x00, 0x0e, 0xe6, 0xdd, 0x9e, 0x80, 0x3d, 0xbc, 0x73, 0xd4, 0x5b, 0x7e, 0x19, 0x2f, 0x98, 0x35, 0x37, 0x5f, 0x7d, 0xbf, 0x3e, 0xc5, 0x07, 0x31, 0x5b, 0x81, 0xa9, 0xa2, 0x28, 0x53, 0x41, 0x01, 0x00, 0x1c, 0x8c, 0x62, 0x2d, 0x00, 0x73, 0xda, 0x57, 0xb2, 0x16, 0xf4, 0x53, 0x2c, 0xf0, 0x2a, 0x66, 0xdf, 0xbf, 0x51, 0xe6, 0xe9, 0xd6, 0xa0, 0x00, 0x00, 0x0e, 0xc6, 0xa2, 0x00, 0xaa, 0xd7, 0xc7, 0x7f, 0x1a, 0xae, 0x5d, 0x59, 0x27, 0xb6, 0x5f, 0x55, 0xde, 0xba, 0x1f, 0x41, 0x34, 0xc3, 0xce, 0xeb, 0xd5, 0xf7, 0xde, 0x9f, 0xaa, 0x1d, 0x7b, 0x46, 0x4a, 0xc0, 0xeb, 0xa3, 0xe8, 0xe5, 0xe1, 0x78, 0xd7, 0xb6, 0xbc, 0x94, 0x47, 0x01, 0x00, 0x1c, 0xcc, 0xbb, 0x3d, 0x01, 0xff, 0x29, 0x6b, 0x6c, 0x33, 0x9a, 0x21, 0x98, 0x5d, 0xbd, 0xa6, 0x2e, 0xbf, 0x1a, 0x95, 0xef, 0x20, 0x7b, 0x9d, 0xd5, 0x96, 0xb2, 0x3d, 0xde, 0x23, 0xfb, 0x7e, 0x66, 0xdf, 0x4b, 0xd5, 0x7d, 0xec, 0x8d, 0xa7, 0xba, 0xfc, 0xe5, 0x79, 0x14, 0x00, 0xc0, 0xc1, 0x5c, 0xad, 0x05, 0xe8, 0x61, 0x9d, 0xdb, 0x79, 0xd7, 0xe3, 0x87, 0xf6, 0x32, 0x7b, 0x33, 0xae, 0x59, 0x3b, 0xf3, 0xcc, 0x26, 0x6b, 0x31, 0x43, 0x16, 0xe2, 0xa2, 0x9c, 0xaa, 0x3d, 0xeb, 0x9c, 0xb9, 0xea, 0xf9, 0x46, 0xc7, 0x6b, 0x6d, 0x6f, 0x57, 0xf0, 0x01, 0x00, 0xc0, 0x2f, 0x2b, 0xf7, 0x04, 0xcc, 0xc6, 0x4d, 0xb3, 0xd1, 0x85, 0xd5, 0x3e, 0x81, 0x91, 0x17, 0x38, 0x6a, 0x79, 0xa2, 0x19, 0x70, 0xab, 0xd7, 0x4c, 0x78, 0x15, 0x47, 0xb5, 0x82, 0xf8, 0x34, 0x05, 0x7a, 0xa9, 0xbc, 0x50, 0x00, 0x00, 0x07, 0x63, 0x59, 0x0b, 0x10, 0xdd, 0x09, 0xa5, 0x17, 0x1f, 0xfe, 0xea, 0x94, 0x53, 0x59, 0x3c, 0x6b, 0x1c, 0xf5, 0x14, 0xac, 0xcf, 0x67, 0xf5, 0x1c, 0xb7, 0x64, 0xcf, 0x3b, 0x47, 0x7f, 0xde, 0x7c, 0x8d, 0xbb, 0xe5, 0xb7, 0x90, 0x07, 0x00, 0x00, 0xaf, 0xcc, 0xd8, 0x0f, 0xc0, 0xba, 0x5a, 0x30, 0x3a, 0x97, 0x1b, 0xb5, 0x67, 0x1d, 0xc7, 0xae, 0x8c, 0x14, 0x8d, 0xb5, 0xde, 0xee, 0xd7, 0x1f, 0x55, 0x6e, 0x59, 0x4b, 0xee, 0xf5, 0xc5, 0xdc, 0xed, 0xbe, 0xb6, 0xbc, 0x8c, 0x17, 0x05, 0x00, 0x70, 0x30, 0xca, 0x28, 0x40, 0x96, 0xea, 0x1c, 0xfa, 0xbb, 0x60, 0xb5, 0x34, 0xd5, 0x96, 0x68, 0x96, 0x17, 0x5b, 0x15, 0x8f, 0xf7, 0x5a, 0x68, 0x75, 0x5e, 0x45, 0x96, 0xea, 0x35, 0x0c, 0x97, 0xe5, 0x51, 0x00, 0x00, 0x07, 0x53, 0xa1, 0x00, 0xaa, 0xe2, 0xfa, 0xd9, 0xbc, 0x81, 0x51, 0x7f, 0xb3, 0xd9, 0xc5, 0xa2, 0x3c, 0x06, 0xe7, 0xab, 0xe2, 0xda, 0x51, 0x9f, 0x8e, 0xea, 0xf9, 0x67, 0x2d, 0xae, 0x77, 0x0d, 0x41, 0x34, 0x4f, 0xa6, 0x34, 0x5f, 0x05, 0x05, 0x00, 0x70, 0x30, 0x33, 0x7c, 0x00, 0xaa, 0xb9, 0xec, 0xad, 0x77, 0x5e, 0x09, 0x10, 0xbd, 0xae, 0xe8, 0xea, 0x36, 0xaf, 0xe2, 0x52, 0x3d, 0x57, 0x75, 0x3f, 0x23, 0xaa, 0xcb, 0xab, 0xeb, 0x8d, 0xf2, 0x67, 0xac, 0x90, 0x07, 0x00, 0x00, 0xaf, 0x7c, 0xfd, 0xfc, 0xd9, 0x10, 0xa0, 0x57, 0xa6, 0xf9, 0xbf, 0x2a, 0xb7, 0x3a, 0x3b, 0x07, 0x8e, 0x5a, 0xa0, 0x5d, 0x73, 0xb7, 0xd5, 0xe3, 0xcf, 0xb6, 0xd7, 0xab, 0xd7, 0x92, 0xbd, 0x7f, 0xa3, 0xfa, 0xaa, 0xf6, 0x9f, 0xa8, 0xde, 0x3b, 0xeb, 0xb8, 0x47, 0xed, 0xf4, 0x50, 0x3f, 0x9f, 0x4b, 0x50, 0x00, 0x00, 0x07, 0x13, 0xd9, 0x13, 0xd0, 0x4a, 0x75, 0x0e, 0xf5, 0xc8, 0x27, 0x60, 0xdd, 0x97, 0x20, 0xfb, 0x25, 0x56, 0x59, 0x3e, 0xaf, 0x05, 0xf4, 0xc6, 0xbd, 0xbd, 0xf7, 0x63, 0x34, 0xe7, 0xec, 0xad, 0xce, 0x7b, 0x92, 0x7d, 0xce, 0x2a, 0x0b, 0x78, 0x17, 0xbc, 0x99, 0x8a, 0xd6, 0xf3, 0x6f, 0x41, 0x01, 0x00, 0x1c, 0x8c, 0x32, 0x0a, 0x30, 0x3b, 0x13, 0xad, 0xb5, 0x50, 0xbd, 0x7a, 0x2a, 0x1f, 0x86, 0x97, 0x5e, 0x3c, 0xd7, 0x3a, 0x67, 0x1c, 0x8d, 0xd3, 0x6a, 0xc1, 0xad, 0xed, 0x59, 0xc7, 0xd1, 0x1e, 0x57, 0x97, 0x1f, 0xdd, 0x37, 0x75, 0xf4, 0x67, 0xb6, 0x97, 0xff, 0xc9, 0x16, 0xd1, 0x2b, 0x14, 0x00, 0xc0, 0xc1, 0x7c, 0x3f, 0xf2, 0x5e, 0x7c, 0x75, 0xce, 0xfe, 0xec, 0x8c, 0x40, 0x2b, 0xd6, 0x7e, 0xaa, 0xc7, 0x93, 0xf5, 0x6d, 0xa8, 0xd6, 0x1a, 0x8c, 0x9e, 0xc3, 0xa8, 0x3d, 0xab, 0x52, 0xf0, 0xfa, 0x12, 0x4a, 0xe7, 0xcc, 0x9f, 0x06, 0x0a, 0x00, 0xe0, 0x60, 0xae, 0xf2, 0x00, 0xb2, 0x71, 0x46, 0x95, 0x65, 0x99, 0x85, 0xba, 0xff, 0x5e, 0x7b, 0x5e, 0x9f, 0x40, 0xaf, 0x3d, 0x00, 0x19, 0x28, 0x00, 0x80, 0x83, 0xb1, 0x44, 0x01, 0xb2, 0x19, 0x4d, 0xd9, 0xf3, 0xd5, 0x19, 0x82, 0x23, 0x5f, 0x42, 0xf6, 0xfa, 0x47, 0xfd, 0xf6, 0xfa, 0x39, 0x65, 0xcd, 0x03, 0x2c, 0x04, 0x05, 0x00, 0x70, 0x30, 0x99, 0x3c, 0x00, 0x75, 0x4e, 0x7a, 0xef, 0xfc, 0x2e, 0x44, 0x33, 0xdb, 0xac, 0x19, 0x91, 0xd6, 0x7a, 0x00, 0x32, 0x50, 0x00, 0x00, 0x07, 0x13, 0x51, 0x00, 0xaa, 0x5c, 0xfe, 0x28, 0xde, 0x9c, 0x74, 0x55, 0xbb, 0x51, 0x76, 0x55, 0x34, 0x00, 0x28, 0x00, 0x80, 0x93, 0xa9, 0x5c, 0x0d, 0x18, 0x25, 0x9b, 0xd1, 0xe6, 0x6d, 0xbf, 0x3d, 0xfe, 0xe8, 0x9c, 0xb7, 0x62, 0x1d, 0x2f, 0x73, 0x7c, 0x58, 0x0e, 0x0a, 0x00, 0xe0, 0x60, 0xbe, 0xfe, 0x24, 0x02, 0x66, 0x33, 0xfe, 0xac, 0xf5, 0x76, 0x23, 0xbb, 0xc3, 0x51, 0x54, 0xa1, 0xec, 0x7e, 0x5f, 0xe0, 0x00, 0x50, 0x00, 0x00, 0x07, 0xf3, 0x35, 0xde, 0x12, 0xf0, 0xe3, 0xc8, 0xae, 0x92, 0x03, 0xf8, 0x18, 0x50, 0x00, 0x00, 0x07, 0xf3, 0x7f, 0x5a, 0x64, 0x2e, 0x4d, 0x55, 0x5d, 0x9a, 0x46, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
#pragma endregion Do not hover.There be dragons here !

    if(auto font_system32 = std::make_unique<KerningFont>(); font_system32->LoadFromBuffer(raw_system32_font)) {
        std::string tex_name = "__Font_";
        {
            std::unique_ptr<Texture> tex;
            {
                const auto img = Image::CreateImageFromFileBuffer(raw_system32_image);
                tex = this->Create2DTextureFromMemory(img.GetData(), img.GetDimensions().x, img.GetDimensions().y);
            }
            tex_name += font_system32->GetName();
            tex->SetDebugName(tex_name);
            GUARANTEE_OR_DIE(this->RegisterTexture(tex_name, std::move(tex)), "Failed to load default font for Console.");
        }
        std::string name = font_system32->GetName();
        std::string shader = "__2D";
        std::ostringstream material_stream;
        material_stream << "<material name=\"__Font_" << name << "\">";
        material_stream << "<shader src=\"" << shader << "\" />";
        material_stream << "<textures>";
        material_stream << "<diffuse src=\"" << tex_name << "\" />";
        material_stream << "</textures>";
        material_stream << "</material>";
        tinyxml2::XMLDocument doc;
        std::string material_string = material_stream.str();
        const auto result = doc.Parse(material_string.c_str(), material_string.size());
        GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "Failed to create default system32 font: Invalid XML file.\n");
        const auto* xml_root = doc.RootElement();
        auto mat = std::make_unique<Material>(*xml_root);
        font_system32->SetMaterial(mat.get());
        this->RegisterMaterial(std::move(mat));
        return font_system32;
    }
    ERROR_AND_DIE("Failed to create default system32 font: Invalid buffer.\n");
}

void Renderer::UnbindAllResourcesAndBuffers() noexcept {
    UnbindAllResources();
    UnbindAllBuffers();
}

void Renderer::UnbindAllResources() noexcept {
    UnbindAllShaderResources();
    UnbindComputeShaderResources();
}

void Renderer::UnbindAllBuffers() noexcept {
    UnbindWorkingVboAndIbo();
    UnbindAllConstantBuffers();
    UnbindComputeConstantBuffers();
}

void Renderer::UnbindAllShaderResources() noexcept {
    if(m_rhi_context) {
        m_materials_need_updating = true;
        m_rhi_context->UnbindAllShaderResources();
    }
}

void Renderer::UnbindAllConstantBuffers() noexcept {
    if(m_rhi_context) {
        m_materials_need_updating = true;
        m_rhi_context->UnbindAllConstantBuffers();
    }
}

void Renderer::UnbindComputeShaderResources() noexcept {
    if(m_rhi_context) {
        m_rhi_context->UnbindAllShaderResources();
    }
}

void Renderer::UnbindComputeConstantBuffers() noexcept {
    if(m_rhi_context) {
        m_rhi_context->UnbindAllConstantBuffers();
    }
}

void Renderer::SetWindowIcon(void* /*iconResource*/) noexcept {
    //if(auto* output = GetOutput()) {
        //if(auto* window = output->GetWindow()) {
            //window->SetIcon(iconResource);
        //}
    //}
}

void Renderer::SetWindowTitle(const std::string& newTitle) noexcept {
    if(auto* output = GetOutput()) {
        if(auto* window = output->GetWindow()) {
            window->SetTitle(newTitle);
        }
    }
}

std::string Renderer::GetWindowTitle() const noexcept {
    if(auto* output = GetOutput()) {
        if(auto* window = output->GetWindow()) {
            return window->GetTitle();
        }
    }
    return std::string{};
}

void Renderer::RegisterDepthStencilState(const std::string& name, std::unique_ptr<DepthStencilState> depthstencil) noexcept {
    if(depthstencil == nullptr) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_depthstencils), std::end(m_depthstencils), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_depthstencils.end()) {
        found_iter->second.reset();
        m_depthstencils.erase(found_iter);
    }
    m_depthstencils.emplace_back(name, std::move(depthstencil));
}

RasterState* Renderer::GetRasterState(const std::string& name) noexcept {
    auto found_iter = std::find_if(std::cbegin(m_rasters), std::cend(m_rasters), [&name](const auto& s) { return s.first == name; });
    if(found_iter == m_rasters.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

void Renderer::CreateAndRegisterSamplerFromSamplerDescription(const std::string& name, const SamplerDesc& desc) noexcept {
    RegisterSampler(name, std::make_unique<Sampler>(m_rhi_device.get(), desc));
}

Sampler* Renderer::GetSampler(const std::string& name) noexcept {
    auto found_iter = std::find_if(std::cbegin(m_samplers), std::cend(m_samplers), [&name](const auto& s) { return s.first == name; });
    if(found_iter == m_samplers.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

void Renderer::SetSampler(Sampler* sampler) noexcept {
    if(sampler == m_current_sampler) {
        return;
    }
    m_rhi_context->SetSampler(sampler);
    m_current_sampler = sampler;
}

void Renderer::RegisterRasterState(const std::string& name, std::unique_ptr<RasterState> raster) noexcept {
    if(raster == nullptr) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_rasters), std::end(m_rasters), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_rasters.end()) {
        found_iter->second.reset();
        m_rasters.erase(found_iter);
    }
    m_rasters.emplace_back(name, std::move(raster));
}

void Renderer::RegisterSampler(const std::string& name, std::unique_ptr<Sampler> sampler) noexcept {
    if(sampler == nullptr) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_samplers), std::end(m_samplers), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_samplers.end()) {
        found_iter->second.reset();
        m_samplers.erase(found_iter);
    }
    m_samplers.emplace_back(name, std::move(sampler));
}

void Renderer::RegisterShader(const std::string& name, std::unique_ptr<Shader> shader) noexcept {
    if(!shader) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_shaders), std::end(m_shaders), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_shaders.end()) {
        found_iter->second.reset();
        m_shaders.erase(found_iter);
    }
    m_shaders.emplace_back(name, std::move(shader));
}

bool Renderer::RegisterShader(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    tinyxml2::XMLDocument doc;
    bool path_exists = FS::exists(filepath);
    bool has_valid_extension = filepath.has_extension() && StringUtils::ToLowerCase(filepath.extension().string()) == std::string{".shader"};
    bool is_valid_path = path_exists && has_valid_extension;
    if(!is_valid_path) {
        return false;
    }

    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec) {
            DebuggerPrintf(std::format("Could not register Shader.\nFilesystem returned the following error:\n{}\n", ec.message()));
            return false;
        }
    }
    filepath.make_preferred();
    if(doc.LoadFile(filepath.string().c_str()) == tinyxml2::XML_SUCCESS) {
        auto s = std::make_unique<Shader>(*doc.RootElement());
        const std::string name = s->GetName();
        RegisterShader(name, std::move(s));
        return true;
    }
    return false;
}

void Renderer::RegisterShader(std::unique_ptr<Shader> shader) noexcept {
    if(!shader) {
        return;
    }
    std::string name = shader->GetName();
    auto found_iter = std::find_if(std::begin(m_shaders), std::end(m_shaders), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_shaders.end()) {
        DebuggerPrintf(std::format("Shader \"{}\" already exists. Overwriting.\n", name));
        found_iter->second.reset();
        m_shaders.erase(found_iter);
    }
    m_shaders.emplace_back(name, std::move(shader));
}

void Renderer::RegisterFont(const std::string& name, std::unique_ptr<KerningFont> font) noexcept {
    if(font == nullptr) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_fonts), std::end(m_fonts), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_fonts.end()) {
        found_iter->second.reset();
        m_fonts.erase(found_iter);
    }
    m_fonts.emplace_back(name, std::move(font));
}

void Renderer::RegisterFont(std::unique_ptr<KerningFont> font) noexcept {
    if(font == nullptr) {
        return;
    }
    std::string name = font->GetName();
    auto found_iter = std::find_if(std::begin(m_fonts), std::end(m_fonts), [&name](const auto& s) { return s.first == name; });
    if(found_iter != m_fonts.end()) {
        found_iter->second.reset();
        m_fonts.erase(found_iter);
    }
    m_fonts.emplace_back(name, std::move(font));
}

bool Renderer::RegisterFont(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    auto font = std::make_unique<KerningFont>();
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    if(font->LoadFromFile(filepath.string())) {
        for(auto& texture_filename : font->GetImagePaths()) {
            namespace FS = std::filesystem;
            FS::path folderpath = font->GetFilePath();
            folderpath = FS::canonical(folderpath);
            folderpath.make_preferred();
            folderpath = folderpath.parent_path();
            FS::path texture_path = folderpath / FS::path{texture_filename};
            texture_path = FS::canonical(texture_path);
            texture_path.make_preferred();
            (void)CreateTexture(texture_path.string(), IntVector3::XY_Axis); //Don't want to store texture for later use.
        }
        if(auto mat = CreateMaterialFromFont(font.get())) {
            font->SetMaterial(mat.get());
            auto mat_name = mat->GetName();
            const std::string font_name = font->GetName();
            RegisterMaterial(mat_name, std::move(mat));
            RegisterFont(font_name, std::move(font));
            return true;
        }
    }
    return false;
}

void Renderer::RegisterFontsFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        DebuggerPrintf(std::format("Attempting to Register Fonts from unknown path: {}\n", FS::absolute(folderpath).string()));
        return;
    }
    folderpath = FS::canonical(folderpath);
    folderpath.make_preferred();
    auto cb =
    [this](const FS::path& p) {
        if(!RegisterFont(p)) {
            const auto pathAsString = p.string();
            DebuggerPrintf(std::format("Failed to load font at {}\n", pathAsString));
        }
    };
    FileUtils::ForEachFileInFolder(folderpath, ".fnt", cb, recursive);
}

void Renderer::CreateAndRegisterDefaultTextures() noexcept {
    auto default_texture = CreateDefaultTexture();
    auto name = "__default";
    default_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(default_texture)), "Failed to register default texture.");

    auto invalid_texture = CreateInvalidTexture();
    name = "__invalid";
    invalid_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(invalid_texture)), "Failed to register default invalid texture.");

    auto diffuse_texture = CreateDefaultDiffuseTexture();
    name = "__diffuse";
    diffuse_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(diffuse_texture)), "Failed to register default diffuse texture.");

    auto normal_texture = CreateDefaultNormalTexture();
    name = "__normal";
    normal_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(normal_texture)), "Failed to register default normal texture.");

    auto displacement_texture = CreateDefaultDisplacementTexture();
    name = "__displacement";
    displacement_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(displacement_texture)), "Failed to register default displacement texture.");

    auto specular_texture = CreateDefaultSpecularTexture();
    name = "__specular";
    specular_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(specular_texture)), "Failed to register default specular texture.");

    auto occlusion_texture = CreateDefaultOcclusionTexture();
    name = "__occlusion";
    occlusion_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(occlusion_texture)), "Failed to register default occlusion texture.");

    auto emissive_texture = CreateDefaultEmissiveTexture();
    name = "__emissive";
    emissive_texture->SetDebugName(name);
    GUARANTEE_OR_DIE(RegisterTexture(name, std::move(emissive_texture)), "Failed to register default emissive texture.");

    CreateDefaultColorTextures();
}

std::unique_ptr<Texture> Renderer::CreateDefaultTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::White};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateInvalidTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::Magenta,
    Rgba::Black,
    Rgba::Black,
    Rgba::Magenta,
    };
    return Create2DTextureFromMemory(data, 2, 2);
}

std::unique_ptr<Texture> Renderer::CreateDefaultDiffuseTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::White};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultNormalTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::NormalZ};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultDisplacementTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::Gray};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultSpecularTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::Black};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultOcclusionTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::White};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultEmissiveTexture() noexcept {
    static const std::vector<Rgba> data = {
    Rgba::Black};
    return Create2DTextureFromMemory(data, 1, 1);
}

std::unique_ptr<Texture> Renderer::CreateDefaultFullscreenTexture() noexcept {
    const IntVector3 dims = GetOutput()->GetBackBuffer()->GetDimensions();
    auto data = std::vector<Rgba>(static_cast<std::size_t>(dims.x) * static_cast<std::size_t>(dims.y), Rgba::Magenta);
    return Create2DTextureFromMemory(data, dims.x, dims.y, BufferUsage::Gpu, BufferBindUsage::Render_Target | BufferBindUsage::Shader_Resource);
}
void Renderer::CreateDefaultColorTextures() noexcept {
    static const std::vector<Rgba> colors = {
    Rgba::White, Rgba::Black, Rgba::Red, Rgba::Pink, Rgba::Green, Rgba::ForestGreen, Rgba::Blue, Rgba::NavyBlue, Rgba::Cyan, Rgba::Yellow, Rgba::Magenta, Rgba::Orange, Rgba::Violet, Rgba::LightGrey, Rgba::LightGray, Rgba::Grey, Rgba::Gray, Rgba::DarkGrey, Rgba::DarkGray, Rgba::Olive, Rgba::SkyBlue, Rgba::Lime, Rgba::Teal, Rgba::Turquoise, Rgba::Periwinkle, Rgba::NormalZ};
    static const std::vector<std::string> names = {
    "__white", "__black", "__red", "__pink", "__green", "__forestGreen", "__blue", "__navyBlue", "__cyan", "__yellow", "__magenta", "__orange", "__violet", "__lightGrey", "__lightGray", "__grey", "__gray", "__darkGrey", "__darkGray", "__olive", "__skyBlue", "__lime", "__teal", "__turquoise", "__periwinkle", "__normalZ"};
    const std::size_t n_s = names.size();
    const std::size_t c_s = colors.size();
    GUARANTEE_OR_DIE(n_s == c_s, "Renderer::CreateDefaultColorTextures: names and color vector sizes do not match!!");
    for(std::size_t i = 0; i < n_s; ++i) {
        auto tex = CreateDefaultColorTexture(colors[i]);
        tex->SetDebugName(names[i]);
        const std::string error_str{"Failed to register default color " + names[i]};
        GUARANTEE_OR_DIE(RegisterTexture(names[i], std::move(tex)), error_str);
    }
}

std::unique_ptr<Texture> Renderer::CreateDefaultColorTexture(const Rgba& color) noexcept {
    std::vector<Rgba> data = {
    color};
    return Create2DTextureFromMemory(data, 1, 1);
}

void Renderer::CreateAndRegisterDefaultShaders() noexcept {
    auto default_shader = CreateDefaultShader();
    std::string name = default_shader->GetName();
    RegisterShader(name, std::move(default_shader));

    auto default_unlit = CreateDefaultUnlitShader();
    name = default_unlit->GetName();
    RegisterShader(name, std::move(default_unlit));

    auto default_2D = CreateDefault2DShader();
    name = default_2D->GetName();
    RegisterShader(name, std::move(default_2D));

    auto default_normal = CreateDefaultNormalShader();
    name = default_normal->GetName();
    RegisterShader(name, std::move(default_normal));

    auto default_normal_map = CreateDefaultNormalMapShader();
    name = default_normal_map->GetName();
    RegisterShader(name, std::move(default_normal_map));

    auto default_font = CreateDefaultFontShader();
    name = default_font->GetName();
    RegisterShader(name, std::move(default_font));

    auto default_invalid = CreateDefaultInvalidShader();
    name = default_invalid->GetName();
    RegisterShader(name, std::move(default_invalid));

    auto default_circle2d = CreateDefaultCircle2DShader();
    name = default_circle2d->GetName();
    RegisterShader(name, std::move(default_circle2d));

    auto default_webp = CreateDefaultWebPShader();
    name = default_webp->GetName();
    RegisterShader(name, std::move(default_webp));
}

std::unique_ptr<Shader> Renderer::CreateDefaultShader() noexcept {
    std::string shader =
    R"(
<shader name="__default">
    <shaderprogram src="__default" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultUnlitShader() noexcept {
    std::string shader =
    R"(
<shader name="__unlit">
    <shaderprogram src="__unlit" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefault2DShader() noexcept {
    std::string shader =
    R"(
<shader name = "__2D">
    <shaderprogram src = "__unlit" />
    <raster>
        <fill>solid</fill>
        <cull>none</cull>
        <antialiasing>false</antialiasing>
    </raster>
    <blends>
        <blend enable = "true">
            <color src = "src_alpha" dest = "inv_src_alpha" op = "add" />
        </blend>
    </blends>
    <depth enable = "false" writable = "false" />
    <stencil enable = "false" readable = "false" writable = "false" />
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultCircle2DShader() noexcept {
    std::string shader =
    R"(
<shader name="__circle2d">
    <shaderprogram src="__circle2d" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
    <depth enable="false" writable="false" />
    <stencil enable="false" readable="false" writable="false" />
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultWebPShader() noexcept {
    std::string shader =
    R"(
<shader name="__webp">
    <shaderprogram src="__webp" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
    <depth enable="false" writable="false" />
    <stencil enable="false" readable="false" writable="false" />
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultNormalShader() noexcept {
    std::string shader =
    R"(
<shader name="__normal">
    <shaderprogram src="__normal" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultNormalMapShader() noexcept {
    std::string shader =
    R"(
<shader name="__normalmap">
    <shaderprogram src="__normalmap" />
    <raster src="__solid" />
    <sampler src="__default" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }
    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultInvalidShader() noexcept {
    std::string shader =
    R"(
<shader name="__invalid">
    <shaderprogram src="__unlit" />
    <raster src="__solid" />
    <sampler src="__invalid" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateDefaultFontShader() noexcept {
    std::string shader =
    R"(
<shader name="__font">
    <shaderprogram src = "__font" />
    <raster>
        <fill>solid</fill>
        <cull>none</cull>
        <antialiasing>false</antialiasing>
    </raster>
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
    <depth enable="false" writable="false" />
    <stencil enable="false" readable="false" writable="false" />
</shader>
)";
    tinyxml2::XMLDocument doc;
    auto parse_result = doc.Parse(shader.c_str(), shader.size());
    if(parse_result != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<Shader>(*doc.RootElement());
}

std::unique_ptr<Shader> Renderer::CreateShaderFromFile(std::filesystem::path filepath) noexcept {
    if(auto buffer = FileUtils::ReadStringBufferFromFile(filepath)) {
        tinyxml2::XMLDocument doc;
        auto parse_result = doc.Parse(buffer->c_str(), buffer->size());
        if(parse_result != tinyxml2::XML_SUCCESS) {
            return nullptr;
        }
        return std::make_unique<Shader>(*doc.RootElement());
    }
    return nullptr;
}

void Renderer::RegisterMaterial(const std::string& name, std::unique_ptr<Material> mat) noexcept {
    if(mat == nullptr) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_materials), std::end(m_materials), [&name](const auto& m) { return m.first == name; });
    if(found_iter != m_materials.end()) {
        DebuggerPrintf(std::format("Material \"{}\" already exists. Overwriting.\n", name));
        found_iter->second.reset();
        m_materials.erase(found_iter);
    }
    m_materials.emplace_back(name, std::move(mat));
}

void Renderer::RegisterMaterial(std::unique_ptr<Material> mat) noexcept {
    if(mat == nullptr) {
        return;
    }
    std::string name = mat->GetName();
    auto found_iter = std::find_if(std::begin(m_materials), std::end(m_materials), [&name](const auto& m) { return m.first == name; });
    if(found_iter != m_materials.end()) {
        DebuggerPrintf(std::format("Material \"{}\" already exists. Overwriting.\n", name));
        found_iter->second.reset();
        m_materials.erase(found_iter);
    }
    m_materials.emplace_back(name, std::move(mat));
}

bool Renderer::RegisterMaterial(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    tinyxml2::XMLDocument doc;
    if(filepath.has_extension() && StringUtils::ToLowerCase(filepath.extension().string()) == ".material") {
        filepath = FS::canonical(filepath);
        filepath.make_preferred();
        const auto p_str = filepath.string();
        if(doc.LoadFile(p_str.c_str()) == tinyxml2::XML_SUCCESS) {
            auto mat = std::make_unique<Material>(*doc.RootElement());
            mat->SetFilepath(filepath);
            auto name = mat->GetName();
            RegisterMaterial(name, std::move(mat));
            return true;
        }
    }
    return false;
}

void Renderer::RegisterMaterialsFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        DebuggerPrintf(std::format("Attempting to Register Materials from unknown path: {}\n", FS::absolute(folderpath).string()));
        return;
    }
    folderpath = FS::canonical(folderpath);
    folderpath.make_preferred();
    auto cb =
    [this](const FS::path& p) {
        const auto pathAsString = p.string();
        if(!RegisterMaterial(p)) {
            DebuggerPrintf(std::format("Failed to load material at {}\n", pathAsString));
        }
    };
    FileUtils::ForEachFileInFolder(folderpath, ".material", cb, recursive);
}

void Renderer::ReloadMaterials() noexcept {
    m_textures.clear();

    CreateAndRegisterDefaultTextures();

    m_materials.clear();
    CreateAndRegisterDefaultMaterials();
    RegisterMaterialsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineMaterials));
    RegisterMaterialsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameMaterials));

    m_fonts.clear();
    CreateAndRegisterDefaultFonts();
    RegisterMaterialsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineFonts));
    RegisterMaterialsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameFonts));
}

void Renderer::RegisterShaderProgram(const std::string& name, std::unique_ptr<ShaderProgram> sp) noexcept {
    if(!sp) {
        return;
    }
    auto found_iter = std::find_if(std::begin(m_shader_programs), std::end(m_shader_programs), [&name](const auto& sp) { return sp.first == name; });
    if(found_iter != m_shader_programs.end()) {
        sp->SetDescription(std::move(found_iter->second->GetDescription()));
        found_iter->second.reset(nullptr);
        m_shader_programs.erase(found_iter);
    }
    m_shader_programs.emplace_back(name, std::move(sp));
}

void Renderer::UpdateVbo(const VertexBuffer::buffer_t& vbo) noexcept {
    if(m_current_vbo_size < vbo.size()) {
        m_temp_vbo = std::move(m_rhi_device->CreateVertexBuffer<VertexBuffer>(vbo, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer));
        m_current_vbo_size = vbo.size();
    }
    m_temp_vbo->Update(*m_rhi_context, vbo);
}

void Renderer::UpdateVbco(const VertexCircleBuffer::buffer_t& vbco) noexcept {
    if(m_current_vbco_size < vbco.size()) {
        m_circle_vbo = std::move(m_rhi_device->CreateVertexBuffer<VertexCircleBuffer>(vbco, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer));
        m_current_vbco_size = vbco.size();
    }
    m_circle_vbo->Update(*m_rhi_context, vbco);
}

void Renderer::UpdateVbio(const VertexBufferInstanced::buffer_t& vbio) noexcept {
    if(m_current_vbio_size < vbio.size()) {
        m_temp_vbio = std::move(m_rhi_device->CreateVertexBufferInstanced(vbio, BufferUsage::Dynamic, BufferBindUsage::Vertex_Buffer));
        m_current_vbio_size = vbio.size();
    }
    m_temp_vbio->Update(*m_rhi_context, vbio);
}

void Renderer::UpdateIbo(const IndexBuffer::buffer_t& ibo) noexcept {
    if(m_current_ibo_size < ibo.size()) {
        m_temp_ibo = std::move(m_rhi_device->CreateIndexBuffer(ibo, BufferUsage::Dynamic, BufferBindUsage::Index_Buffer));
        m_current_ibo_size = ibo.size();
    }
    m_temp_ibo->Update(*m_rhi_context, ibo);
}

RHIDeviceContext* Renderer::GetDeviceContext() const noexcept {
    return m_rhi_context.get();
}

RHIDevice* Renderer::GetDevice() const noexcept {
    return m_rhi_device.get();
}

RHIOutput* Renderer::GetOutput() const noexcept {
    return m_rhi_output.get();
}

RHIInstance* Renderer::GetInstance() const noexcept {
    return m_rhi_instance;
}

ShaderProgram* Renderer::GetShaderProgram(const std::string& nameOrFile) noexcept {
    namespace FS = std::filesystem;
    FS::path p{nameOrFile};
    if(!StringUtils::StartsWith(p.string(), "__")) {
        p = FS::canonical(p);
    }
    p.make_preferred();
    auto found_iter = std::find_if(std::cbegin(m_shader_programs), std::cend(m_shader_programs), [&p](const auto& sp) { return sp.first == p.string(); });
    if(found_iter == m_shader_programs.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

std::unique_ptr<ShaderProgram> Renderer::CreateShaderProgramFromCsoFile(std::filesystem::path filepath, const PipelineStage& target) const noexcept {
    bool requested_retry = false;
    std::unique_ptr<ShaderProgram> sp = nullptr;
    do {
        if(auto contents = FileUtils::ReadBinaryBufferFromFile(filepath); contents.has_value()) {
            sp = std::move(RHIDevice::CreateShaderProgramFromCsoBinaryBuffer(*GetDevice(), *contents, filepath.string(), target));
            requested_retry = false;
#ifdef RENDER_DEBUG
            if(sp == nullptr) {
                std::ostringstream error_msg{};
                error_msg << "Compiled Shader \"" << filepath << "\" is ill-formed.\n";
                error_msg << "See Output window for details.\n";
                error_msg << "Press Retry to reload.";
                auto button_id = ::MessageBoxA(nullptr, error_msg.str().c_str(), "Compiled Shader load error.", MB_RETRYCANCEL | MB_ICONERROR);
                requested_retry = button_id == IDRETRY;
            }
#endif
        }
    } while(requested_retry);
    return sp;
}

std::unique_ptr<ShaderProgram> Renderer::CreateShaderProgramFromDesc(ShaderProgramDesc&& desc) const noexcept {
    return std::make_unique<ShaderProgram>(std::move(desc));
}

void Renderer::CreateAndRegisterShaderProgramFromCsoFile(std::filesystem::path filepath, const PipelineStage& target) noexcept {
    auto sp = CreateShaderProgramFromCsoFile(filepath, target);
    {
        const auto error_msg = filepath.string() + " is not a valid compiled shader program.\n";
        GUARANTEE_OR_DIE(sp, error_msg.c_str());
    }
    RegisterShaderProgram(filepath.string(), std::move(sp));
}

void Renderer::CreateAndRegisterRasterStateFromRasterDescription(const std::string& name, const RasterDesc& desc) noexcept {
    RegisterRasterState(name, std::make_unique<RasterState>(m_rhi_device.get(), desc));
}

void Renderer::SetRasterState(RasterState* raster) noexcept {
    if(raster == m_current_raster_state) {
        return;
    }
    m_rhi_context->SetRasterState(raster);
    m_current_raster_state = raster;
}

void Renderer::SetRasterState(FillMode fillmode, CullMode cullmode) noexcept {
    switch(fillmode) {
    case FillMode::Solid:
        SetSolidRaster(cullmode);
        break;
    case FillMode::Wireframe:
        SetWireframeRaster(cullmode);
        break;
    default:
        ERROR_AND_DIE("SetRasterState: Invalid fill mode");
    }
}

void Renderer::SetVSync(bool value) noexcept {
    m_vsync = value;
}

Material* Renderer::GetMaterial(const std::string& nameOrFile) noexcept {
    auto found_iter = std::find_if(std::cbegin(m_materials), std::cend(m_materials), [&nameOrFile](const auto& m) { return m.first == nameOrFile; });
    if(found_iter == m_materials.end()) {
        return GetMaterial("__invalid");
    }
    return found_iter->second.get();
}

void Renderer::SetMaterial(Material* material) noexcept {
    if(material == nullptr) {
        material = GetMaterial("__invalid");
    }
    ResetMaterial();
    m_rhi_context->SetMaterial(material);
    m_current_material = material;
    m_current_raster_state = material->GetShader()->GetRasterState();
    m_current_depthstencil_state = material->GetShader()->GetDepthStencilState();
    m_current_sampler = material->GetShader()->GetSampler();
    //_materials_need_updating = false;
}

void Renderer::SetMaterial(const std::string& nameOrFile) noexcept {
    SetMaterial(GetMaterial(nameOrFile));
}

void Renderer::ResetMaterial() noexcept {
    m_rhi_context->UnbindAllShaderResources();
    m_rhi_context->SetShader(nullptr);
    m_current_material = nullptr;
    m_current_raster_state = nullptr;
    m_current_depthstencil_state = nullptr;
    m_current_sampler = nullptr;
    m_materials_need_updating = true;
}

bool Renderer::IsTextureLoaded(const std::string& nameOrFile) const noexcept {
    namespace FS = std::filesystem;
    FS::path p{nameOrFile};
    if(!StringUtils::StartsWith(p.string(), "__")) {
        std::error_code ec{};
        p = FS::canonical(p, ec);
        if(ec) {
            return false;
        }
    }
    const auto found_iter = std::find_if(std::cbegin(m_textures), std::cend(m_textures), [&p](const auto& t) { return t.first == p.string(); });
    return found_iter != std::cend(m_textures);
}

bool Renderer::IsTextureNotLoaded(const std::string& nameOrFile) const noexcept {
    return !IsTextureLoaded(nameOrFile);
}

Shader* Renderer::GetShader(const std::string& nameOrFile) noexcept {
    const auto found_iter = std::find_if(std::cbegin(m_shaders), std::cend(m_shaders), [&nameOrFile](const auto& s) { return s.first == nameOrFile; });
    if(found_iter == m_shaders.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

std::string Renderer::GetShaderName(const std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    tinyxml2::XMLDocument doc;
    if(auto load_result = doc.LoadFile(filepath.string().c_str()); load_result == tinyxml2::XML_SUCCESS) {
        const auto* element = doc.RootElement();
        DataUtils::ValidateXmlElement(*element, "shader", "shaderprogram", "name", "depth,stencil,blends,raster,sampler,cbuffers");
        return DataUtils::ParseXmlAttribute(*element, std::string("name"), std::string{});
    }
    return {};
}

void Renderer::SetComputeShader(Shader* shader) noexcept {
    if(shader == nullptr) {
        m_rhi_context->SetComputeShaderProgram(nullptr);
    } else {
        m_rhi_context->SetComputeShaderProgram(shader->GetShaderProgram());
    }
}

KerningFont* Renderer::GetFont(const std::string& nameOrFile) noexcept {
    auto found_iter = std::find_if(std::cbegin(m_fonts), std::cend(m_fonts), [&nameOrFile](const auto& f) { return f.first == nameOrFile; });
    if(found_iter == m_fonts.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

void Renderer::SetModelMatrix(const Matrix4& mat /*= Matrix4::I*/) noexcept {
    m_matrix_data.model = mat;
    m_matrix_cb->Update(*m_rhi_context, &m_matrix_data);
    SetConstantBuffer(GetMatrixBufferIndex(), m_matrix_cb.get());
}

void Renderer::SetViewMatrix(const Matrix4& mat /*= Matrix4::I*/) noexcept {
    m_matrix_data.view = mat;
    m_matrix_cb->Update(*m_rhi_context, &m_matrix_data);
    SetConstantBuffer(GetMatrixBufferIndex(), m_matrix_cb.get());
}

void Renderer::SetProjectionMatrix(const Matrix4& mat /*= Matrix4::I*/) noexcept {
    m_matrix_data.projection = mat;
    m_matrix_cb->Update(*m_rhi_context, &m_matrix_data);
    SetConstantBuffer(GetMatrixBufferIndex(), m_matrix_cb.get());
}

void Renderer::ResetModelViewProjection() noexcept {
    SetModelMatrix(Matrix4::I);
    SetViewMatrix(Matrix4::I);
    SetProjectionMatrix(Matrix4::I);
}

void Renderer::AppendModelMatrix(const Matrix4& modelMatrix) noexcept {
    m_matrix_data.model = Matrix4::MakeRT(modelMatrix, m_matrix_data.model);
    m_matrix_cb->Update(*m_rhi_context, &m_matrix_data);
    SetConstantBuffer(GetMatrixBufferIndex(), m_matrix_cb.get());
}

void Renderer::SetOrthoProjection(const Vector2& leftBottom, const Vector2& rightTop, const Vector2& near_far) noexcept {
    Matrix4 proj = Matrix4::CreateDXOrthographicProjection(leftBottom.x, rightTop.x, leftBottom.y, rightTop.y, near_far.x, near_far.y);
    SetProjectionMatrix(proj);
}

void Renderer::SetOrthoProjection(const Vector2& dimensions, const Vector2& origin, float nearz, float farz) noexcept {
    Vector2 half_extents = dimensions * 0.5f;
    Vector2 leftBottom = Vector2(origin.x - half_extents.x, origin.y - half_extents.y);
    Vector2 rightTop = Vector2(origin.x + half_extents.x, origin.y + half_extents.y);
    SetOrthoProjection(leftBottom, rightTop, Vector2(nearz, farz));
}

void Renderer::SetOrthoProjectionFromViewHeight(float viewHeight, float aspectRatio, float nearz, float farz) noexcept {
    float view_height = viewHeight;
    float view_width = view_height * aspectRatio;
    Vector2 view_half_extents = Vector2(view_width, view_height) * 0.50f;
    Vector2 leftBottom = Vector2(-view_half_extents.x, -view_half_extents.y);
    Vector2 rightTop = Vector2(view_half_extents.x, view_half_extents.y);
    SetOrthoProjection(leftBottom, rightTop, Vector2(nearz, farz));
}

void Renderer::SetOrthoProjectionFromViewWidth(float viewWidth, float aspectRatio, float nearz, float farz) noexcept {
    float inv_aspect_ratio = 1.0f / aspectRatio;
    float view_width = viewWidth;
    float view_height = view_width * inv_aspect_ratio;
    Vector2 view_half_extents = Vector2(view_width, view_height) * 0.50f;
    Vector2 leftBottom = Vector2(-view_half_extents.x, -view_half_extents.y);
    Vector2 rightTop = Vector2(view_half_extents.x, view_half_extents.y);
    SetOrthoProjection(leftBottom, rightTop, Vector2(nearz, farz));
}

void Renderer::SetOrthoProjectionFromCamera(const Camera3D& camera) noexcept {
    float view_height = camera.CalcNearViewHeight();
    float view_width = view_height * camera.GetAspectRatio();
    Vector2 view_half_extents = Vector2(view_width, view_height) * 0.50f;
    Vector2 leftBottom = Vector2(-view_half_extents.x, -view_half_extents.y);
    Vector2 rightTop = Vector2(view_half_extents.x, view_half_extents.y);
    SetOrthoProjection(leftBottom, rightTop, Vector2(camera.GetNearDistance(), camera.GetFarDistance()));
}

void Renderer::SetPerspectiveProjection(const Vector2& vfovDegrees_aspect, const Vector2& nz_fz) noexcept {
    Matrix4 proj = Matrix4::CreateDXPerspectiveProjection(vfovDegrees_aspect.x, vfovDegrees_aspect.y, nz_fz.x, nz_fz.y);
    SetProjectionMatrix(proj);
}

void Renderer::SetPerspectiveProjectionFromCamera(const Camera3D& camera) noexcept {
    SetPerspectiveProjection(Vector2{camera.CalcFovYDegrees(), camera.GetAspectRatio()}, Vector2{camera.GetNearDistance(), camera.GetFarDistance()});
}

void Renderer::SetCamera(const Camera3D& camera) noexcept {
    m_camera = camera;
    SetViewMatrix(camera.GetViewMatrix());
    SetProjectionMatrix(camera.GetProjectionMatrix());
}

void Renderer::SetCamera(const Camera2D& camera) noexcept {
    m_camera = camera;
    SetViewMatrix(camera.GetViewMatrix());
    SetProjectionMatrix(camera.GetProjectionMatrix());
}

Camera3D Renderer::GetCamera() const noexcept {
    return m_camera;
}

Vector2 Renderer::ConvertWorldToScreenCoords(const Vector3& worldCoords) const noexcept {
    return ConvertWorldToScreenCoords(m_camera, worldCoords);
}

Vector2 Renderer::ConvertWorldToScreenCoords(const Vector2& worldCoords) const noexcept {
    return ConvertWorldToScreenCoords(m_camera, Vector3{worldCoords, 0.0f});
}

Vector2 Renderer::ConvertWorldToScreenCoords(const Camera2D& camera, const Vector2& worldCoords) const noexcept {
    return ConvertWorldToScreenCoords(Camera3D{camera}, Vector3{worldCoords, 0.0f});
}

Vector2 Renderer::ConvertWorldToScreenCoords(const Camera3D& camera, const Vector3& worldCoords) const noexcept {
    const auto& WtoS = camera.GetViewProjectionMatrix();
    const auto clipSpace = Vector4::CalcHomogeneous(WtoS * Vector4{worldCoords, 1.0f});
    const auto ndc = Vector2{clipSpace.x, -clipSpace.y};
    const auto screenDims = Vector2{GetOutput()->GetDimensions()};
    const auto mouseCoords = (ndc + Vector2::One) * screenDims * 0.5f;
    return mouseCoords;
}

Vector3 Renderer::ConvertScreenToWorldCoords(const Vector2& mouseCoords) const noexcept {
    return ConvertScreenToWorldCoords(m_camera, mouseCoords);
}

Vector3 Renderer::ConvertScreenToWorldCoords(const Camera3D& camera, const Vector2& mouseCoords) const noexcept {
    const auto ndc = 2.0f * mouseCoords / Vector2(GetOutput()->GetDimensions()) - Vector2::One;
    const auto screenCoords4 = Vector4(ndc.x, -ndc.y, 1.0f, 1.0f);
    const auto& sToW = camera.GetInverseViewProjectionMatrix();
    const auto worldPos4 = Vector4::CalcHomogeneous(sToW * screenCoords4);
    const auto worldPos3 = Vector3(worldPos4);
    return worldPos3;
}

Vector2 Renderer::ConvertScreenToWorldCoords(const Camera2D& camera, const Vector2& mouseCoords) const noexcept {
    return Vector2{ConvertScreenToWorldCoords(Camera3D{camera}, mouseCoords)};
}

Vector3 Renderer::ConvertScreenToNdcCoords(const Camera3D& /*camera*/, const Vector2& mouseCoords) const noexcept {
    const auto ndc = 2.0f * mouseCoords / Vector2(GetOutput()->GetDimensions()) - Vector2::One;
    const auto ndc3 = Vector3(ndc.x, -ndc.y, 1.0f);
    return ndc3;
}

Vector2 Renderer::ConvertScreenToNdcCoords(const Camera2D& camera, const Vector2& mouseCoords) const noexcept {
    return Vector2{ConvertScreenToNdcCoords(Camera3D{camera}, mouseCoords)};
}

Vector3 Renderer::ConvertScreenToNdcCoords(const Vector2& mouseCoords) const noexcept {
    return ConvertScreenToNdcCoords(m_camera, mouseCoords);
}

void Renderer::SetConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept {
    m_rhi_context->SetConstantBuffer(index, buffer);
}

void Renderer::SetComputeConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept {
    m_rhi_context->SetComputeConstantBuffer(index, buffer);
}

void Renderer::SetStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept {
    m_rhi_context->SetStructuredBuffer(index, buffer);
}

void Renderer::SetComputeStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept {
    m_rhi_context->SetComputeStructuredBuffer(index, buffer);
}

void Renderer::DrawCube(const Vector3& position /*= Vector3::ZERO*/, const Vector3& halfExtents /*= Vector3::ONE * 0.5f*/, const Rgba& color /*= Rgba::White*/) {
    const auto left = Vector3{-halfExtents.x, 0.0f, 0.0f};
    const auto right = Vector3{halfExtents.x, 0.0f, 0.0f};
    const auto up = Vector3{0.0f, halfExtents.y, 0.0f};
    const auto down = Vector3{0.0f, -halfExtents.y, 0.0f};
    const auto forward = Vector3{0.0f, 0.0f, -halfExtents.z};
    const auto back = Vector3{0.0f, 0.0f, halfExtents.z};

    const Vector3 v_ldf = (position + left + down + forward);
    const Vector3 v_ldb = (position + left + down + back);
    const Vector3 v_luf = (position + left + up + forward);
    const Vector3 v_lub = (position + left + up + back);
    const Vector3 v_ruf = (position + right + up + forward);
    const Vector3 v_rub = (position + right + up + back);
    const Vector3 v_rdf = (position + right + down + forward);
    const Vector3 v_rdb = (position + right + down + back);
    // clang-format off
    std::vector<Vertex3D> vbo
    {
        {v_rdf, color}, {v_ldf, color}, {v_luf, color}, {v_ruf, color}, //Front
        {v_ldb, color}, {v_rdb, color}, {v_rub, color}, {v_lub, color}, //Back
        {v_ldf, color}, {v_ldb, color}, {v_lub, color}, {v_luf, color}, //Left
        {v_rdb, color}, {v_rdf, color}, {v_ruf, color}, {v_rub, color}, //Right
        {v_ruf, color}, {v_luf, color}, {v_lub, color}, {v_rub, color}, //Top
        {v_rdb, color}, {v_ldb, color}, {v_ldf, color}, {v_rdf, color}, //Bottom
    };
    std::vector<unsigned int> ibo{
        0, 1, 2, 0, 2, 3, //Front
        4, 5, 6, 4, 6, 7, //Back
        8, 9, 10, 8, 10, 11, //Left
        12, 13, 14, 12, 14, 15, //Right
        16, 17, 18, 16, 18, 19, //Top
        20, 21, 22, 20, 22, 23, //Bottom
    };
    // clang-format on
    DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
}

void Renderer::DrawQuad(const Vector3& position /*= Vector3::ZERO*/, const Vector3& halfExtents /*= Vector3::XY_AXIS * 0.5f*/, const Rgba& color /*= Rgba::WHITE*/, const Vector4& texCoords /*= Vector4::ZW_AXIS*/, const Vector3& normalFront /*= Vector3::Z_AXIS*/, const Vector3& worldUp /*= Vector3::Y_AXIS*/) noexcept {
    Vector3 right = MathUtils::CrossProduct(worldUp, normalFront).GetNormalize();
    Vector3 up = MathUtils::CrossProduct(normalFront, right).GetNormalize();
    Vector3 left = -right;
    Vector3 down = -up;
    Vector3 normalBack = -normalFront;
    Vector3 v_lb = (position + left + down) * halfExtents;
    Vector3 v_lt = (position + left + up) * halfExtents;
    Vector3 v_rt = (position + right + up) * halfExtents;
    Vector3 v_rb = (position + right + down) * halfExtents;
    Vector2 uv_lt = Vector2(texCoords.x, texCoords.y);
    Vector2 uv_lb = Vector2(texCoords.x, texCoords.w);
    Vector2 uv_rt = Vector2(texCoords.z, texCoords.y);
    Vector2 uv_rb = Vector2(texCoords.z, texCoords.w);

    std::vector<Vertex3D> vbo{
    Vertex3D(v_lb, color, uv_lb, normalFront), Vertex3D(v_lt, color, uv_lt, normalFront), Vertex3D(v_rt, color, uv_rt, normalFront), Vertex3D(v_rb, color, uv_rb, normalFront), Vertex3D(v_rb, color, uv_rb, normalBack), Vertex3D(v_rt, color, uv_rt, normalBack), Vertex3D(v_lt, color, uv_lt, normalBack), Vertex3D(v_lb, color, uv_lb, normalBack)};

    std::vector<unsigned int> ibo{
    0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
}

void Renderer::DrawQuad(const Rgba& frontColor, const Rgba& backColor, const Vector3& position /*= Vector3::ZERO*/, const Vector3& halfExtents /*= Vector3::XY_AXIS * 0.5f*/, const Vector4& texCoords /*= Vector4::ZW_AXIS*/, const Vector3& normalFront /*= Vector3::Z_AXIS*/, const Vector3& worldUp /*= Vector3::Y_AXIS*/) noexcept {
    Vector3 right = MathUtils::CrossProduct(worldUp, normalFront).GetNormalize();
    Vector3 up = MathUtils::CrossProduct(normalFront, right).GetNormalize();
    Vector3 left = -right;
    Vector3 down = -up;
    Vector3 normalBack = -normalFront;
    Vector3 v_lb = (position + left + down) * halfExtents;
    Vector3 v_lt = (position + left + up) * halfExtents;
    Vector3 v_rt = (position + right + up) * halfExtents;
    Vector3 v_rb = (position + right + down) * halfExtents;
    Vector2 uv_lt = Vector2(texCoords.x, texCoords.y);
    Vector2 uv_lb = Vector2(texCoords.x, texCoords.w);
    Vector2 uv_rt = Vector2(texCoords.z, texCoords.y);
    Vector2 uv_rb = Vector2(texCoords.z, texCoords.w);

    std::vector<Vertex3D> vbo{
    Vertex3D(v_lb, frontColor, uv_lb, normalFront), Vertex3D(v_lt, frontColor, uv_lt, normalFront), Vertex3D(v_rt, frontColor, uv_rt, normalFront), Vertex3D(v_rb, frontColor, uv_rb, normalFront), Vertex3D(v_rb, backColor, uv_rb, normalBack), Vertex3D(v_rt, backColor, uv_rt, normalBack), Vertex3D(v_lt, backColor, uv_lt, normalBack), Vertex3D(v_lb, backColor, uv_lb, normalBack)};

    std::vector<unsigned int> ibo{
    0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
}

void Renderer::ClearRenderTargets(const RenderTargetType& rtt) noexcept {
    ID3D11DepthStencilView* dsv = m_current_depthstencil->GetDepthStencilView();
    ID3D11RenderTargetView* rtv = m_current_target->GetRenderTargetView();
    switch(rtt) {
    case RenderTargetType::None:
        return;
    case RenderTargetType::Color:
        rtv = nullptr;
        break;
    case RenderTargetType::Depth:
        dsv = nullptr;
        break;
    case RenderTargetType::Both:
        rtv = nullptr;
        dsv = nullptr;
        break;
    default:
        /* DO NOTHING */
        return;
    }
    m_rhi_context->GetDxContext()->OMSetRenderTargets(1, &rtv, dsv);
}

void Renderer::SetRenderTarget(FrameBuffer& frameBuffer) noexcept {
    frameBuffer.Bind();
}

void Renderer::SetRenderTarget(Texture* color_target /*= nullptr*/, Texture* depthstencil_target /*= nullptr*/) noexcept {
    if(color_target != nullptr) {
        m_current_target = color_target;
    } else {
        m_current_target = m_rhi_output->GetBackBuffer();
    }

    if(depthstencil_target != nullptr) {
        m_current_depthstencil = depthstencil_target;
    } else {
        m_current_depthstencil = m_rhi_output->GetDepthStencil();
    }
    ID3D11DepthStencilView* dsv = m_current_depthstencil->GetDepthStencilView();
    ID3D11RenderTargetView* rtv = m_current_target->GetRenderTargetView();
    m_rhi_context->GetDxContext()->OMSetRenderTargets(1, &rtv, dsv);
}

void Renderer::SetRenderTargetsToBackBuffer() noexcept {
    SetRenderTarget();
}

ViewportDesc Renderer::GetCurrentViewport() const {
    return GetViewport(std::size_t{0u});
}

float Renderer::GetCurrentViewportAspectRatio() const {
    const auto desc = GetCurrentViewport();
    const auto width = desc.width;
    const auto height = desc.height;
    return width / height;
}

[[nodiscard]] unsigned int Renderer::GetViewportCount() const noexcept {
    unsigned int viewportsCount = 1u;
    m_rhi_context->GetDxContext()->RSGetViewports(&viewportsCount, nullptr);
    return viewportsCount;
}

ViewportDesc Renderer::GetViewport(std::size_t index) const noexcept {
    auto viewportsCount = GetViewportCount();
    std::vector<D3D11_VIEWPORT> viewports(viewportsCount, D3D11_VIEWPORT{});
    m_rhi_context->GetDxContext()->RSGetViewports(&viewportsCount, viewports.data());
    std::vector<ViewportDesc> viewportDescs = GetAllViewports();
    if(!viewportDescs.empty()) {
        return viewportDescs[index];
    }
    return ViewportDesc{};
}

std::vector<ViewportDesc> Renderer::GetAllViewports() const noexcept {
    auto viewportsCount = GetViewportCount();
    std::vector<D3D11_VIEWPORT> viewports(viewportsCount, D3D11_VIEWPORT{});
    m_rhi_context->GetDxContext()->RSGetViewports(&viewportsCount, viewports.data());

    std::vector<ViewportDesc> viewportDescs(viewportsCount, ViewportDesc{});
    for(std::size_t vpIdx = 0u; vpIdx < viewportsCount; ++vpIdx) {
        const auto& cur_vp = viewports[vpIdx];
        auto& cur_desc = viewportDescs[vpIdx];
        cur_desc.x = cur_vp.TopLeftX;
        cur_desc.y = cur_vp.TopLeftY;
        cur_desc.width = cur_vp.Width;
        cur_desc.height = cur_vp.Height;
        cur_desc.minDepth = cur_vp.MinDepth;
        cur_desc.maxDepth = cur_vp.MaxDepth;
    }
    return viewportDescs;
}

void Renderer::SetViewport(const ViewportDesc& desc) noexcept {
    SetViewport(desc.x,
                desc.y,
                desc.width,
                desc.height);
}

void Renderer::SetViewport(float x, float y, float width, float height) noexcept {
    D3D11_VIEWPORT viewport;
    memset(&viewport, 0, sizeof(viewport));

    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_rhi_context->GetDxContext()->RSSetViewports(1, &viewport);
}

void Renderer::SetViewport(const AABB2& viewport) noexcept {
    SetViewport(viewport.mins.x, viewport.mins.y, viewport.maxs.x - viewport.mins.x, viewport.maxs.y - viewport.mins.y);
}

void Renderer::SetViewportAndScissor(float x, float y, float width, float height) noexcept {
    SetScissorAndViewport(x, y, width, height);
}

void Renderer::SetViewportAndScissor(const AABB2& viewport_and_scissor) noexcept {
    SetViewportAndScissor(viewport_and_scissor.mins.x, viewport_and_scissor.mins.y, viewport_and_scissor.maxs.x - viewport_and_scissor.mins.x, viewport_and_scissor.maxs.y - viewport_and_scissor.mins.y);
}

void Renderer::SetViewports(const std::vector<AABB3>& viewports) noexcept {
    std::vector<D3D11_VIEWPORT> dxViewports{};
    dxViewports.resize(viewports.size());

    for(std::size_t i = 0; i < dxViewports.size(); ++i) {
        dxViewports[i].TopLeftX = viewports[i].mins.x;
        dxViewports[i].TopLeftY = viewports[i].mins.y;
        dxViewports[i].Width = viewports[i].maxs.x;
        dxViewports[i].Height = viewports[i].maxs.y;
        dxViewports[i].MinDepth = viewports[i].mins.z;
        dxViewports[i].MaxDepth = viewports[i].maxs.z;
    }
    m_rhi_context->GetDxContext()->RSSetViewports(static_cast<unsigned int>(dxViewports.size()), dxViewports.data());
}

void Renderer::SetScissor(unsigned int x, unsigned int y, unsigned int width, unsigned int height) noexcept {
    D3D11_RECT scissor{};
    scissor.left = x;
    scissor.right = x + width;
    scissor.top = y;
    scissor.bottom = y + height;
    m_rhi_context->GetDxContext()->RSSetScissorRects(1, &scissor);
}

void Renderer::SetScissor(const AABB2& scissor) noexcept {
    SetScissor(static_cast<unsigned int>(std::floor(scissor.mins.x)), static_cast<unsigned int>(std::floor(scissor.mins.y)), static_cast<unsigned int>(std::floor(scissor.maxs.x - scissor.mins.x)), static_cast<unsigned int>(std::floor(scissor.maxs.y - scissor.mins.y)));
}

void Renderer::SetScissorAsPercent(float x /*= 0.0f*/, float y /*= 0.0f*/, float w /*= 1.0f*/, float h /*= 1.0f*/) noexcept {
    auto window_dimensions = GetOutput()->GetDimensions();
    auto window_width = window_dimensions.x;
    auto window_height = window_dimensions.y;
    auto left = x * window_width;
    auto top = y * window_height;
    auto width = window_width * w;
    auto height = window_height * h;
    SetScissor(static_cast<unsigned int>(std::floor(left)),
               static_cast<unsigned int>(std::floor(top)),
               static_cast<unsigned int>(std::floor(width)),
               static_cast<unsigned int>(std::floor(height)));
}

void Renderer::SetScissorAndViewport(float x, float y, float width, float height) noexcept {
    SetViewport(x, y, width, height);
    SetScissor(static_cast<unsigned int>(std::floor(x)), static_cast<unsigned int>(std::floor(y)), static_cast<unsigned int>(std::floor(width)), static_cast<unsigned int>(std::floor(height)));
}

void Renderer::SetScissorAndViewport(const AABB2& scissor_and_viewport) noexcept {
    SetScissorAndViewport(scissor_and_viewport.mins.x, scissor_and_viewport.mins.y, scissor_and_viewport.maxs.x - scissor_and_viewport.mins.x, scissor_and_viewport.maxs.y - scissor_and_viewport.mins.y);
}

void Renderer::SetScissorAndViewportAsPercent(float x /*= 0.0f*/, float y /*= 0.0f*/, float w /*= 1.0f*/, float h /*= 1.0f*/) noexcept {
    SetViewportAndScissorAsPercent(x, y, w, h);
}

void Renderer::SetScissors(const std::vector<AABB2>& scissors) noexcept {
    std::vector<D3D11_RECT> dxScissors{};
    dxScissors.resize(scissors.size());

    for(std::size_t i = 0; i < scissors.size(); ++i) {
        dxScissors[i].left = static_cast<long>(std::floor(scissors[i].mins.x));
        dxScissors[i].top = static_cast<long>(std::floor(scissors[i].mins.y));
        dxScissors[i].right = static_cast<long>(std::floor(scissors[i].maxs.x));
        dxScissors[i].bottom = static_cast<long>(std::floor(scissors[i].maxs.y));
    }
    m_rhi_context->GetDxContext()->RSSetScissorRects(static_cast<unsigned int>(dxScissors.size()), dxScissors.data());
}

void Renderer::SetViewportAsPercent(float x /*= 0.0f*/, float y /*= 0.0f*/, float w /*= 1.0f*/, float h /*= 1.0f*/) noexcept {
    auto window_dimensions = GetOutput()->GetDimensions();
    auto window_width = window_dimensions.x;
    auto window_height = window_dimensions.y;
    auto left = x * window_width;
    auto top = y * window_height;
    auto width = window_width * w;
    auto height = window_height * h;
    SetViewport(left,
                top,
                width,
                height);
}

void Renderer::SetViewportAndScissorAsPercent(float x /*= 0.0f*/, float y /*= 0.0f*/, float w /*= 1.0f*/, float h /*= 1.0f*/) noexcept {
    SetViewportAsPercent(x, y, w, h);
    SetScissorAsPercent(x, y, w, h);
}

void Renderer::EnableScissorTest() {
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> state{};
    auto* dc = GetDeviceContext();
    auto* dx_dc = dc->GetDxContext();
    dx_dc->RSGetState(state.GetAddressOf());

    D3D11_RASTERIZER_DESC desc{};
    state->GetDesc(&desc);
    if(!desc.ScissorEnable) {
        desc.ScissorEnable = true;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> newState{};
        auto hr = GetDevice()->GetDxDevice()->CreateRasterizerState(&desc, newState.GetAddressOf());
        if(SUCCEEDED(hr)) {
            dx_dc->RSSetState(newState.Get());
        }
    }
}

void Renderer::DisableScissorTest() {
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> state{};
    auto* dc = GetDeviceContext();
    auto* dx_dc = dc->GetDxContext();
    dx_dc->RSGetState(state.GetAddressOf());

    D3D11_RASTERIZER_DESC desc{};
    state->GetDesc(&desc);
    if(desc.ScissorEnable) {
        desc.ScissorEnable = false;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> newState{};
        GetDevice()->GetDxDevice()->CreateRasterizerState(&desc, newState.GetAddressOf());
        dx_dc->RSSetState(newState.Get());
    }
}

void Renderer::ClearColor(const Rgba& color) noexcept {
    m_rhi_context->ClearColorTarget(m_current_target, color);
}

void Renderer::ClearTargetColor(Texture* target, const Rgba& color) noexcept {
    m_rhi_context->ClearColorTarget(target, color);
}

void Renderer::ClearDepthStencilBuffer() noexcept {
    m_rhi_context->ClearDepthStencilTarget(m_current_depthstencil);
}

void Renderer::ClearTargetDepthStencilBuffer(Texture* target, bool depth /*= true*/, bool stencil /*= true*/, float depthValue /*= 1.0f*/, unsigned char stencilValue /*= 0*/) noexcept {
    m_rhi_context->ClearDepthStencilTarget(target, depth, stencil, depthValue, stencilValue);
}

void Renderer::Present() noexcept {
    m_rhi_output->Present(m_vsync);
}

Texture* Renderer::CreateOrGetTexture(const std::filesystem::path& filepath, const IntVector3& dimensions) noexcept {
    namespace FS = std::filesystem;
    FS::path p(filepath);
    p = FS::canonical(p);
    p.make_preferred();
    auto texture_iter = std::find_if(std::cbegin(m_textures), std::cend(m_textures), [&p](const auto& t) { return t.first == p.string(); });
    if(texture_iter == m_textures.end()) {
        return CreateTexture(p.string(), dimensions);
    } else {
        return GetTexture(p.string());
    }
}

void Renderer::RegisterTexturesFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        DebuggerPrintf(std::format("Attempting to Register Textures from unknown path: {}\n", FS::absolute(folderpath).string()));
        return;
    }
    folderpath = FS::canonical(folderpath);
    folderpath.make_preferred();
    auto cb =
    [this](const FS::path& p) {
        const auto pathAsString = p.string();
        if(!RegisterTexture(p)) {
            DebuggerPrintf(std::format("Failed to load texture at {}\n", pathAsString));
        }
    };
    FileUtils::ForEachFileInFolder(folderpath, std::string{}, cb, recursive);
}

bool Renderer::RegisterTexture(const std::filesystem::path& filepath) noexcept {
    Texture* tex = CreateTexture(filepath, IntVector3::XY_Axis);
    if(tex) {
        return true;
    }
    return false;
}

Texture* Renderer::CreateTexture(std::filesystem::path filepath,
                                 const IntVector3& dimensions /*= IntVector3::XY_AXIS*/,
                                 const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/,
                                 const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/,
                                 const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    if(dimensions.y == 0 && dimensions.z == 0) {
        return Create1DTexture(filepath, bufferUsage, bindUsage, imageFormat);
    } else if(dimensions.z == 0) {
        return Create2DTexture(filepath, bufferUsage, bindUsage, imageFormat);
    } else {
        return Create3DTexture(filepath, dimensions, bufferUsage, bindUsage, imageFormat);
    }
}

void Renderer::SetTexture(Texture* texture, unsigned int registerIndex /*= 0*/) noexcept {
    if(texture == nullptr) {
        texture = GetTexture("__invalid");
    }
    if(m_current_target == texture) {
        return;
    }
    m_current_target = texture;
    m_rhi_context->SetTexture(registerIndex, m_current_target);
}

std::unique_ptr<Texture> Renderer::CreateDepthStencil(const RHIDevice& owner, const IntVector2& dimensions) noexcept {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_resource{};

    D3D11_TEXTURE2D_DESC descDepth{};
    descDepth.Width = dimensions.x;
    descDepth.Height = dimensions.y;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = ImageFormatToDxgiFormat(ImageFormat::D24_UNorm_S8_UInt);
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = BufferUsageToD3DUsage(BufferUsage::Default);
    descDepth.BindFlags = BufferBindUsageToD3DBindFlags(BufferBindUsage::Depth_Stencil);
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    auto hr_texture = owner.GetDxDevice()->CreateTexture2D(&descDepth, nullptr, &dx_resource);
    if(SUCCEEDED(hr_texture)) {
        return std::make_unique<Texture2D>(owner, dx_resource);
    }
    return nullptr;
}

std::unique_ptr<Texture> Renderer::CreateRenderableDepthStencil(const RHIDevice& owner, const IntVector2& dimensions) noexcept {
    ID3D11Texture2D* dx_resource = nullptr;

    D3D11_TEXTURE2D_DESC descDepth{};
    descDepth.Width = dimensions.x;
    descDepth.Height = dimensions.y;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = ImageFormatToDxgiFormat(ImageFormat::R32_Typeless);
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = BufferUsageToD3DUsage(BufferUsage::Default);
    descDepth.BindFlags = BufferBindUsageToD3DBindFlags(BufferBindUsage::Depth_Stencil | BufferBindUsage::Shader_Resource);
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    HRESULT texture_hr = owner.GetDxDevice()->CreateTexture2D(&descDepth, nullptr, &dx_resource);
    bool texture_creation_succeeded = SUCCEEDED(texture_hr);
    if(texture_creation_succeeded) {
        return std::make_unique<Texture2D>(owner, dx_resource);
    }
    return nullptr;
}

void Renderer::SetDepthStencilState(DepthStencilState* depthstencil) noexcept {
    if(depthstencil == m_current_depthstencil_state) {
        return;
    }
    m_rhi_context->SetDepthStencilState(depthstencil);
    m_current_depthstencil_state = depthstencil;
}

DepthStencilState* Renderer::GetDepthStencilState(const std::string& name) noexcept {
    auto found_iter = std::find_if(std::cbegin(m_depthstencils), std::cend(m_depthstencils), [&name](const auto& ds) { return ds.first == name; });
    if(found_iter == m_depthstencils.end()) {
        return nullptr;
    }
    return found_iter->second.get();
}

void Renderer::CreateAndRegisterDepthStencilStateFromDepthStencilDescription(const std::string& name, const DepthStencilDesc& desc) noexcept {
    RegisterDepthStencilState(name, std::make_unique<DepthStencilState>(m_rhi_device.get(), desc));
}

void Renderer::EnableDepth(bool isDepthEnabled) noexcept {
    isDepthEnabled ? EnableDepth() : DisableDepth();
}

void Renderer::EnableDepth() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.DepthEnable = true;
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::DisableDepth() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.DepthEnable = false;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::EnableDepthWrite(bool isDepthWriteEnabled) noexcept {
    isDepthWriteEnabled ? EnableDepthWrite() : DisableDepthWrite();
}

void Renderer::EnableDepthWrite() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::DisableDepthWrite() noexcept {
    auto* dx = GetDeviceContext();
    auto* dx_dc = dx->GetDxContext();
    unsigned int stencil_value = 0;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state{};
    dx_dc->OMGetDepthStencilState(state.GetAddressOf(), &stencil_value);
    D3D11_DEPTH_STENCIL_DESC desc{};
    state->GetDesc(&desc);
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    GetDevice()->GetDxDevice()->CreateDepthStencilState(&desc, &state);
    dx_dc->OMSetDepthStencilState(state.Get(), stencil_value);
}

void Renderer::SetWireframeRaster(CullMode cullmode /* = CullMode::Back */) noexcept {
    switch(cullmode) {
    case CullMode::None:
        SetRasterState(GetRasterState("__wireframenc"));
        break;
    case CullMode::Front:
        SetRasterState(GetRasterState("__wireframefc"));
        break;
    case CullMode::Back:
        SetRasterState(GetRasterState("__wireframe"));
        break;
    default:
        break;
    }
}

void Renderer::SetSolidRaster(CullMode cullmode /* = CullMode::Back */) noexcept {
    switch(cullmode) {
    case CullMode::None:
        SetRasterState(GetRasterState("__solidnc"));
        break;
    case CullMode::Front:
        SetRasterState(GetRasterState("__solidfc"));
        break;
    case CullMode::Back:
        SetRasterState(GetRasterState("__solid"));
        break;
    default:
        break;
    }
}

Texture* Renderer::Create1DTexture(std::filesystem::path filepath, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        return GetTexture("__invalid");
    }
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    Image img = Image(filepath);

    D3D11_TEXTURE1D_DESC tex_desc{};
    tex_desc.Width = img.GetDimensions().x;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    tex_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subresource_data{};
    const auto width = img.GetDimensions().x;
    const auto height = img.GetDimensions().y;
    subresource_data.pSysMem = img.GetData();
    subresource_data.SysMemPitch = width * sizeof(unsigned int); // pitch is byte size of a single row)
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);
    //Force specific usages for unordered access
    if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    Microsoft::WRL::ComPtr<ID3D11Texture1D> dx_tex{};

    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture1D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        auto tex = std::make_unique<Texture1D>(*m_rhi_device, dx_tex);
        tex->SetDebugName(filepath.string().c_str());
        tex->IsLoaded(true);
        auto* tex_ptr = tex.get();
        if(RegisterTexture(filepath.string(), std::move(tex))) {
            return tex_ptr;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create1DTextureFromMemory(const unsigned char* data, unsigned int width /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    D3D11_TEXTURE1D_DESC tex_desc{};
    tex_desc.Width = width;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(bindUsage == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data{};
    subresource_data.pSysMem = data;
    subresource_data.SysMemPitch = width * sizeof(unsigned int);
    subresource_data.SysMemSlicePitch = width * sizeof(unsigned int);

    Microsoft::WRL::ComPtr<ID3D11Texture1D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = false;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture1D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture1D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create1DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    D3D11_TEXTURE1D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data{};

    subresource_data.pSysMem = data.data();
    subresource_data.SysMemPitch = width * sizeof(Rgba);
    subresource_data.SysMemSlicePitch = width * sizeof(Rgba);

    Microsoft::WRL::ComPtr<ID3D11Texture1D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = false;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture1D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture1D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

Texture* Renderer::Create2DTexture(std::filesystem::path filepath, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        return GetTexture("__invalid");
    }
    filepath = FS::canonical(filepath);
    filepath.make_preferred();
    Image img = Image(filepath.string());

    const auto is_gif = filepath.has_extension() && StringUtils::ToLowerCase(filepath.extension().string()) == ".gif";
    D3D11_TEXTURE2D_DESC tex_desc{};
    tex_desc.Width = img.GetDimensions().x;                        // width...
    tex_desc.Height = img.GetDimensions().y;                       // ...and height of image in pixels.
    tex_desc.MipLevels = 1;                                        // setting to 0 means there's a full chain (or can generate a full chain) - we're immutable, so not allowed
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);           // data is set at creation time and won't change
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);        // R8G8B8A8 texture
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage); // we're going to be using this texture as a shader resource
                                                                    //Make every texture a shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage); // Determines how I can access this resource CPU side (IMMUTABLE, So none)
    tex_desc.MiscFlags = 0;                                        // Extra Flags, of note is;
                                                                    // D3D11_RESOURCE_MISC_GENERATE_MIPS - if we want to use this to be able to generate mips (not compatible with IMMUTABLE)

    // If Multisampling - set this up - we're not multisampling, so 1 and 0
    // (MSAA as far as I know only makes sense for Render Targets, not shader resource textures)
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data;
    memset(&subresource_data, 0, sizeof(subresource_data));

    const auto width = img.GetDimensions().x;
    const auto height = img.GetDimensions().y;
    subresource_data.pSysMem = img.GetData();
    subresource_data.SysMemPitch = width * sizeof(unsigned int); // pitch is byte size of a single row)
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);
    //Force specific usages for unordered access
    if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture2D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        std::unique_ptr<Texture> tex{};
        if(is_gif) {
            tex = std::make_unique<TextureArray2D>(*m_rhi_device, dx_tex);
        } else {
            tex = std::make_unique<Texture2D>(*m_rhi_device, dx_tex);
        }
        tex->SetDebugName(filepath.string().c_str());
        tex->IsLoaded(true);
        auto* tex_ptr = tex.get();
        if(RegisterTexture(filepath.string(), std::move(tex))) {
            return tex_ptr;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create2DTextureFromMemory(const unsigned char* data, unsigned int width /*= 1*/, unsigned int height /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) const noexcept {
    D3D11_TEXTURE2D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(bindUsage == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data = {};

    subresource_data.pSysMem = data;
    subresource_data.SysMemPitch = width * sizeof(unsigned int);
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture2D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture2D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create2DTextureFromMemory(const void* data, std::size_t elementSize, unsigned int width /*= 1*/, unsigned int height /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) const noexcept {
    D3D11_TEXTURE2D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data{};

    subresource_data.pSysMem = data;
    const auto row_pitch = width * static_cast<unsigned int>(elementSize);
    subresource_data.SysMemPitch = row_pitch;
    subresource_data.SysMemSlicePitch = row_pitch * height;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || isMultiSampled;

    if(HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture2D(&tex_desc, mustUseInitialData ? &subresource_data : nullptr, dx_tex.GetAddressOf()); SUCCEEDED(hr)) {
        return std::make_unique<Texture2D>(*m_rhi_device, dx_tex);
    }
    return nullptr;
}

std::unique_ptr<Texture> Renderer::Create2DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width /*= 1*/, unsigned int height /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) const noexcept {
    D3D11_TEXTURE2D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data = {};

    subresource_data.pSysMem = data.data();
    subresource_data.SysMemPitch = width * sizeof(Rgba);
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(Rgba);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture2D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture2D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create2DTextureArrayFromMemory(const unsigned char* data, unsigned int width /*= 1*/, unsigned int height /*= 1*/, unsigned int depth /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    D3D11_TEXTURE2D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = depth;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(bindUsage == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA* subresource_data = new D3D11_SUBRESOURCE_DATA[depth];
    for(unsigned int i = 0; i < depth; ++i) {
        subresource_data[i].pSysMem = data;
        subresource_data[i].SysMemPitch = width * sizeof(unsigned int);
        subresource_data[i].SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);
    }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture2D(&tex_desc, (mustUseInitialData ? subresource_data : nullptr), &dx_tex);
    delete[] subresource_data;
    subresource_data = nullptr;
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<TextureArray2D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

Texture* Renderer::Create3DTexture(std::filesystem::path filepath, const IntVector3& dimensions, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        return GetTexture("__invalid");
    }
    filepath = FS::canonical(filepath);
    filepath.make_preferred();

    D3D11_TEXTURE3D_DESC tex_desc{};

    tex_desc.Width = dimensions.x;
    tex_desc.Height = dimensions.y;
    tex_desc.Depth = dimensions.z;
    tex_desc.MipLevels = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subresource_data{};

    if(const auto& data = FileUtils::ReadBinaryBufferFromFile(filepath)) {
        auto width = dimensions.x;
        auto height = dimensions.y;
        subresource_data.pSysMem = data->data();
        subresource_data.SysMemPitch = width * sizeof(unsigned int);
        subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);
        //Force specific usages for unordered access
        if((bindUsage & BufferBindUsage::Unordered_Access) == BufferBindUsage::Unordered_Access) {
            tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
            tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
        }
    }
    Microsoft::WRL::ComPtr<ID3D11Texture3D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = false;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture3D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        auto tex = std::make_unique<Texture3D>(*m_rhi_device, dx_tex);
        tex->SetDebugName(filepath.string().c_str());
        tex->IsLoaded(true);
        auto* tex_ptr = tex.get();
        if(RegisterTexture(filepath.string(), std::move(tex))) {
            return tex_ptr;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create3DTextureFromMemory(const unsigned char* data, unsigned int width /*= 1*/, unsigned int height /*= 1*/, unsigned int depth /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    D3D11_TEXTURE3D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.Depth = depth;
    tex_desc.MipLevels = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(bindUsage == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data{};

    subresource_data.pSysMem = data;
    subresource_data.SysMemPitch = width * sizeof(unsigned int);
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(unsigned int);

    Microsoft::WRL::ComPtr<ID3D11Texture3D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = false;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture3D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture3D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}

std::unique_ptr<Texture> Renderer::Create3DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width /*= 1*/, unsigned int height /*= 1*/, unsigned int depth /*= 1*/, const BufferUsage& bufferUsage /*= BufferUsage::STATIC*/, const BufferBindUsage& bindUsage /*= BufferBindUsage::SHADER_RESOURCE*/, const ImageFormat& imageFormat /*= ImageFormat::R8G8B8A8_UNORM*/) noexcept {
    D3D11_TEXTURE3D_DESC tex_desc{};

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.Depth = depth;
    tex_desc.MipLevels = 1;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(imageFormat);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    //Make every texture a target and shader resource
    tex_desc.BindFlags |= BufferBindUsageToD3DBindFlags(BufferBindUsage::Shader_Resource);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(bindUsage == BufferBindUsage::Unordered_Access) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if((bufferUsage & BufferUsage::Staging) == BufferUsage::Staging) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data{};

    subresource_data.pSysMem = data.data();
    subresource_data.SysMemPitch = width * sizeof(Rgba);
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(width) * static_cast<unsigned long long>(height) * sizeof(Rgba);

    Microsoft::WRL::ComPtr<ID3D11Texture3D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = false;
    bool isImmutable = bufferUsage == BufferUsage::Static;
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    HRESULT hr = m_rhi_device->GetDxDevice()->CreateTexture3D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
    bool succeeded = SUCCEEDED(hr);
    if(succeeded) {
        return std::make_unique<Texture3D>(*m_rhi_device, dx_tex);
    } else {
        return nullptr;
    }
}
