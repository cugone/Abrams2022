#include "Engine/UI/UISystem.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Input/KeyCode.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

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

#include <Thirdparty/Imgui/imgui_internal.h>
#include <Thirdparty/clay/clay.h>

#include <algorithm>

UISystem::UISystem() noexcept
: EngineSubsystem()
{
    /* DO NOTHING */
}

void UISystem::Initialize() noexcept {
    m_imgui.Initialize();
    m_clay.Initialize();
}

void UISystem::SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept {
    m_clay.SetClayLayoutCallback(std::move(layoutCallback));
}

bool UISystem::IsClayDebugWindowVisible() const noexcept {
    return m_clay.IsClayDebugWindowVisible();
}

void UISystem::ToggleClayDebugWindow() noexcept {
    m_clay.ToggleClayDebugWindow();
}

void UISystem::BeginFrame() noexcept {
    m_imgui.BeginFrame();
    m_clay.BeginFrame();
}

void UISystem::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_imgui.Update();
    m_clay.Update(deltaSeconds);
}

void UISystem::Render() const noexcept {
    m_imgui.Render();
    m_clay.Render();
}

void UISystem::EndFrame() noexcept {
    m_imgui.EndFrame();
    m_clay.EndFrame();
}

bool UISystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    return m_imgui.ProcessSystemMessage(msg);
}

bool UISystem::HasFocus() const noexcept {
    return m_imgui.HasFocus();
}

bool UISystem::WantsInputCapture() const noexcept {
    return m_imgui.WantsInputKeyboardCapture() || m_imgui.WantsInputMouseCapture();
}

bool UISystem::WantsInputKeyboardCapture() const noexcept {
    return m_imgui.WantsInputKeyboardCapture();
}

bool UISystem::WantsInputMouseCapture() const noexcept {
    return m_imgui.WantsInputMouseCapture();
}

bool UISystem::IsImguiDemoWindowVisible() const noexcept {
    return m_imgui.IsImguiDemoWindowVisible();
}

void UISystem::ToggleImguiDemoWindow() noexcept {
    m_imgui.ToggleImguiDemoWindow();
}

bool UISystem::IsImguiMetricsWindowVisible() const noexcept {
    return m_imgui.IsImguiMetricsWindowVisible();
}

void UISystem::ToggleImguiMetricsWindow() noexcept {
    m_imgui.ToggleImguiMetricsWindow();
}

bool UISystem::IsAnyImguiDebugWindowVisible() const noexcept {
    return m_imgui.IsAnyImguiDebugWindowVisible();
}

bool UISystem::IsAnyDebugWindowVisible() const noexcept {
    return IsAnyImguiDebugWindowVisible() || IsClayDebugWindowVisible();
}
