#include "Engine/UI/DearImgui.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <Thirdparty/Imgui/imgui_internal.h>

#ifndef UI_DEBUG
    #define IMGUI_DISABLE_DEMO_WINDOWS
    #define IMGUI_DISABLE_METRICS_WINDOW
#else
    #undef IMGUI_DISABLE_DEMO_WINDOWS
    #undef IMGUI_DISABLE_METRICS_WINDOW
#endif

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DearImgui::DearImgui() noexcept
: m_imguiContext(ImGui::CreateContext())
{
#ifdef UI_DEBUG
    IMGUI_CHECKVERSION();
#endif
}

DearImgui::~DearImgui() noexcept {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext(m_imguiContext);
    m_imguiContext = nullptr;
}

void DearImgui::Initialize() noexcept {

    auto* renderer = ServiceLocator::get<IRendererService>();
    const auto dims = Vector2{renderer->GetOutput()->GetDimensions()};
    auto& io = ImGui::GetIO();
    io.DisplaySize.x = dims.x;
    io.DisplaySize.y = dims.y;

    ImGui::StyleColorsDark();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    if(std::filesystem::exists(m_ini_filepath)) {
        ImGui::LoadIniSettingsFromDisk(m_ini_filepath.string().c_str());
    } else {
        ImGui::SaveIniSettingsToDisk(m_ini_filepath.string().c_str());
    }

    m_ini_saveTimer.SetSeconds(TimeUtils::FPSeconds{io.IniSavingRate});

    io.ConfigWindowsResizeFromEdges = true;
    io.ConfigDockingWithShift = true;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

    auto* hwnd = renderer->GetOutput()->GetWindow()->GetWindowHandle();
    ImGui_ImplWin32_Init(hwnd);

    auto* dx_device = renderer->GetDevice()->GetDxDevice();
    auto* dx_context = renderer->GetDeviceContext()->GetDxContext();
    ImGui_ImplDX11_Init(dx_device, dx_context);
}

void DearImgui::BeginFrame() noexcept {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if(m_ini_saveTimer.CheckAndReset()) {
        ImGui::SaveIniSettingsToDisk(m_ini_filepath.string().c_str());
    }
}

void DearImgui::Update() noexcept {
    const auto* const app = ServiceLocator::get<IAppService>();
    auto& io = ImGui::GetIO();
    io.AddFocusEvent(app->HasFocus());

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    if(m_show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
    }
    if(m_show_imgui_metrics_window) {
        ImGui::ShowMetricsWindow(&m_show_imgui_metrics_window);
    }
#endif
}

void DearImgui::Render() const noexcept {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DearImgui::EndFrame() noexcept {
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
}

bool DearImgui::HasFocus() const noexcept {
    return WantsInputCapture();
}

bool DearImgui::WantsInputCapture() const noexcept {
    return WantsInputKeyboardCapture() || WantsInputMouseCapture();
}

bool DearImgui::WantsInputKeyboardCapture() const noexcept {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool DearImgui::WantsInputMouseCapture() const noexcept {
    return ImGui::GetIO().WantCaptureMouse;
}

bool DearImgui::IsImguiDemoWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    return m_show_imgui_demo_window;
#else
    return false;
#endif
}

void DearImgui::ToggleImguiDemoWindow() noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    m_show_imgui_demo_window = !m_show_imgui_demo_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
#endif
}

bool DearImgui::IsImguiMetricsWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    return m_show_imgui_metrics_window;
#else
    return false;
#endif
}

void DearImgui::ToggleImguiMetricsWindow() noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    m_show_imgui_metrics_window = !m_show_imgui_metrics_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
#endif
}

bool DearImgui::IsAnyImguiDebugWindowVisible() const noexcept {
#ifdef UI_DEBUG
    return IsImguiDemoWindowVisible() || IsImguiMetricsWindowVisible();
#else
    return false;
#endif
}

bool DearImgui::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    return ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(msg.hWnd), msg.nativeMessage, msg.wparam, msg.lparam);
}

namespace ImGui {
void Image(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = border_col.GetAsFloats();
        ImGui::Image(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, Vector4{tr, tg, tb, ta}, Vector4{br, bg, bb, ba});
    }
}
void Image(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = border_col.GetAsFloats();
        ImGui::Image(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, Vector4{tr, tg, tb, ta}, Vector4{br, bg, bb, ba});
    }
}

bool ImageButton(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = bg_col.GetAsFloats();
        return ImGui::ImageButton(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, frame_padding, Vector4{br, bg, bb, ba}, Vector4{tr, tg, tb, ta});
    }
    return false;
}
bool ImageButton(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = bg_col.GetAsFloats();
        return ImGui::ImageButton(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, frame_padding, Vector4{br, bg, bb, ba}, Vector4{tr, tg, tb, ta});
    }
    return false;
}

bool ColorEdit3(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, 1.0f};
    if(ImGui::ColorEdit3(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x, colorAsFloats.y, colorAsFloats.z, 1.0f});
        return true;
    }
    return false;
}
bool ColorEdit4(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, a};
    if(ImGui::ColorEdit4(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x, colorAsFloats.y, colorAsFloats.z, colorAsFloats.w});
        return true;
    }
    return false;
}
bool ColorPicker3(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, 1.0f};
    if(ImGui::ColorPicker3(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x, colorAsFloats.y, colorAsFloats.z});
        return true;
    }
    return false;
}
bool ColorPicker4(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/, Rgba* refColor /*= nullptr*/) noexcept {
    Vector4 refColorAsFloats{};
    if(refColor) {
        const auto&& [rr, rg, rb, ra] = refColor->GetAsFloats();
        refColorAsFloats = Vector4{rr, rg, rb, ra};
    }
    const auto&& [r, g, b, a] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, a};
    if(ImGui::ColorPicker4(label, colorAsFloats.GetAsFloatArray(), flags, refColor ? refColorAsFloats.GetAsFloatArray() : nullptr)) {
        color.SetFromFloats({colorAsFloats.x, colorAsFloats.y, colorAsFloats.z, colorAsFloats.w});
        if(refColor) {
            refColor->SetFromFloats({refColorAsFloats.x, refColorAsFloats.y, refColorAsFloats.z, refColorAsFloats.w});
        }
        return true;
    }
    return false;
}
bool ColorButton(const char* desc_id, const Rgba& color, ImGuiColorEditFlags flags /*= 0*/, Vector2 size /*= Vector2::ZERO*/) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    return ImGui::ColorButton(desc_id, Vector4{r, g, b, a}, flags, size);
}

void TextColored(const Rgba& color, const char* fmt, ...) noexcept {
    auto&& [r, g, b, a] = color.GetAsFloats();
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(Vector4{r, g, b, a}, fmt, args);
    va_end(args);
}

} // namespace ImGui
