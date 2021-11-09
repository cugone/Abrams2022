#pragma once

#include <type_traits>

class JobSystem;
class FileLogger;
class Renderer;
class Console;
class Config;
class UISystem;
class InputSystem;
class PhysicsSystem;
class AudioSystem;
class EngineSubsystem;
class GameBase;

template<typename GameType>
class App;

inline JobSystem* g_theJobSystem = nullptr;
inline FileLogger* g_theFileLogger = nullptr;
inline Renderer* g_theRenderer = nullptr;
inline Console* g_theConsole = nullptr;
inline Config* g_theConfig = nullptr;
inline UISystem* g_theUISystem = nullptr;
inline InputSystem* g_theInputSystem = nullptr;
inline AudioSystem* g_theAudioSystem = nullptr;
inline PhysicsSystem* g_thePhysicsSystem = nullptr;
inline GameBase* g_theGame = nullptr;
inline EngineSubsystem* g_theSubsystemHead = nullptr;

template<typename GameType>
inline App<GameType>* g_theApp = nullptr;

template<typename GameDerived>
GameDerived* GetGameAs() noexcept {
    static_assert(std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameBase>>>, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameDerived>>>>);
    return dynamic_cast<GameDerived*>(g_theGame);
}
