#pragma once

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Platform/Win.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#ifndef UI_DEBUG
    #define IMGUI_DISABLE_DEMO_WINDOWS
    #define IMGUI_DISABLE_METRICS_WINDOW
#else
    #undef IMGUI_DISABLE_DEMO_WINDOWS
    #undef IMGUI_DISABLE_METRICS_WINDOW
#endif

#include <Thirdparty/Imgui/imgui.h>
#include <Thirdparty/Imgui/imgui_impl_dx11.h>
#include <Thirdparty/Imgui/imgui_impl_win32.h>
#include <Thirdparty/Imgui/imgui_stdlib.h>
#include <Thirdparty/clay/clay.h>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>

class UIWidget;
class Renderer;
class FileLogger;
class InputSystem;
class KerningFont;

//TODO: Maybe make Service
class UISystem : public EngineSubsystem {
public:
    UISystem() noexcept;
    UISystem(const UISystem& other) = default;
    UISystem(UISystem&& other) = default;
    UISystem& operator=(const UISystem& other) = default;
    UISystem& operator=(UISystem&& other) = default;
    virtual ~UISystem() noexcept;

    virtual void Initialize() noexcept override;
    virtual void BeginFrame() noexcept override;
    virtual void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    virtual void Render() const noexcept override;
    virtual void EndFrame() noexcept override;
    [[nodiscard]] virtual bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;

    [[nodiscard]] bool HasFocus() const noexcept;

    [[nodiscard]] bool WantsInputCapture() const noexcept;
    [[nodiscard]] bool WantsInputKeyboardCapture() const noexcept;
    [[nodiscard]] bool WantsInputMouseCapture() const noexcept;

    [[nodiscard]] bool IsImguiDemoWindowVisible() const noexcept;
    void ToggleImguiDemoWindow() noexcept;
    [[nodiscard]] bool IsImguiMetricsWindowVisible() const noexcept;
    void ToggleImguiMetricsWindow() noexcept;
    [[nodiscard]] bool IsAnyImguiDebugWindowVisible() const noexcept;

    void SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept;

protected:
private:
    void ClayInit() noexcept;
    void ClayUpdate(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ClayRender() const noexcept;

    ImGuiContext* m_context{};
    mutable Camera2D m_ui_camera{};
    std::filesystem::path m_ini_filepath{FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineConfig) / "ui.ini"};
    Stopwatch m_ini_saveTimer{};
    std::function<void()> m_clayLayoutCallback{};
    mutable Clay_RenderCommandArray m_clay_commands{};
    bool m_show_imgui_demo_window = false;
    bool m_show_imgui_metrics_window = false;
    bool m_save_settings_to_disk = false;
};

class Texture;
class Rgba;
class Vector2;
class Vector4;
//Custom ImGui overloads
namespace ImGui {
void Image(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept;
void Image(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept;
[[nodiscard]] bool ImageButton(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept;
[[nodiscard]] bool ImageButton(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept;

[[nodiscard]] bool ColorEdit3(const char* label, Rgba& color, ImGuiColorEditFlags flags = 0) noexcept;
[[nodiscard]] bool ColorEdit4(const char* label, Rgba& color, ImGuiColorEditFlags flags = 0) noexcept;
[[nodiscard]] bool ColorPicker3(const char* label, Rgba& color, ImGuiColorEditFlags flags = 0) noexcept;
[[nodiscard]] bool ColorPicker4(const char* label, Rgba& color, ImGuiColorEditFlags flags = 0, Rgba* refColor = nullptr) noexcept;
[[nodiscard]] bool ColorButton(const char* desc_id, Rgba& color, ImGuiColorEditFlags flags = 0, Vector2 size = Vector2::Zero) noexcept;
void TextColored(const Rgba& color, const char* fmt, ...) noexcept;
} // namespace ImGui

namespace Clay {
Clay_Color RgbaToClayColor(Rgba color) noexcept;
Clay_String StrToClayString(std::string str) noexcept;
Clay_Dimensions Vector2ToClayDimensions(Vector2 v) noexcept;
Clay_Vector2 Vector2ToClayVector2(Vector2 v) noexcept;
}