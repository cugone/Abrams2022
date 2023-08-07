#pragma once

#include <concepts>
#include <type_traits>

class JobSystem;
class FileLogger;
class Renderer;
class VideoSystem;
class Console;
class Config;
class UISystem;
class InputSystem;
class PhysicsSystem;
class AudioSystem;
class EngineSubsystem;
class GameBase;


template<typename T>
concept GameType = std::derived_from<T, GameBase>;

template<GameType T>
class App;

inline JobSystem* g_theJobSystem = nullptr;
inline FileLogger* g_theFileLogger = nullptr;
inline Renderer* g_theRenderer = nullptr;
inline VideoSystem* g_theVideoSystem = nullptr;
inline Console* g_theConsole = nullptr;
inline Config* g_theConfig = nullptr;
inline UISystem* g_theUISystem = nullptr;
inline InputSystem* g_theInputSystem = nullptr;
inline AudioSystem* g_theAudioSystem = nullptr;
inline PhysicsSystem* g_thePhysicsSystem = nullptr;
inline GameBase* g_theGame = nullptr;
inline EngineSubsystem* g_theSubsystemHead = nullptr;

template<GameType T>
inline App<T>* g_theApp = nullptr;

template<GameType T>
T* GetGameAs() noexcept {
    return dynamic_cast<T*>(g_theGame);
}
