#pragma once

/************************************************/
/* Audio System built based on HUGS system      */
/* by Youtube User ChiliTomatoNoodle            */
/* https://www.youtube.com/watch?v=T51Eqbbald4  */
/************************************************/

#include "Engine/Audio/Wav.hpp"
#include "Engine/Audio/XAudio.hpp"

#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Services/IAudioService.hpp"

#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace FileUtils {
class Wav;
}

class Audio3DEmitter;
class Audio3DListener;

class AudioSystem : public EngineSubsystem, public IAudioService {
private:
    class Channel;
    class ChannelGroup;

public:
    class EngineCallback : public IXAudio2EngineCallback {
    public:
        virtual ~EngineCallback() {
        }
        virtual void STDMETHODCALLTYPE OnProcessingPassStart() override {};
        virtual void STDMETHODCALLTYPE OnProcessingPassEnd() override {};
        virtual void STDMETHODCALLTYPE OnCriticalError(HRESULT error) override;
    };
    class Sound {
    public:
        Sound(AudioSystem& audiosystem, std::filesystem::path filepath);
        void AddChannel(Channel* channel) noexcept;
        void RemoveChannel(Channel* channel) noexcept;
        [[nodiscard]] const std::size_t GetId() const noexcept;
        [[nodiscard]] static const std::size_t GetCount() noexcept;
        [[nodiscard]] const FileUtils::Wav* const GetWav() const noexcept;
        const std::vector<Channel*>& GetChannels() const noexcept;
    private:
        inline static std::size_t _id{0u};
        AudioSystem* _audio_system{};
        std::size_t _my_id{0u};
        FileUtils::Wav* _wave_file{};
        std::vector<Channel*> _channels{};
        mutable std::mutex _cs{};
    };
    struct SoundDesc {
        float volume{1.0f};
        float frequency{1.0f};
        int loopCount{0};
        bool stopWhenFinishedLooping{false};
        TimeUtils::FPSeconds loopBegin{};
        TimeUtils::FPSeconds loopEnd{};
        std::string groupName{};
    };

    void Play(const std::filesystem::path& filepath) noexcept override;
    void Play(const std::size_t id) noexcept override;
    void Play(const std::filesystem::path& filepath, const bool looping) noexcept override;
    void Play(const std::size_t id, const bool looping) noexcept override;
    void Stop(const std::filesystem::path& filepath) noexcept override;
    void Stop(const std::size_t id) noexcept override;
    void StopAll() noexcept override;

private:
    class Channel {
    public:
        class VoiceCallback : public IXAudio2VoiceCallback {
        public:
            virtual ~VoiceCallback() {
            }
            virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(uint32_t /*bytesRequired*/) override {};
            virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {};
            virtual void STDMETHODCALLTYPE OnStreamEnd() override {};
            virtual void STDMETHODCALLTYPE OnBufferStart(void* /*pBufferContext*/) override {};
            virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
            virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
            virtual void STDMETHODCALLTYPE OnVoiceError(void* /*pBufferContext*/, HRESULT /*Error*/) override {};
        };
        struct ChannelDesc {
            ChannelDesc() = default;
            ChannelDesc(const ChannelDesc& other) = default;
            ChannelDesc& operator=(const ChannelDesc& other) = default;
            ChannelDesc& operator=(const SoundDesc& sndDesc);
            explicit ChannelDesc(AudioSystem* audioSystem);

            AudioSystem* audio_system{nullptr};
            unsigned long long repeat_count{0ull};
            float volume{1.0f};
            float frequency{1.0f};
            float frequency_max{2.0f};
            uint32_t loop_count{0};
            uint32_t loop_beginSamples{0};
            uint32_t loop_endSamples{0};
            bool stopWhenFinishedLooping{false};
            std::string groupName{};
        };
        explicit Channel(AudioSystem& audioSystem, const ChannelDesc& desc) noexcept;
        ~Channel() noexcept;

        void SetDSPSettings(AudioDSPResults& settings);

        void Play(Sound& snd) noexcept;
        void Stop() noexcept;
        void Pause() noexcept;
        void SetStopWhenFinishedLooping(bool value);

        void SetDSPSettings(AudioDSPResults& settings, uint32_t operationSetId);
        void Play(Sound& snd, uint32_t operationSetId) noexcept;
        void Stop(uint32_t operationSetId) noexcept;
        void Pause(uint32_t operationSetId) noexcept;

        void SetLoopCount(int count) noexcept;
        [[nodiscard]] uint32_t GetLoopCount() const noexcept;

        void SetLoopBegin(TimeUtils::FPSeconds start);
        void SetLoopEnd(TimeUtils::FPSeconds end);
        void SetLoopRange(TimeUtils::FPSeconds start, TimeUtils::FPSeconds end);

        [[nodiscard]] float GetVolume() const noexcept;
        void SetVolume(float newVolume) noexcept;

        [[nodiscard]] float GetFrequency() const noexcept;
        void SetFrequency(float newFrequency) noexcept;

    private:
        XAUDIO2_BUFFER _buffer{};
        IXAudio2SourceVoice* _voice = nullptr;
        Sound* _sound = nullptr;
        AudioSystem* _audio_system = nullptr;
        ChannelDesc _desc{};

        friend class VoiceCallback;
        friend class ChannelGroup;
    };

public:
    AudioSystem() noexcept;
    explicit AudioSystem(std::size_t max_channels) noexcept;
    AudioSystem(const AudioSystem& other) = delete;
    AudioSystem(AudioSystem&& other) = delete;
    AudioSystem& operator=(const AudioSystem& rhs) = delete;
    AudioSystem& operator=(AudioSystem&& rhs) = delete;
    virtual ~AudioSystem() noexcept;
    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;
    [[nodiscard]] virtual bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;

    void SuspendAudio() noexcept;
    void ResumeAudio() noexcept;

    void SetFormat(const WAVEFORMATEXTENSIBLE& format) noexcept;
    void SetFormat(const FileUtils::Wav::WavFormatChunk& format) noexcept;

    void RegisterWavFilesFromFolder(std::filesystem::path folderpath, bool recursive = false) noexcept;
    void RegisterWavFile(std::filesystem::path filepath) noexcept;

    void Register3DAudioEmitter(Audio3DEmitter* emitter) noexcept override;
    void Register3DAudioListener(Audio3DListener* listener) noexcept override;

    void Play(Sound& snd, SoundDesc desc = SoundDesc{}) noexcept;
    void Play(std::filesystem::path filepath, SoundDesc desc = SoundDesc{}) noexcept;

    void SetDSPSettings(const AudioDSPSettings& newSettings) noexcept;

    [[nodiscard]] Sound* CreateSound(std::filesystem::path filepath) noexcept;
    [[nodiscard]] Sound* CreateSoundInstance(std::filesystem::path filepath) noexcept;

    [[nodiscard]] ChannelGroup* GetChannelGroup(const std::string& name) const noexcept;

    void SubmitDeferredOperation(uint32_t operationSetId) noexcept;
    const std::atomic_uint32_t& GetOperationSetId() const noexcept;
    const std::atomic_uint32_t& IncrementAndGetOperationSetId() noexcept;
    void IncrementOperationSetId() noexcept;

    void SetEngineCallback(EngineCallback* callback) noexcept;
    [[nodiscard]] const WAVEFORMATEXTENSIBLE& GetFormat() const noexcept;
    [[nodiscard]] FileUtils::Wav::WavFormatChunk GetLoadedWavFileFormat() const noexcept;

    AudioDSPResults CalculateDSP(const Audio3DEmitter& emitter, const Audio3DListener& listener, const AudioDSPSettings& settings) const noexcept;

protected:
private:
    void InitializeAudioSystem() noexcept;

    void DeactivateChannel(Channel& channel) noexcept;

    void EmitterListenerDSP_worker() noexcept;

    bool IsRunning() const noexcept;

    WAVEFORMATEXTENSIBLE _audio_format_ex{};
    std::size_t _sound_count{};
    std::size_t _max_channels{64u};
    std::vector<std::pair<std::filesystem::path, std::unique_ptr<FileUtils::Wav>>> _wave_files{};
    std::vector<std::pair<std::filesystem::path, std::unique_ptr<Sound>>> _sounds{};
    std::vector<std::unique_ptr<Channel>> _active_channels{};
    std::vector<std::unique_ptr<Channel>> _idle_channels{};
    std::vector<Audio3DEmitter*> _emitters{};
    std::vector<Audio3DListener*> _listeners{};
    std::atomic_uint32_t _operationID{};
    IXAudio2* _xaudio2 = nullptr;
    X3DAUDIO_HANDLE _x3daudio;
    IXAudio2MasteringVoice* _master_voice{};
    EngineCallback _engine_callback{};
    uint32_t _input_channels{};
    mutable std::mutex _cs{};
    std::thread _dsp_thread{};
    AudioDSPSettings _dsp_settings{};
    std::atomic_bool _is_running{false};
    std::condition_variable _signal{};
};
