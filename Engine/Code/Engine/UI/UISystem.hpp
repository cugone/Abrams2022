#pragma once

#include "Engine/Core/EngineSubsystem.hpp"

#include "Engine/UI/DearImgui.hpp"
#include "Engine/UI/ClayUI.hpp"

#include <functional>

//TODO: Maybe make Service
class UISystem : public EngineSubsystem {
public:
    UISystem() noexcept;
    UISystem(const UISystem& other) = default;
    UISystem(UISystem&& other) = default;
    UISystem& operator=(const UISystem& other) = default;
    UISystem& operator=(UISystem&& other) = default;
    virtual ~UISystem() noexcept = default;
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
    [[nodiscard]] bool IsClayDebugWindowVisible() const noexcept;
    void ToggleClayDebugWindow() noexcept;

    [[nodiscard]] bool IsAnyDebugWindowVisible() const noexcept;

protected:
private:
    DearImgui m_imgui{};
    ClayUI m_clay{};
};
