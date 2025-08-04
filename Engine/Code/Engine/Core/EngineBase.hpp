#pragma once

#include "Engine/Core/App.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Platform/Win.hpp"

#include "Engine/Game/GameBase.hpp"

#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"


#ifdef PROFILE_BUILD
    #include <Thirdparty/Tracy/tracy/Tracy.hpp>
#endif

#include <concepts>
#include <string>
#include <type_traits>

template<GameType T>
class Engine {
public:
    static void Initialize(const std::string& title) noexcept;
    static void Run() noexcept;
    static void Shutdown() noexcept;
    static const bool Available() noexcept;

private:
    static inline bool m_initCalled{false};
    static inline bool m_shutdownCalled{false};
};

template<GameType T>
/*static*/
const bool Engine<T>::Available() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    return m_initCalled && !m_shutdownCalled;
}

template<GameType T>
/*static*/
void Engine<T>::Initialize(const std::string& title) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    if(!m_initCalled) {
        m_shutdownCalled = false;
        m_initCalled = true;
        App<T>::CreateApp(title);
        auto* app = ServiceLocator::get<IAppService>();
        app->InitializeService();
    }
}

template<GameType T>
/*static*/
void Engine<T>::Run() noexcept {
    GUARANTEE_OR_DIE(!m_shutdownCalled, "Engine::Shutdown called before Run!");
    GUARANTEE_OR_DIE(m_initCalled, "Engine::Initialize not called before Run");
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    auto* app = ServiceLocator::get<IAppService>();
    while(!app->IsQuitting()) {
        app->RunFrame();
    }
}

template<GameType T>
/*static*/
void Engine<T>::Shutdown() noexcept {
    GUARANTEE_OR_DIE(m_initCalled, "Engine::Initialize not called before Shutdown");
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    if(!m_shutdownCalled) {
        m_shutdownCalled = true;
        m_initCalled = false;
        App<T>::DestroyApp();
    }
}
