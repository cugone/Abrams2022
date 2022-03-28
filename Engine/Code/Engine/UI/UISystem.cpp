#include "Engine/UI/UISystem.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include "Engine/UI/UIWidget.hpp"

#include <Thirdparty/Imgui/imgui_internal.h>

#include <algorithm>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, 1.0f});
        return true;
    }
    return false;
}
bool ColorEdit4(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, a};
    if(ImGui::ColorEdit4(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, colorAsFloats.w});
        return true;
    }
    return false;
}
bool ColorPicker3(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, 1.0f};
    if(ImGui::ColorPicker3(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z});
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
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, colorAsFloats.w});
        if(refColor) {
            refColor->SetFromFloats({refColorAsFloats.x,refColorAsFloats.y,refColorAsFloats.z, refColorAsFloats.w});
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

UISystem::UISystem() noexcept
: EngineSubsystem()
, m_context(ImGui::CreateContext()) {
#ifdef UI_DEBUG
    IMGUI_CHECKVERSION();
#endif
}

UISystem::~UISystem() noexcept {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext(m_context);
    m_context = nullptr;
    m_widgets.clear();
}

void UISystem::Initialize() noexcept {
    namespace FS = std::filesystem;

    auto&& renderer = ServiceLocator::get<IRendererService>();
    auto* hwnd = renderer.GetOutput()->GetWindow()->GetWindowHandle();
    auto* dx_device = renderer.GetDevice()->GetDxDevice();
    auto* dx_context = renderer.GetDeviceContext()->GetDxContext();

    const auto dims = Vector2{renderer.GetOutput()->GetDimensions()};
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

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dx_device, dx_context);

}

void UISystem::BeginFrame() noexcept {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if(m_ini_saveTimer.CheckAndReset()) {
        ImGui::SaveIniSettingsToDisk(m_ini_filepath.string().c_str());
    }
}

void UISystem::Update(TimeUtils::FPSeconds /*deltaSeconds*/) noexcept {
    const auto& app = ServiceLocator::get<IAppService>();
    auto& io = ImGui::GetIO();
    io.AddFocusEvent(app.HasFocus());

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    if(m_show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
    }
    if(m_show_imgui_metrics_window) {
        ImGui::ShowMetricsWindow(&m_show_imgui_metrics_window);
    }
#endif
}

void UISystem::Render() const noexcept {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    //2D View / HUD
    auto&& renderer = ServiceLocator::get<IRendererService>();
    const float ui_view_height = renderer.GetCurrentViewport().height;
    const float ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, renderer.GetCurrentViewportAspectRatio());
    renderer.SetCamera(m_ui_camera);

    for(const auto* cur_widget : m_active_widgets) {
        cur_widget->Render();
    }
#if defined(RENDER_DEBUG)
    for(const auto* cur_widget : m_active_widgets) {
        cur_widget->DebugRender();
    }
#endif
}

void UISystem::EndFrame() noexcept {
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
}

bool UISystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    return ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(msg.hWnd), msg.nativeMessage, msg.wparam, msg.lparam);
}

bool UISystem::HasFocus() const noexcept {
    auto& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

bool UISystem::WantsInputCapture() const noexcept {
    return WantsInputKeyboardCapture() || WantsInputMouseCapture();
}

bool UISystem::WantsInputKeyboardCapture() const noexcept {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool UISystem::WantsInputMouseCapture() const noexcept {
    return ImGui::GetIO().WantCaptureMouse;
}

bool UISystem::IsImguiDemoWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    return m_show_imgui_demo_window;
#else
    return false;
#endif
}

void UISystem::ToggleImguiDemoWindow() noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    m_show_imgui_demo_window = !m_show_imgui_demo_window;
    auto&& input = ServiceLocator::get<IInputService>();
    if(!input.IsMouseCursorVisible()) {
        input.ShowMouseCursor();
    }
#endif
}

bool UISystem::IsImguiMetricsWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    return m_show_imgui_metrics_window;
#else
    return false;
#endif
}

void UISystem::ToggleImguiMetricsWindow() noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    m_show_imgui_metrics_window = !m_show_imgui_metrics_window;
    auto&& input = ServiceLocator::get<IInputService>();
    if(!input.IsMouseCursorVisible()) {
        input.ShowMouseCursor();
    }
#endif
}

bool UISystem::IsAnyImguiDebugWindowVisible() const noexcept {
#ifdef UI_DEBUG
    return IsImguiDemoWindowVisible() || IsImguiMetricsWindowVisible();
#else
    return false;
#endif
}

void UISystem::RegisterUiWidgetsFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) {
    const auto widgets_lambda = [this](const std::filesystem::path& path) {
        auto newWidget = std::make_unique<UIWidget>(path);
        const std::string name = newWidget->name;
        m_widgets.try_emplace(name, std::move(newWidget));
    };
    FileUtils::ForEachFileInFolder(folderpath, ".ui", widgets_lambda, recursive);
}

bool UISystem::IsWidgetLoaded(const UIWidget& widget) const noexcept {
    return std::find(std::begin(m_active_widgets), std::end(m_active_widgets), &widget) != std::end(m_active_widgets);
}

void UISystem::LoadUiWidgetsFromFolder(std::filesystem::path path, bool recursive /*= false*/) {
    const auto widgets_lambda = [this](const std::filesystem::path& path) {
        if(tinyxml2::XMLDocument doc; tinyxml2::XML_SUCCESS == doc.LoadFile(path.string().c_str())) {
            if(const auto* root = doc.RootElement(); DataUtils::HasAttribute(*root, "name")) {
                if(const auto name = DataUtils::ParseXmlAttribute(*root, "name", std::string{}); !name.empty()) {
                    LoadUiWidget(name);
                }
            }
        }
    };
    FileUtils::ForEachFileInFolder(path, ".ui", widgets_lambda, recursive);
}

void UISystem::LoadUiWidget(const std::string& name) {
    if(auto* widget = GetWidgetByName(name)) {
        m_active_widgets.push_back(widget);
    }
}

void UISystem::UnloadUiWidget(const std::string& name) {
    m_active_widgets.erase(std::remove_if(std::begin(m_active_widgets), std::end(m_active_widgets), [&name](UIWidget* widget) { return widget->name == name; }), std::end(m_active_widgets));
}

void UISystem::AddUiWidgetToViewport(UIWidget& widget) {
    //auto&& renderer = ServiceLocator::get<IRendererService>();
    //const auto viewport = renderer.GetCurrentViewport();
    //const auto viewportDims = Vector2{viewport.width, viewport.height};
    if(!IsWidgetLoaded(widget)) {
        LoadUiWidget(widget.name);
    }
    //TODO: Implement adding widgets to viewport.
}

void UISystem::RemoveUiWidgetFromViewport(UIWidget& widget) {
    UnloadUiWidget(widget.name);
}

UIWidget* UISystem::GetWidgetByName(const std::string& name) const {
    if(const auto& found = m_widgets.find(name); found != std::end(m_widgets)) {
        return found->second.get();
    }
    return nullptr;
}
