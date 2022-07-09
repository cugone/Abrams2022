#include "Engine/Game/GameBase.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

GameBase::~GameBase() noexcept {
    m_ActiveScene.reset();
}

void GameBase::Initialize() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void GameBase::BeginFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void GameBase::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void GameBase::Render() const noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void GameBase::EndFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void GameBase::HandleWindowResize([[maybe_unused]] unsigned int newWidth, [[maybe_unused]] unsigned int newHeight) noexcept {
    /* DO NOTHING */
}

const GameSettings& GameBase::GetSettings() const noexcept {
    return defaultSettings;
}

GameSettings& GameBase::GetSettings() noexcept {
    return defaultSettings;
}

std::weak_ptr<Scene> GameBase::GetActiveScene() const noexcept {
    return m_ActiveScene;
}
