#pragma once

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/Config.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Engine/Physics/PhysicsSystem.hpp"
#include "Engine/Profiling/AllocationTracker.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/IAudioService.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IConfigService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/IJobSystemService.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IPhysicsService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include "Engine/System/System.hpp"

#include "Engine/UI/UISystem.hpp"

#include "Engine/Game/GameBase.hpp"

#include <algorithm>
#include <condition_variable>
#include <iomanip>
#include <memory>

template<typename T>
class App : public EngineSubsystem, public IAppService {
public:
    App() noexcept = default;
    explicit App(const std::string& title, const std::string& cmdString);
    App(const App& other) = default;
    App(App&& other) = default;
    App& operator=(const App& other) = default;
    App& operator=(App&& other) = default;

    using GameType = T;
    static_assert(std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameBase>>>, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<GameType>>>>);

    virtual ~App() noexcept;

    static void CreateApp(const std::string& title, const std::string& cmdString) noexcept;
    static void DestroyApp() noexcept;

    void InitializeService() override;
    void RunFrame() override;

    bool IsQuitting() const override;
    void SetIsQuitting(bool value) override;

    bool HasFocus() const override;
    bool LostFocus() const override;
    bool GainedFocus() const override;

protected:
private:
    void RunMessagePump() const;

    void SetupEngineSystemPointers();
    void SetupEngineSystemChainOfResponsibility();

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;
    bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;
    void HandleResize([[maybe_unused]] unsigned int newWidth, [[maybe_unused]] unsigned int newHeight) noexcept;

    void LogSystemDescription() const;

    bool _isQuitting = false;
    bool _current_focus = false;
    bool _previous_focus = false;
    bool _enteredSizeMove = false;
    bool _doneSizeMove = false;

    std::string _title{"UNTITLED GAME"};

    std::unique_ptr<JobSystem> _theJobSystem{};
    std::unique_ptr<FileLogger> _theFileLogger{};
    std::unique_ptr<Config> _theConfig{};
    std::unique_ptr<Renderer> _theRenderer{};
    std::unique_ptr<Console> _theConsole{};
    std::unique_ptr<PhysicsSystem> _thePhysicsSystem{};
    std::unique_ptr<InputSystem> _theInputSystem{};
    std::unique_ptr<UISystem> _theUI{};
    std::unique_ptr<AudioSystem> _theAudioSystem{};
    std::unique_ptr<GameType> _theGame{};

    static inline std::unique_ptr<App<GameType>> _theApp{};

    static inline NullAppService _nullApp{};
};

namespace detail {
    bool CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    EngineMessage GetEngineMessageFromWindowsParams(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

template<typename T>
/*static*/ void App<T>::CreateApp(const std::string& title, const std::string& cmdString) noexcept {
    if(_theApp) {
        return;
    }
    _theApp = std::make_unique<App<T>>(title, cmdString);
    ServiceLocator::provide(*static_cast<IAppService*>(_theApp.get()));
}

template<typename T>
/*static*/ void App<T>::DestroyApp() noexcept {
    if(!_theApp) {
        return;
    }
    _theApp.reset(nullptr);
}

template<typename T>
App<T>::App(const std::string& title, const std::string& cmdString)
: EngineSubsystem()
, _title{title}
, _theConfig{std::make_unique<Config>(KeyValueParser{cmdString})} {
    SetupEngineSystemPointers();
    SetupEngineSystemChainOfResponsibility();
    LogSystemDescription();
}

template<typename T>
App<T>::~App() noexcept {
    if(g_theApp<T>) {
        g_theSubsystemHead = g_theApp<T>;
    }
}

template<typename T>
void App<T>::SetupEngineSystemPointers() {
    ServiceLocator::provide(*static_cast<IConfigService*>(_theConfig.get()));

    _theJobSystem = std::make_unique<JobSystem>(-1, static_cast<std::size_t>(JobType::Max), new std::condition_variable);
    ServiceLocator::provide(*static_cast<IJobSystemService*>(_theJobSystem.get()));

    _theFileLogger = std::make_unique<FileLogger>("game");
    ServiceLocator::provide(*static_cast<IFileLoggerService*>(_theFileLogger.get()));

    _thePhysicsSystem = std::make_unique<PhysicsSystem>();
    ServiceLocator::provide(*static_cast<IPhysicsService*>(_thePhysicsSystem.get()));

    _theRenderer = std::make_unique<Renderer>();
    ServiceLocator::provide(*static_cast<IRendererService*>(_theRenderer.get()));

    _theInputSystem = std::make_unique<InputSystem>();
    ServiceLocator::provide(*static_cast<IInputService*>(_theInputSystem.get()));

    _theAudioSystem = std::make_unique<AudioSystem>();
    ServiceLocator::provide(*static_cast<IAudioService*>(_theAudioSystem.get()));

    _theUI = std::make_unique<UISystem>();
    _theConsole = std::make_unique<Console>();
    _theGame = std::make_unique<GameType>();

    g_theJobSystem = _theJobSystem.get();
    g_theFileLogger = _theFileLogger.get();
    g_theConfig = _theConfig.get();
    g_theRenderer = _theRenderer.get();
    g_theUISystem = _theUI.get();
    g_theConsole = _theConsole.get();
    g_thePhysicsSystem = _thePhysicsSystem.get();
    g_theInputSystem = _theInputSystem.get();
    g_theAudioSystem = _theAudioSystem.get();
    g_theGame = _theGame.get();
    g_theApp<T> = this;
}

template<typename T>
void App<T>::SetupEngineSystemChainOfResponsibility() {
    g_theConsole->SetNextHandler(g_theUISystem);
    g_theUISystem->SetNextHandler(g_theInputSystem);
    g_theInputSystem->SetNextHandler(g_thePhysicsSystem);
    g_thePhysicsSystem->SetNextHandler(g_theRenderer);
    g_theRenderer->SetNextHandler(g_theApp<T>);
    g_theApp<T>->SetNextHandler(nullptr);
    g_theSubsystemHead = g_theConsole;
}

template<typename T>
void App<T>::Initialize() noexcept {
    auto& settings = g_theGame->GetSettings();

    bool vsync = settings.DefaultVsyncEnabled();
    if(g_theConfig->HasKey("vsync")) {
        g_theConfig->GetValue(std::string{"vsync"}, vsync);
    } else {
        g_theConfig->SetValue(std::string{"vsync"}, vsync);
    }
    settings.SetVsyncEnabled(vsync);

    int width = settings.DefaultWindowWidth();
    int height = settings.DefaultWindowHeight();
    if(g_theConfig->HasKey("width")) {
        g_theConfig->GetValue(std::string{"width"}, width);
    } else {
        g_theConfig->SetValue(std::string{"width"}, width);
    }
    if(g_theConfig->HasKey("height")) {
        g_theConfig->GetValue(std::string{"height"}, height);
    } else {
        g_theConfig->SetValue(std::string{"height"}, height);
    }
    settings.SetWindowResolution(IntVector2{width, height});

    g_theRenderer->Initialize();
    g_theRenderer->SetVSync(vsync);
    auto* output = g_theRenderer->GetOutput();
    output->SetTitle(_title);
    output->GetWindow()->custom_message_handler = detail::WindowProc;

    g_theUISystem->Initialize();
    g_theInputSystem->Initialize();
    g_theConsole->Initialize();
    g_theAudioSystem->Initialize();
    g_thePhysicsSystem->Initialize();
    g_theGame->Initialize();
}

template<typename T>
void App<T>::InitializeService() {
    Initialize();
}

template<typename T>
void App<T>::BeginFrame() noexcept {
    g_theJobSystem->BeginFrame();
    g_theUISystem->BeginFrame();
    g_theInputSystem->BeginFrame();
    g_theConsole->BeginFrame();
    g_theAudioSystem->BeginFrame();
    g_thePhysicsSystem->BeginFrame();
    g_theGame->BeginFrame();
    g_theRenderer->BeginFrame();
}

template<typename T>
void App<T>::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    g_theUISystem->Update(deltaSeconds);
    g_theInputSystem->Update(deltaSeconds);
    g_theConsole->Update(deltaSeconds);
    g_theAudioSystem->Update(deltaSeconds);
    g_thePhysicsSystem->Update(deltaSeconds);
    g_theGame->Update(deltaSeconds);
    g_theRenderer->Update(deltaSeconds);
}

template<typename T>
void App<T>::Render() const noexcept {
    g_theGame->Render();
    g_theUISystem->Render();
    g_theConsole->Render();
    g_theAudioSystem->Render();
    g_theInputSystem->Render();
    g_thePhysicsSystem->Render();
    g_theRenderer->Render();
}

template<typename T>
void App<T>::EndFrame() noexcept {
    g_theUISystem->EndFrame();
    g_theGame->EndFrame();
    g_theConsole->EndFrame();
    g_theAudioSystem->EndFrame();
    g_theInputSystem->EndFrame();
    g_thePhysicsSystem->EndFrame();
    g_theRenderer->EndFrame();
}

template<typename T>
bool App<T>::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    switch(msg.wmMessageCode) {
    case WindowsSystemMessage::Window_Close: {
        SetIsQuitting(true);
        return true;
    }
    case WindowsSystemMessage::Window_Quit: {
        SetIsQuitting(true);
        return true;
    }
    case WindowsSystemMessage::Window_Destroy: {
        ::PostQuitMessage(0);
        return true;
    }
    case WindowsSystemMessage::Window_ActivateApp: {
        WPARAM wp = msg.wparam;
        bool losing_focus = wp == FALSE;
        bool gaining_focus = wp == TRUE;
        if(losing_focus) {
            _current_focus = false;
            _previous_focus = true;
        }
        if(gaining_focus) {
            _current_focus = true;
            _previous_focus = false;
        }
        return false;
    }
    case WindowsSystemMessage::Keyboard_Activate: {
        WPARAM wp = msg.wparam;
        auto active_type = LOWORD(wp);
        switch(active_type) {
        case WA_ACTIVE: /* FALLTHROUGH */
        case WA_CLICKACTIVE:
            _current_focus = true;
            _previous_focus = false;
            return true;
        case WA_INACTIVE:
            _current_focus = false;
            _previous_focus = true;
            return true;
        default:
            return false;
        }
    }
    case WindowsSystemMessage::Window_EnterSizeMove:
    {
        _enteredSizeMove = true;
        return true;
    }
    case WindowsSystemMessage::Window_ExitSizeMove:
    {
        if(_enteredSizeMove) {
            _doneSizeMove = true;
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Window_Size:
    {
        if(_enteredSizeMove && _doneSizeMove) {
            _enteredSizeMove = false;
            _doneSizeMove = false;
            LPARAM lp = msg.lparam;
            const auto w = HIWORD(lp);
            const auto h = LOWORD(lp);
            HandleResize(w, h);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

template<typename T>
bool App<T>::IsQuitting() const {
    return _isQuitting;
}

template<typename T>
void App<T>::SetIsQuitting(bool value) {
    _isQuitting = value;
}

template<typename T>
void App<T>::RunFrame() {
    using namespace TimeUtils;

    RunMessagePump();

    BeginFrame();

    static FPSeconds previousFrameTime = TimeUtils::GetCurrentTimeElapsed();
    FPSeconds currentFrameTime = TimeUtils::GetCurrentTimeElapsed();
    FPSeconds deltaSeconds = (currentFrameTime - previousFrameTime);
    previousFrameTime = currentFrameTime;

#ifdef DEBUG_BUILD
    deltaSeconds = (std::min)(FPSeconds{FPFrames{1}}, deltaSeconds);
#endif

    Update(deltaSeconds);
    Render();
    EndFrame();
    AllocationTracker::tick();
}

template<typename T>
void App<T>::LogSystemDescription() const {
    const auto section_break_field_width = std::size_t{80u};
    const auto system = System::GetSystemDesc();
    std::ostringstream ss;
    ss << std::right << std::setfill('-') << std::setw(section_break_field_width) << '\n';
    ss << StringUtils::to_string(system);
    ss << std::right << std::setfill('-') << std::setw(section_break_field_width) << '\n';
    g_theFileLogger->LogAndFlush(ss.str());
}

template<typename T>
bool App<T>::HasFocus() const {
    return _current_focus;
}

template<typename T>
bool App<T>::LostFocus() const {
    return _previous_focus && !_current_focus;
}

template<typename T>
bool App<T>::GainedFocus() const {
    return !_previous_focus && _current_focus;
}

template<typename T>
void App<T>::RunMessagePump() const {
    MSG msg{};
    for(;;) {
        const BOOL hasMsg = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if(!hasMsg) {
            break;
        }
        if(!::TranslateAcceleratorA(reinterpret_cast<HWND>(g_theRenderer->GetOutput()->GetWindow()->GetWindowHandle()), reinterpret_cast<HACCEL>(g_theConsole->GetAcceleratorTable()), &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

template<typename T>
void App<T>::HandleResize(unsigned int newWidth, unsigned int newHeight) noexcept {
    g_theRenderer->ResizeBuffers();
    GetGameAs<T>()->HandleWindowResize(newWidth, newHeight);
}

