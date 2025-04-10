#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include <Thirdparty/clay/clay.h>

#include <functional>
#include <string>
#include <memory>

class KerningFont;

class ClayUI {
public:
    ClayUI() = default;
    ClayUI(const ClayUI& other) = default;
    ClayUI(ClayUI&& other) = default;
    ClayUI& operator=(const ClayUI& other) = default;
    ClayUI& operator=(ClayUI&& other) = default;
    ~ClayUI() noexcept = default;

    void Initialize() noexcept;
    void BeginFrame() noexcept;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render() const noexcept;
    void EndFrame() noexcept;

    
    void SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept;
    [[nodiscard]] bool IsClayDebugWindowVisible() const noexcept;
    void ToggleClayDebugWindow() noexcept;

protected:
private:
    mutable Camera2D m_ui_camera{};
    std::function<void()> m_clayLayoutCallback{};
    std::function<void(void*)> m_clayCustomCallback{};
    mutable Clay_RenderCommandArray m_clay_commands{};
    Clay_Context* m_clayContext{};
    std::unique_ptr<char[]> m_clayMemoryBlock{};
    float m_clayScrollSpeed{10.0f};
    bool m_show_clay_debug_window{false};
};

namespace Clay {
Clay_Color RgbaToClayColor(Rgba color) noexcept;
Clay_String StrToClayString(const std::string& str) noexcept;
Clay_Dimensions Vector2ToClayDimensions(Vector2 v) noexcept;
Clay_Vector2 Vector2ToClayVector2(Vector2 v) noexcept;
Rgba ClayColorToRgba(Clay_Color textColor) noexcept;
} // namespace Clay