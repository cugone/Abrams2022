#pragma once

#include "Engine/Services/IService.hpp"

#include <filesystem>

class Audio3DListener;
class Audio3DEmitter;

class IAudioService : public IService {
public:
    virtual ~IAudioService() noexcept { /* DO NOTHING */ };
    virtual void SuspendAudio() noexcept = 0;
    virtual void ResumeAudio() noexcept = 0;

    virtual void Play(const std::filesystem::path& filepath) noexcept = 0;
    virtual void Play(const std::size_t id) noexcept = 0;
    virtual void Play(const std::filesystem::path& filepath, const bool looping) noexcept = 0;
    virtual void Play(const std::size_t id, const bool looping) noexcept = 0;
    
    virtual void Stop(const std::filesystem::path& filepath) noexcept = 0;
    virtual void Stop(const std::size_t id) noexcept = 0;
    virtual void StopAll() noexcept = 0;

    virtual void Register3DAudioListener(Audio3DListener* newListener) noexcept = 0;
    virtual void Register3DAudioEmitter(Audio3DEmitter* newEmitter) noexcept = 0;
protected:
private:
};

class NullAudioService : public IAudioService {
    void SuspendAudio() noexcept override{};
    void ResumeAudio() noexcept override{};

    void Play([[maybe_unused]] const std::filesystem::path& filepath) noexcept override{};
    void Play([[maybe_unused]] const std::size_t id) noexcept override{};
    void Play([[maybe_unused]] const std::filesystem::path& filepath, [[maybe_unused]] const bool looping) noexcept override{};
    void Play([[maybe_unused]] const std::size_t id, [[maybe_unused]] const bool looping) noexcept override{};

    void Stop([[maybe_unused]] const std::filesystem::path& filepath) noexcept override{};
    void Stop([[maybe_unused]] const std::size_t id) noexcept override{};
    void StopAll() noexcept override{};

    void Register3DAudioListener([[maybe_unused]] Audio3DListener* newListener) noexcept {};
    void Register3DAudioEmitter([[maybe_unused]] Audio3DEmitter* newEmitter) noexcept {};

};
