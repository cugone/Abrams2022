#pragma once

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/Config.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/EngineBase.hpp"
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

#include "Engine/Platform/Win.hpp"
#include "Engine/Profiling/Instrumentor.hpp"

#include "Engine/RHI/RHIOutput.hpp"

#include "Engine/Services/IAudioService.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IConfigService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/IJobSystemService.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IPhysicsService.hpp"
#include "Engine/Services/IVideoService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include "Engine/System/System.hpp"

#include "Engine/UI/UISystem.hpp"

#include "Engine/Video/VideoSystem.hpp"

#include "Engine/Game/GameBase.hpp"

#include <algorithm>
#include <concepts>
#include <condition_variable>
#include <format>
#include <iomanip>
#include <memory>

template<GameType T>
class App : public EngineSubsystem, public IAppService {
public:
    App() noexcept = delete;
    explicit App(const std::string& title, const std::string& cmdString);
    App(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;
    virtual ~App() noexcept;

    static void CreateApp(const std::string& title) noexcept;
    static void DestroyApp() noexcept;

    void InitializeService() override;
    void RunFrame() override;

    bool IsQuitting() const override;
    void SetIsQuitting(bool value) override;

    bool HasFocus() const override;
    bool LostFocus() const override;
    bool GainedFocus() const override;


    void Minimize() const override;
    void Restore() const override;
    void Maximize() const override;

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

    bool m_isQuitting = false;
    bool m_current_focus = false;
    bool m_previous_focus = false;
    bool m_enteredSizeMove = false;
    bool m_doneSizeMove = false;

    std::string m_title{"UNTITLED GAME"};

    std::unique_ptr<JobSystem> m_theJobSystem{};
    std::unique_ptr<FileLogger> m_theFileLogger{};
    std::unique_ptr<Config> m_theConfig{};
    std::unique_ptr<Renderer> m_theRenderer{};
    std::unique_ptr<VideoSystem> m_theVideoSystem{};
    std::unique_ptr<Console> m_theConsole{};
    std::unique_ptr<PhysicsSystem> m_thePhysicsSystem{};
    std::unique_ptr<InputSystem> m_theInputSystem{};
    std::unique_ptr<UISystem> m_theUI{};
    std::unique_ptr<AudioSystem> m_theAudioSystem{};
    std::unique_ptr<T> m_theGame{};

    static inline std::unique_ptr<App<T>> m_theApp{};

    static inline NullAppService m_nullApp{};
    static inline NullJobSystemService m_nullJobSystem{};
    static inline NullFileLoggerService m_nullFileLogger{};
    static inline NullConfigService m_nullConfig{};
    static inline NullRendererService m_nullRenderer{};
    static inline NullVideoService m_nullVideoSystem{};
    static inline NullConsoleService m_nullConsole{};
    static inline NullPhysicsService m_nullPhysicsSystem{};
    static inline NullInputService m_nullInputSystem{};
    static inline NullAudioService m_nullAudioSystem{};
};

namespace detail {
    bool CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    EngineMessage GetEngineMessageFromWindowsParams(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

template<GameType T>
/*static*/ void App<T>::CreateApp(const std::string& title) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(m_theApp) {
        return;
    }
    const auto cmdStringW = GetCommandLineArgs();
    const auto cmdString = StringUtils::ConvertUnicodeToMultiByte(cmdStringW);
    m_theApp = std::make_unique<App<T>>(title, cmdString);
    ServiceLocator::provide(*static_cast<IAppService*>(m_theApp.get()), m_nullApp);
}

template<GameType T>
/*static*/ void App<T>::DestroyApp() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(!m_theApp) {
        return;
    }
    m_theApp.reset(nullptr);
}

template<GameType T>
App<T>::App(const std::string& title, const std::string& cmdString)
: EngineSubsystem()
, m_title{title}
, m_theConfig{std::make_unique<Config>(KeyValueParser{cmdString})} {
    PROFILE_BENCHMARK_FUNCTION();
    SetupEngineSystemPointers();
    SetupEngineSystemChainOfResponsibility();
    LogSystemDescription();
}

template<GameType T>
App<T>::~App() noexcept {
    if(g_theApp<T>) {
        g_theSubsystemHead = g_theApp<T>;
        m_theGame.reset();
        m_theAudioSystem.reset();
        m_theUI.reset();
        m_theInputSystem.reset();
        m_thePhysicsSystem.reset();
        m_theConsole.reset();
        m_theVideoSystem.reset();
        m_theRenderer.reset();
        m_theConfig.reset();
        m_theFileLogger.reset();
        m_theJobSystem.reset();
    }
    ServiceLocator::remove_all();
}

template<GameType T>
void App<T>::SetupEngineSystemPointers() {
    PROFILE_BENCHMARK_FUNCTION();
    ServiceLocator::provide(*static_cast<IConfigService*>(m_theConfig.get()), m_nullConfig);

    m_theJobSystem = std::make_unique<JobSystem>(-1, static_cast<std::size_t>(JobType::Max), std::move(std::make_unique<std::condition_variable>()));
    ServiceLocator::provide(*static_cast<IJobSystemService*>(m_theJobSystem.get()), m_nullJobSystem);

    m_theFileLogger = std::make_unique<FileLogger>("game");
    ServiceLocator::provide(*static_cast<IFileLoggerService*>(m_theFileLogger.get()), m_nullFileLogger);

    m_thePhysicsSystem = std::make_unique<PhysicsSystem>();
    ServiceLocator::provide(*static_cast<IPhysicsService*>(m_thePhysicsSystem.get()), m_nullPhysicsSystem);

    m_theRenderer = std::make_unique<Renderer>();
    ServiceLocator::provide(*static_cast<IRendererService*>(m_theRenderer.get()), m_nullRenderer);

    m_theVideoSystem = std::make_unique<VideoSystem>();
    ServiceLocator::provide(*static_cast<IVideoService*>(m_theVideoSystem.get()), m_nullVideoSystem);

    m_theInputSystem = std::make_unique<InputSystem>();
    ServiceLocator::provide(*static_cast<IInputService*>(m_theInputSystem.get()), m_nullInputSystem);

    m_theAudioSystem = std::make_unique<AudioSystem>();
    ServiceLocator::provide(*static_cast<IAudioService*>(m_theAudioSystem.get()), m_nullAudioSystem);

    m_theUI = std::make_unique<UISystem>();

    m_theConsole = std::make_unique<Console>();
    ServiceLocator::provide(*static_cast<IConsoleService*>(m_theConsole.get()), m_nullConsole);

    m_theGame = std::make_unique<T>();

    g_theJobSystem = m_theJobSystem.get();
    g_theFileLogger = m_theFileLogger.get();
    g_theConfig = m_theConfig.get();
    g_theRenderer = m_theRenderer.get();
    g_theVideoSystem = m_theVideoSystem.get();
    g_theUISystem = m_theUI.get();
    g_theConsole = m_theConsole.get();
    g_thePhysicsSystem = m_thePhysicsSystem.get();
    g_theInputSystem = m_theInputSystem.get();
    g_theAudioSystem = m_theAudioSystem.get();
    g_theGame = m_theGame.get();
    g_theApp<T> = this;
}

template<GameType T>
void App<T>::SetupEngineSystemChainOfResponsibility() {
    PROFILE_BENCHMARK_FUNCTION();
    g_theConsole->SetNextHandler(g_theUISystem);
    g_theUISystem->SetNextHandler(g_theInputSystem);
    g_theInputSystem->SetNextHandler(g_thePhysicsSystem);
    g_thePhysicsSystem->SetNextHandler(g_theRenderer);
    g_theRenderer->SetNextHandler(g_theApp<T>);
    g_theApp<T>->SetNextHandler(nullptr);
    g_theSubsystemHead = g_theConsole;
}

template<GameType T>
void App<T>::Initialize() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
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
    g_theVideoSystem->Initialize();
    g_theRenderer->SetVSync(vsync);
    g_theRenderer->SetWinProc(detail::WindowProc);
    auto* output = g_theRenderer->GetOutput();
    output->SetTitle(m_title);

    g_theUISystem->Initialize();
    g_theInputSystem->Initialize();
    g_theConsole->Initialize();
    g_theAudioSystem->Initialize();
    g_theVideoSystem->Initialize();
    g_thePhysicsSystem->Initialize();
    g_theGame->Initialize();
}

template<GameType T>
void App<T>::InitializeService() {
    Initialize();
}

template<GameType T>
void App<T>::BeginFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    g_theJobSystem->BeginFrame();
    g_theUISystem->BeginFrame();
    g_theInputSystem->BeginFrame();
    g_theConsole->BeginFrame();
    g_theAudioSystem->BeginFrame();
    g_theVideoSystem->BeginFrame();
    g_thePhysicsSystem->BeginFrame();
    g_theGame->BeginFrame();
    g_theRenderer->BeginFrame();
}

template<GameType T>
void App<T>::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    g_theUISystem->Update(deltaSeconds);
    g_theInputSystem->Update(deltaSeconds);
    g_theConsole->Update(deltaSeconds);
    g_theAudioSystem->Update(deltaSeconds);
    g_theVideoSystem->Update(deltaSeconds);
    g_thePhysicsSystem->Update(deltaSeconds);
    g_theGame->Update(deltaSeconds);
    g_theRenderer->Update(deltaSeconds);
}

template<GameType T>
void App<T>::Render() const noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    g_theGame->Render();
    g_theUISystem->Render();
    g_theConsole->Render();
    g_theAudioSystem->Render();
    g_theInputSystem->Render();
    g_theVideoSystem->Render();
    g_thePhysicsSystem->Render();
    g_theRenderer->Render();
}

template<GameType T>
void App<T>::EndFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    g_theUISystem->EndFrame();
    g_theGame->EndFrame();
    g_theConsole->EndFrame();
    g_theAudioSystem->EndFrame();
    g_theInputSystem->EndFrame();
    g_thePhysicsSystem->EndFrame();
    g_theVideoSystem->EndFrame();
    g_theRenderer->EndFrame();
}

template<GameType T>
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
            m_current_focus = false;
            m_previous_focus = true;
        }
        if(gaining_focus) {
            m_current_focus = true;
            m_previous_focus = false;
        }
        return false;
    }
    case WindowsSystemMessage::Keyboard_Activate: {
        WPARAM wp = msg.wparam;
        auto active_type = LOWORD(wp);
        switch(active_type) {
        case WA_ACTIVE:
            [[fallthrough]];
        case WA_CLICKACTIVE:
            m_current_focus = true;
            m_previous_focus = false;
            return true;
        case WA_INACTIVE:
            m_current_focus = false;
            m_previous_focus = true;
            return true;
        default:
            return false;
        }
    }
    case WindowsSystemMessage::Window_EnterSizeMove:
    {
        m_enteredSizeMove = true;
        return false;
    }
    case WindowsSystemMessage::Window_ExitSizeMove:
    {
        if(m_enteredSizeMove) {
            m_doneSizeMove = true;
            return false;
        }
        return false;
    }
    case WindowsSystemMessage::Window_Size:
    {
        const auto size_type = EngineSubsystem::GetResizeTypeFromWmSize(msg);
        switch(size_type) {
        case WindowResizeType::Maximized:
            return true;
        case WindowResizeType::Minimized:
            return true;
        case WindowResizeType::Restored:
            if(m_enteredSizeMove && m_doneSizeMove) {
                m_enteredSizeMove = false;
                m_doneSizeMove = false;
                LPARAM lp = msg.lparam;
                const auto w = HIWORD(lp);
                const auto h = LOWORD(lp);
                HandleResize(w, h);
                return true;
            }
            break;
        case WindowResizeType::MaxHide:
            return false;
        case WindowResizeType::MaxShow:
            return false;
        default:
            break;
        }
        return false;
    }
    default:
        return false;
    }
}

template<GameType T>
bool App<T>::IsQuitting() const {
    return m_isQuitting;
}

template<GameType T>
void App<T>::SetIsQuitting(bool value) {
    m_isQuitting = value;
}

template<GameType T>
void App<T>::RunFrame() {
    RunMessagePump();

    BeginFrame();

    static TimeUtils::FPMilliseconds accumulator{};
    static TimeUtils::FPSeconds previousFrameTime = TimeUtils::GetCurrentTimeElapsed();
    TimeUtils::FPSeconds currentFrameTime = TimeUtils::GetCurrentTimeElapsed();
    TimeUtils::FPSeconds deltaSeconds = (currentFrameTime - previousFrameTime);
    previousFrameTime = currentFrameTime;
    accumulator += deltaSeconds;

    while(accumulator >= deltaSeconds) {
        Update(deltaSeconds);
        accumulator -= deltaSeconds;
    }

    Render();
    EndFrame();
    AllocationTracker::tick();
}

template<GameType T>
void App<T>::LogSystemDescription() const {
    PROFILE_BENCHMARK_FUNCTION();
    const auto system = System::GetSystemDesc();
    g_theFileLogger->LogAndFlush(std::format("{:->80}{}{:->80}", '\n', StringUtils::to_string(system), '\n'));
}

template<GameType T>
bool App<T>::HasFocus() const {
    return m_current_focus;
}

template<GameType T>
bool App<T>::LostFocus() const {
    return m_previous_focus && !m_current_focus;
}

template<GameType T>
bool App<T>::GainedFocus() const {
    return !m_previous_focus && m_current_focus;
}

template<GameType T>
void App<T>::Minimize() const {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetWindowedMode();
    auto* hwnd = reinterpret_cast<HWND*>(renderer->GetOutput()->GetWindow()->GetWindowHandle());
    ::SendMessageA(*hwnd, WM_SIZE, SIZE_MINIMIZED, MAKELPARAM(0, 0) );
}

template<GameType T>
void App<T>::Restore() const {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetWindowedMode();
}

template<GameType T>
void App<T>::Maximize() const {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetWindowedMode();
    auto* window = renderer->GetOutput()->GetWindow();
    auto* hwnd = reinterpret_cast<HWND*>(window->GetWindowHandle());
    const auto desktop_resolution = window->GetDesktopResolution();
    ::SendMessageA(*hwnd, WM_SIZE, SIZE_MAXIMIZED, MAKELPARAM(desktop_resolution.x, desktop_resolution.y));
}

template<GameType T>
void App<T>::RunMessagePump() const {
    MSG msg{};
    for(;;) {
        const BOOL hasMsg = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if(!hasMsg) {
            break;
        }
        const auto* output = g_theRenderer->GetOutput();
        const auto* window = output->GetWindow();
        auto hwnd = reinterpret_cast<HWND>(window->GetWindowHandle());
        auto console_accel_table = reinterpret_cast<HACCEL>(g_theConsole->GetAcceleratorTable());
        if(!::TranslateAcceleratorA(hwnd, console_accel_table, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

template<GameType T>
void App<T>::HandleResize(unsigned int newWidth, unsigned int newHeight) noexcept {
    g_theRenderer->ResizeBuffers();
    GetGameAs<T>()->HandleWindowResize(newWidth, newHeight);
}

