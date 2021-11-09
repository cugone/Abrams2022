#pragma once

#include "Engine/Core/App.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Game/GameBase.hpp"

#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"

[[nodiscard]] Window* GetWindowFromHwnd(HWND hwnd);

LRESULT CALLBACK EngineMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam);

void RunMessagePump() noexcept;

template<typename GameType>
class Engine {
public:
    static_assert(std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameBase>>>, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameType>>>>, "GameType template parameter is not derived from GameBase.");
    static void Initialize(const std::string& title, const std::string& cmdString) noexcept;
    static void Run() noexcept;
    static void Shutdown() noexcept;
private:
    static inline bool _initCalled{false};
    static inline bool _shutdownCalled{false};
};

template<typename GameType>
/*static*/
void Engine<GameType>::Initialize(const std::string& title, const std::string& cmdString) noexcept {
    _initCalled = true;
    App<GameType>::CreateApp(title, cmdString);
    auto& app = ServiceLocator::get<IAppService>();
    app.InitializeService();
}

template<typename GameType>
/*static*/
void Engine<GameType>::Run() noexcept {
    GUARANTEE_OR_DIE(_initCalled, "Engine::Initialize not called before Run");
    auto& app = ServiceLocator::get<IAppService>();
    while(!app.IsQuitting()) {
        app.RunFrame();
    }
}

template<typename GameType>
/*static*/
void Engine<GameType>::Shutdown() noexcept {
    GUARANTEE_OR_DIE(_initCalled, "Engine::Initialize not called before Shutdown");
    if(!_shutdownCalled) {
        _shutdownCalled = true;
        App<GameType>::DestroyApp();
    }
}
