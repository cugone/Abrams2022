#include "Engine/UI/UISystem.hpp"

#include "Engine/Core/BuildConfig.hpp"

#ifdef PROFILE_BUILD
#include <Thirdparty/Tracy/tracy/Tracy.hpp>
#endif

UISystem::UISystem() noexcept
: EngineSubsystem()
{
    /* DO NOTHING */
}

void UISystem::Initialize() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.Initialize();
    m_clay.Initialize();
}

void UISystem::SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_clay.SetClayLayoutCallback(std::move(layoutCallback));
}

bool UISystem::IsClayDebugWindowVisible() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_clay.IsClayDebugWindowVisible();
}

void UISystem::ToggleClayDebugWindow() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_clay.ToggleClayDebugWindow();
}

void UISystem::BeginFrame() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.BeginFrame();
    m_clay.BeginFrame();
}

void UISystem::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.Update();
    m_clay.Update(deltaSeconds);
}

void UISystem::Render() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.Render();
    m_clay.Render();
}

void UISystem::EndFrame() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.EndFrame();
    m_clay.EndFrame();
}

bool UISystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.ProcessSystemMessage(msg);
}

bool UISystem::HasFocus() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.HasFocus();
}

bool UISystem::WantsInputCapture() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.WantsInputKeyboardCapture() || m_imgui.WantsInputMouseCapture();
}

bool UISystem::WantsInputKeyboardCapture() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.WantsInputKeyboardCapture();
}

bool UISystem::WantsInputMouseCapture() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.WantsInputMouseCapture();
}

bool UISystem::IsImguiDemoWindowVisible() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.IsImguiDemoWindowVisible();
}

void UISystem::ToggleImguiDemoWindow() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.ToggleImguiDemoWindow();
}

bool UISystem::IsImguiMetricsWindowVisible() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.IsImguiMetricsWindowVisible();
}

void UISystem::ToggleImguiMetricsWindow() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    m_imgui.ToggleImguiMetricsWindow();
}

bool UISystem::IsAnyImguiDebugWindowVisible() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_imgui.IsAnyImguiDebugWindowVisible();
}

bool UISystem::IsAnyDebugWindowVisible() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return IsAnyImguiDebugWindowVisible() || IsClayDebugWindowVisible();
}
