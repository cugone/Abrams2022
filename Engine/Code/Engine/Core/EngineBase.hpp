#pragma once

#include "Engine/Core/App.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Game/GameBase.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"

template<typename GameType>
class Engine {
public:
    static_assert(std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameBase>>>, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameType>>>>, "GameType template parameter is not derived from GameBase.");
    static void Initialize(const std::string& title, const std::string& cmdString) noexcept;
    static void Run() noexcept;
    static void Shutdown() noexcept;
    static const bool Available() noexcept;

private:
    static inline bool m_initCalled{false};
    static inline bool m_shutdownCalled{false};
};

template<typename GameType>
/*static*/
const bool Engine<GameType>::Available() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    return m_initCalled && !m_shutdownCalled;
}

template<typename GameType>
/*static*/
void Engine<GameType>::Initialize(const std::string& title, const std::string& cmdString) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(!m_initCalled) {
        m_shutdownCalled = false;
        m_initCalled = true;
        App<GameType>::CreateApp(title, cmdString);
        auto* app = ServiceLocator::get<IAppService>();
        app->InitializeService();
    }
}

template<typename GameType>
/*static*/
void Engine<GameType>::Run() noexcept {
    GUARANTEE_OR_DIE(!m_shutdownCalled, "Engine::Shutdown called before Run!");
    GUARANTEE_OR_DIE(m_initCalled, "Engine::Initialize not called before Run");
    PROFILE_BENCHMARK_FUNCTION();
    auto* app = ServiceLocator::get<IAppService>();
    while(!app->IsQuitting()) {
        app->RunFrame();
    }
}

template<typename GameType>
/*static*/
void Engine<GameType>::Shutdown() noexcept {
    GUARANTEE_OR_DIE(m_initCalled, "Engine::Initialize not called before Shutdown");
    PROFILE_BENCHMARK_FUNCTION();
    if(!m_shutdownCalled) {
        m_shutdownCalled = true;
        m_initCalled = false;
        App<GameType>::DestroyApp();
    }
}
