#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Audio/Wav.hpp"
#include "Engine/Audio/Audio3DEmitter.hpp"
#include "Engine/Audio/Audio3DListener.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ThreadUtils.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <algorithm>

void AudioSystem::EmitterListenerDSP_worker() noexcept {
    while(IsRunning()) {
        static thread_local auto updateAudioThisFrame = uint8_t{0u};
        std::unique_lock<std::mutex> lock(m_cs);
        //Condition to wake up: not running or should update this frame.
        m_signal.wait(lock, [this]() -> bool { return !m_is_running || !!updateAudioThisFrame; });
        if(!!updateAudioThisFrame) {
            for(auto& active : m_active_channels) {
                for(auto& emitter : m_emitters) {
                    for(auto& listener : m_listeners) {
                        //const auto old_freq = active->GetFrequency();
                        auto dsp = CalculateDSP(*emitter, *listener, m_dsp_settings);
                        active->SetDSPSettings(dsp);
                    }
                }
            }
        }
        ++updateAudioThisFrame;
        updateAudioThisFrame &= 1;
    }
}

bool AudioSystem::IsRunning() const noexcept {
    bool running = false;
    {
        std::scoped_lock<std::mutex> lock(m_cs);
        running = m_is_running;
    }
    return running;
}

AudioSystem::AudioSystem() noexcept
: EngineSubsystem()
, IAudioService()
{
    InitializeAudioSystem();
}

AudioSystem::AudioSystem(std::size_t max_channels) noexcept
: EngineSubsystem()
, IAudioService()
, m_max_channels(max_channels)
{
    InitializeAudioSystem();
}

AudioSystem::~AudioSystem() noexcept {
    m_is_running = false;
    m_signal.notify_one();

    m_dsp_thread.join();

    for(auto& channel : m_active_channels) {
        channel->Stop();
    }

    {
        bool done_cleanup = false;
        do {
            std::this_thread::yield();
            std::scoped_lock<std::mutex> lock(m_cs);
            done_cleanup = m_active_channels.empty();
        } while(!done_cleanup);
    }

    m_active_channels.clear();
    m_active_channels.shrink_to_fit();

    m_idle_channels.clear();
    m_idle_channels.shrink_to_fit();

    m_sounds.clear();
    m_wave_files.clear();

    if(m_master_voice) {
        m_master_voice->DestroyVoice();
        m_master_voice = nullptr;
    }

    if(m_xaudio2) {
        m_xaudio2->UnregisterForCallbacks(&m_engine_callback);

        m_xaudio2->Release();
        m_xaudio2 = nullptr;
    }

    ::CoUninitialize();
}

void AudioSystem::InitializeAudioSystem() noexcept {
    bool co_init_succeeded = SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    GUARANTEE_OR_DIE(co_init_succeeded, "Failed to setup Audio System.");
    bool xaudio2_create_succeeded = SUCCEEDED(::XAudio2Create(&m_xaudio2));
    GUARANTEE_OR_DIE(xaudio2_create_succeeded, "Failed to create Audio System.");
}

void AudioSystem::Initialize() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
#ifdef AUDIO_DEBUG
    XAUDIO2_DEBUG_CONFIGURATION config{};
    config.LogFileline = true;
    config.LogFunctionName = true;
    config.LogThreadID = true;
    config.LogTiming = true;
    config.BreakMask = XAUDIO2_LOG_WARNINGS;
    config.TraceMask = XAUDIO2_LOG_DETAIL | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_FUNC_CALLS;
    m_xaudio2->SetDebugConfiguration(&config);
#endif
    m_xaudio2->CreateMasteringVoice(&m_master_voice);

    XAUDIO2_VOICE_DETAILS details{};
    m_master_voice->GetVoiceDetails(&details);
    m_input_channels = details.InputChannels;

    DebuggerPrintf(std::format("Mastering voice expects {} input channels.\n", details.InputChannels));

    DWORD dwChannelMask;
    m_master_voice->GetChannelMask(&dwChannelMask);

    ::X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, m_x3daudio);

    m_idle_channels.reserve(m_max_channels);
    m_active_channels.reserve(m_max_channels);

    FileUtils::detail::WavFormatChunk fmt{};
    fmt.formatId = 1;
    fmt.channelCount = 1;
    fmt.samplesPerSecond = 44100;
    fmt.bytesPerSecond = 88200;
    fmt.dataBlockSize = 2;
    fmt.bitsPerSample = 16;
    SetFormat(fmt);

    SetEngineCallback(&m_engine_callback);
    m_is_running = true;

    m_dsp_thread = std::thread(&AudioSystem::EmitterListenerDSP_worker, this);
    ThreadUtils::SetThreadDescription(m_dsp_thread, std::string{"AudioSystem Updater"});

    for(std::size_t i = 0; i < m_max_channels; ++i) {
        m_idle_channels.push_back(std::make_unique<Channel>(*this, AudioSystem::Channel::ChannelDesc{this}));
    }
}

const std::atomic_uint32_t& AudioSystem::GetOperationSetId() const noexcept {
    return m_operationID;
}
const std::atomic_uint32_t& AudioSystem::IncrementAndGetOperationSetId() noexcept {
    IncrementOperationSetId();
    return GetOperationSetId();
}

void AudioSystem::IncrementOperationSetId() noexcept {
    m_operationID++;
}

void AudioSystem::SubmitDeferredOperation(uint32_t operationSetId) noexcept {
    m_xaudio2->CommitChanges(operationSetId);
}

void AudioSystem::SetEngineCallback(EngineCallback* callback) noexcept {
    if(&m_engine_callback == callback) {
        return;
    }
    m_xaudio2->UnregisterForCallbacks(&m_engine_callback);
    m_engine_callback = *callback;
    m_xaudio2->RegisterForCallbacks(&m_engine_callback);
}

const WAVEFORMATEXTENSIBLE& AudioSystem::GetFormat() const noexcept {
    return m_audio_format_ex;
}

FileUtils::detail::WavFormatChunk AudioSystem::GetLoadedWavFileFormat() const noexcept {
    FileUtils::detail::WavFormatChunk fmt{};
    if(m_wave_files.empty()) {
        return fmt;
    }
    return m_wave_files.begin()->second->GetFormatChunk();
}

AudioDSPResults AudioSystem::CalculateDSP(const Audio3DEmitter& emitter, const Audio3DListener& listener, const AudioDSPSettings& settings) const noexcept {
    X3DAUDIO_EMITTER x3daudio_emitter{};
    X3DAUDIO_CONE emitter_cone{};
    if(const auto is_emitter_omniDirectional = emitter.IsOmniDirectional(); is_emitter_omniDirectional) {
        emitter_cone = GetDefaultOmniDirectionalX3DAudioCone();
    } else {
        emitter_cone = Audio3DConeToX3DAudioCone(emitter.Get3DCone());
    }
    x3daudio_emitter.pCone = &emitter_cone;

    const auto& x3daudio_listener = listener.GetX3DAudioListener();

    uint32_t flags = AudioDSPSettingsToX3DAudioDSPFlags(settings);

    X3DAUDIO_DSP_SETTINGS dsp_settings{};
    dsp_settings.pMatrixCoefficients = settings.use_matrix_table ? new float[m_input_channels * m_max_channels] : nullptr;
    dsp_settings.pDelayTimes = settings.use_delay_array && m_input_channels > 1 ? new float[m_max_channels] : nullptr;
    dsp_settings.SrcChannelCount = 1;
    dsp_settings.DstChannelCount = m_input_channels * static_cast<uint32_t>(m_max_channels);

    ::X3DAudioCalculate(m_x3daudio, &x3daudio_listener, &x3daudio_emitter, flags, &dsp_settings);
    
    AudioDSPResults results{};
    results.dopplerFactor = dsp_settings.DopplerFactor;
    results.emitterToListenerAngleRadians = dsp_settings.EmitterToListenerAngle;
    results.emitterToListenerDistance = dsp_settings.EmitterToListenerDistance;
    results.emitterVelocityComponent = dsp_settings.EmitterVelocityComponent;
    results.lowPassFilterDirectCoefficient = dsp_settings.LPFDirectCoefficient;
    results.lowPassFilterReverbCoefficient = dsp_settings.LPFReverbCoefficient;
    results.pMatrixCoefficients = &dsp_settings.pMatrixCoefficients;

    return results;
}

void AudioSystem::BeginFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void AudioSystem::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void AudioSystem::Render() const noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

void AudioSystem::EndFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
}

bool AudioSystem::ProcessSystemMessage(const EngineMessage& /*msg*/) noexcept {
    return false;
}

void AudioSystem::SuspendAudio() noexcept {
    if(m_xaudio2) {
        m_xaudio2->StopEngine();
    }
}

void AudioSystem::ResumeAudio() noexcept {
    if(m_xaudio2) {
        m_xaudio2->StartEngine();
    }
}

void AudioSystem::SetFormat(const WAVEFORMATEXTENSIBLE& format) noexcept {
    m_audio_format_ex = format;
}

void AudioSystem::SetFormat(const FileUtils::detail::WavFormatChunk& format) noexcept {
    auto* fmt_buffer = reinterpret_cast<const unsigned char*>(&format);
    std::memcpy(&m_audio_format_ex, fmt_buffer, sizeof(m_audio_format_ex));
}

void AudioSystem::RegisterWavFilesFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
        logger->LogErrorLine("Attempting to Register Wav Files from unknown path: " + FS::absolute(folderpath).string());
        return;
    }
    {
        std::error_code ec{};
        folderpath = FS::canonical(folderpath, ec);
        if(ec || !FileUtils::IsSafeReadPath(folderpath)) {
            auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
            logger->LogErrorLine("Attempting to Register Wav Files from inaccessible path: " + FS::absolute(folderpath).string());
            return;
        }
    }
    folderpath.make_preferred();
    if(!FS::is_directory(folderpath)) {
        return;
    }
    const auto cb =
    [this](const std::filesystem::path& p) {
        RegisterWavFile(p);
    };
    FileUtils::ForEachFileInFolder(folderpath, ".wav", cb, recursive);
}

void AudioSystem::DeactivateChannel(Channel& channel) noexcept {
    std::scoped_lock<std::mutex> lock(m_cs);
    const auto found_iter = std::find_if(std::begin(m_active_channels), std::end(m_active_channels),
                                         [&channel](const std::unique_ptr<Channel>& c) { return c.get() == &channel; });
    m_idle_channels.push_back(std::move(*found_iter));
    m_active_channels.erase(found_iter);
}

void AudioSystem::Play(Sound& snd, SoundDesc desc /* = SoundDesc{}*/) noexcept {
    std::scoped_lock<std::mutex> lock(m_cs);
    if(m_idle_channels.empty()) {
        return;
    }
    m_active_channels.push_back(std::move(m_idle_channels.back()));
    m_idle_channels.pop_back();
    auto& inserted_channel = m_active_channels.back();
    inserted_channel->Play(snd);
}

void AudioSystem::Play(std::filesystem::path filepath, SoundDesc desc /*= SoundDesc{}*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        return;
    }
    Sound* snd = CreateSound(filepath);
    Play(*snd, desc);
}

void AudioSystem::Play(const std::filesystem::path& filepath) noexcept {
    Play(filepath, SoundDesc{});
}

void AudioSystem::Play(const std::size_t id) noexcept {
    Play(m_sounds[id].first, SoundDesc{});
}

void AudioSystem::Play(const std::filesystem::path& filepath, const bool looping) noexcept {
    SoundDesc desc{};
    desc.loopCount = looping ? -1 : 0;
    Play(filepath, desc);
}

void AudioSystem::Play(const std::size_t id, const bool looping) noexcept {
    SoundDesc desc{};
    desc.loopCount = looping ? -1 : 0;
    Play(m_sounds[id].first, desc);
}

void AudioSystem::SetDSPSettings(const AudioDSPSettings& newSettings) noexcept {
    m_dsp_settings = newSettings;
}

void AudioSystem::Stop(const std::filesystem::path& filepath) noexcept {
    const auto& found = std::find_if(std::cbegin(m_sounds), std::cend(m_sounds), [&filepath](const auto& snd) { return snd.first == filepath; });
    if(found != std::cend(m_sounds)) {
        for(auto& channel : found->second->GetChannels()) {
            channel->Stop();
            DeactivateChannel(*channel);
        }
    }
}

void AudioSystem::Stop(const std::size_t id) noexcept {
    auto& channel = m_active_channels[id];
    channel->Stop();
    DeactivateChannel(*channel);
}

void AudioSystem::StopAll() noexcept {
    const auto& op_id = IncrementAndGetOperationSetId();
    for(auto& active_sound : m_active_channels) {
        active_sound->Stop(op_id);
        DeactivateChannel(*active_sound);
    }
    SubmitDeferredOperation(op_id);
}

AudioSystem::Sound* AudioSystem::CreateSound(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
        logger->LogErrorLine("Could not find file: " + filepath.string());
        return nullptr;
    }

    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
            logger->LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return nullptr;
        }
    }
    filepath.make_preferred();
    const auto finder = [&filepath](const auto& a) { return a.first == filepath; };
    auto found_iter = std::find_if(std::begin(m_sounds), std::end(m_sounds), finder);
    if(found_iter == m_sounds.end()) {
        m_sounds.emplace_back(std::make_pair(filepath, std::move(std::make_unique<Sound>(*this, filepath))));
        found_iter = std::find_if(std::begin(m_sounds), std::end(m_sounds), finder);
    }
    return found_iter->second.get();
}

AudioSystem::Sound* AudioSystem::CreateSoundInstance(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
        logger->LogErrorLine("Could not find file: " + filepath.string());
        return nullptr;
    }
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
            logger->LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return nullptr;
        }
    }
    filepath.make_preferred();
    m_sounds.emplace_back(std::make_pair(filepath, std::move(std::make_unique<Sound>(*this, filepath))));
    return m_sounds.back().second.get();
}

void AudioSystem::RegisterWavFile(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
        logger->LogErrorLine("Attempting to register wav file that does not exist: " + filepath.string());
        return;
    }
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
            logger->LogErrorLine("File: " + filepath.string() + " is inaccessible.");
        }
    }
    if(const auto found = std::find_if(std::cbegin(m_wave_files), std::cend(m_wave_files), [&filepath](const auto& wav) { return wav.first == filepath; }); found != std::cend(m_wave_files)) {
        return;
    }

    if(const auto wav_result = [&]() {
           auto&& wav = std::make_unique<FileUtils::Wav>();
           if(const auto result = wav->Load(filepath); result == FileUtils::Wav::WAV_SUCCESS) {
               m_wave_files.emplace_back(std::make_pair(filepath, std::move(wav)));
               return result;
           } else {
               return result;
           }
       }(); //IIIL
       wav_result != FileUtils::Wav::WAV_SUCCESS) {
        auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
        switch(wav_result) {
        case FileUtils::Wav::WAV_ERROR_NOT_A_WAV: {
            logger->LogErrorLine(filepath.string() + " is not a .wav file.");
            break;
        }
        case FileUtils::Wav::WAV_ERROR_BAD_FILE: {
            logger->LogErrorLine(filepath.string() + " is improperly formatted.");
            break;
        }
        default: {
            logger->LogErrorLine("Unknown error attempting to load " + filepath.string());
            break;
        }
        }
    }
}

void AudioSystem::Register3DAudioEmitter(Audio3DEmitter* emitter) noexcept {
    m_emitters.emplace_back(emitter);
}

void AudioSystem::Register3DAudioListener(Audio3DListener* listener) noexcept {
    m_listeners.emplace_back(listener);
}

void STDMETHODCALLTYPE AudioSystem::Channel::VoiceCallback::OnBufferEnd(void* pBufferContext) {
    Channel& channel = *static_cast<Channel*>(pBufferContext);
    channel.Stop();
    channel.m_sound->RemoveChannel(&channel);
    channel.m_sound = nullptr;
    channel.m_audio_system->DeactivateChannel(channel);
}

void STDMETHODCALLTYPE AudioSystem::Channel::VoiceCallback::OnLoopEnd(void* pBufferContext) {
    Channel& channel = *static_cast<Channel*>(pBufferContext);
    if(channel.m_desc.stopWhenFinishedLooping && channel.m_desc.loop_count != XAUDIO2_LOOP_INFINITE) {
        if(++channel.m_desc.repeat_count >= channel.m_desc.loop_count) {
            channel.Stop();
        }
    }
}

AudioSystem::Channel::Channel(AudioSystem& audioSystem, const ChannelDesc& desc) noexcept
: m_audio_system(&audioSystem)
, m_desc{desc} {
    static VoiceCallback vcb;
    m_buffer.pContext = this;
    auto* fmt = reinterpret_cast<const WAVEFORMATEX*>(&(m_audio_system->GetFormat()));
    m_audio_system->m_xaudio2->CreateSourceVoice(&m_voice, fmt, 0, m_desc.frequency_max, &vcb);
    //if(auto* group = _audio_system->GetChannelGroup(desc.groupName); group != nullptr) {
    //    group->AddChannel(this);
    //}
}

AudioSystem::Channel::~Channel() noexcept {
    if(m_voice) {
        Stop();
        m_voice->DestroyVoice();
        m_voice = nullptr;
    }
}

void AudioSystem::Channel::SetDSPSettings(AudioDSPResults& settings) {
    if(this && m_voice) {
        if(settings.pMatrixCoefficients && *settings.pMatrixCoefficients) {
            m_voice->SetOutputMatrix(m_audio_system->m_master_voice, 1, m_audio_system->m_input_channels, *settings.pMatrixCoefficients);
            delete[] *settings.pMatrixCoefficients;
            *settings.pMatrixCoefficients = nullptr;
        }
        XAUDIO2_FILTER_PARAMETERS filterParameters{XAUDIO2_FILTER_TYPE::LowPassFilter, 2.0f * std::sin(MathUtils::M_1PI_6 * settings.lowPassFilterDirectCoefficient)};
        m_voice->SetFilterParameters(&filterParameters);
    }
}

void AudioSystem::Channel::SetDSPSettings(AudioDSPResults& settings, uint32_t operationSetId) {
    if(m_voice) {
        if(settings.pMatrixCoefficients && *settings.pMatrixCoefficients) {
            m_voice->SetOutputMatrix(m_audio_system->m_master_voice, 1, m_audio_system->m_input_channels, *settings.pMatrixCoefficients, operationSetId);
            delete[] *settings.pMatrixCoefficients;
            *settings.pMatrixCoefficients = nullptr;
        }
        XAUDIO2_FILTER_PARAMETERS filterParameters{XAUDIO2_FILTER_TYPE::LowPassFilter, 2.0f * std::sin(MathUtils::M_1PI_6 * settings.lowPassFilterDirectCoefficient)};
        m_voice->SetFilterParameters(&filterParameters, operationSetId);
    }
}

void AudioSystem::Channel::Play(Sound& snd) noexcept {
    snd.AddChannel(this);
    m_sound = &snd;
    if(const auto* wav = snd.GetWav()) {
        m_buffer.pAudioData = wav->GetDataBuffer();
        m_buffer.AudioBytes = wav->GetDataBufferSize();
        m_buffer.LoopCount = m_desc.loop_count;
        m_buffer.LoopBegin = 0;
        m_buffer.LoopLength = 0;
        if(m_desc.loop_count) {
            m_buffer.LoopBegin = m_desc.loop_beginSamples;
            m_buffer.LoopLength = m_desc.loop_endSamples - m_desc.loop_beginSamples;
        }
        m_voice->SubmitSourceBuffer(&m_buffer, nullptr);
        m_voice->SetVolume(m_desc.volume);
        m_voice->SetFrequencyRatio(m_desc.frequency);
        m_voice->Start();
    }
}

void AudioSystem::Channel::Play(Sound& snd, uint32_t operationSetId) noexcept {
    snd.AddChannel(this);
    m_sound = &snd;
    if(const auto* wav = snd.GetWav()) {
        m_buffer.pAudioData = wav->GetDataBuffer();
        m_buffer.AudioBytes = wav->GetDataBufferSize();
        m_buffer.LoopCount = m_desc.loop_count;
        m_buffer.LoopBegin = 0;
        m_buffer.LoopLength = 0;
        if(m_desc.loop_count) {
            m_buffer.LoopBegin = m_desc.loop_beginSamples;
            m_buffer.LoopLength = m_desc.loop_endSamples - m_desc.loop_beginSamples;
        }
        m_voice->SubmitSourceBuffer(&m_buffer, nullptr);
        m_voice->SetVolume(m_desc.volume, operationSetId);
        m_voice->SetFrequencyRatio(m_desc.frequency, operationSetId);
        m_voice->Start(0, operationSetId);
    }
}

void AudioSystem::Channel::Stop() noexcept {
    if(m_voice) {
        m_voice->Stop();
        m_voice->FlushSourceBuffers();
    }
}

void AudioSystem::Channel::Stop(uint32_t operationSetId) noexcept {
    if(m_voice) {
        m_voice->Stop(0, operationSetId);
        m_voice->FlushSourceBuffers();
    }
}

void AudioSystem::Channel::Pause() noexcept {
    if(m_voice) {
        m_voice->Stop();
    }
}

void AudioSystem::Channel::Pause(uint32_t operationSetId) noexcept {
    if(m_voice) {
        m_voice->Stop(0, operationSetId);
    }
}

AudioSystem::Channel::ChannelDesc::ChannelDesc(AudioSystem* audioSystem)
: audio_system{audioSystem} {
    /* DO NOTHING */
}

AudioSystem::Channel::ChannelDesc& AudioSystem::Channel::ChannelDesc::operator=(const SoundDesc& sndDesc) {
    volume = sndDesc.volume;
    frequency = sndDesc.frequency;
    loop_count = sndDesc.loopCount <= -1 ? XAUDIO2_LOOP_INFINITE : (std::clamp(sndDesc.loopCount, 0, XAUDIO2_MAX_LOOP_COUNT));
    stopWhenFinishedLooping = sndDesc.stopWhenFinishedLooping;
    const auto& fmt = audio_system->GetLoadedWavFileFormat();
    loop_beginSamples = static_cast<uint32_t>(fmt.samplesPerSecond * sndDesc.loopBegin.count());
    loop_endSamples = static_cast<uint32_t>(fmt.samplesPerSecond * sndDesc.loopEnd.count());
    groupName = sndDesc.groupName;
    return *this;
}

void AudioSystem::Channel::SetStopWhenFinishedLooping(bool value) {
    m_desc.stopWhenFinishedLooping = value;
}

void AudioSystem::Channel::SetLoopCount(int count) noexcept {
    if(count <= -1) {
        m_desc.loop_count = XAUDIO2_LOOP_INFINITE;
    } else {
        count = std::clamp(count, 0, XAUDIO2_MAX_LOOP_COUNT);
        m_desc.loop_count = count;
    }
}

uint32_t AudioSystem::Channel::GetLoopCount() const noexcept {
    return m_desc.loop_count;
}

void AudioSystem::Channel::SetLoopRange(TimeUtils::FPSeconds start, TimeUtils::FPSeconds end) {
    SetLoopBegin(start);
    SetLoopEnd(end);
}

void AudioSystem::Channel::SetLoopBegin(TimeUtils::FPSeconds start) {
    const auto& fmt = m_audio_system->GetLoadedWavFileFormat();
    m_desc.loop_beginSamples = static_cast<uint32_t>(fmt.samplesPerSecond * start.count());
}

void AudioSystem::Channel::SetLoopEnd(TimeUtils::FPSeconds end) {
    const auto& fmt = m_audio_system->GetLoadedWavFileFormat();
    m_desc.loop_endSamples = static_cast<uint32_t>(fmt.samplesPerSecond * end.count());
}

void AudioSystem::Channel::SetVolume(float newVolume) noexcept {
    m_desc.volume = newVolume;
}

void AudioSystem::Channel::SetFrequency(float newFrequency) noexcept {
    newFrequency = std::clamp(newFrequency, XAUDIO2_MIN_FREQ_RATIO, m_desc.frequency_max);
    m_desc.frequency = newFrequency;
}

float AudioSystem::Channel::GetVolume() const noexcept {
    return m_desc.volume;
}

float AudioSystem::Channel::GetFrequency() const noexcept {
    return m_desc.frequency;
}

AudioSystem::Sound::Sound(AudioSystem& audiosystem, std::filesystem::path filepath)
: m_audio_system(&audiosystem) {
    namespace FS = std::filesystem;
    GUARANTEE_OR_DIE(FS::exists(filepath), "Attempting to create sound that does not exist.\n");
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto* logger = ServiceLocator::get<IFileLoggerService, NullFileLoggerService>();
            logger->LogErrorLine("File: " + filepath.string() + " is inaccessible.");
        }
    }
    filepath.make_preferred();
    const auto pred = [&filepath](const auto& wav) { return wav.first == filepath; };
    auto found = std::find_if(std::begin(m_audio_system->m_wave_files), std::end(m_audio_system->m_wave_files), pred);
    if(found == m_audio_system->m_wave_files.end()) {
        m_audio_system->RegisterWavFile(filepath);
        found = std::find_if(std::begin(m_audio_system->m_wave_files), std::end(m_audio_system->m_wave_files), pred);
    }
    if(found != std::end(m_audio_system->m_wave_files)) {
        m_my_id = m_id++;
        m_wave_file = found->second.get();
    }
}

void AudioSystem::Sound::AddChannel(Channel* channel) noexcept {
    std::scoped_lock<std::mutex> lock(m_cs);
    m_channels.push_back(channel);
}

void AudioSystem::Sound::RemoveChannel(Channel* channel) noexcept {
    std::scoped_lock<std::mutex> lock(m_cs);
    m_channels.erase(std::remove_if(std::begin(m_channels), std::end(m_channels),
                                   [channel](Channel* c) -> bool { return c == channel; }),
                    std::end(m_channels));
}

const std::size_t AudioSystem::Sound::GetId() const noexcept {
    return m_my_id;
}

const std::size_t AudioSystem::Sound::GetCount() noexcept {
    return m_id;
}

const FileUtils::Wav* const AudioSystem::Sound::GetWav() const noexcept {
    return m_wave_file;
}

const std::vector<AudioSystem::Channel*>& AudioSystem::Sound::GetChannels() const noexcept {
    return m_channels;
}

void STDMETHODCALLTYPE AudioSystem::EngineCallback::OnCriticalError(HRESULT error) {
    std::ostringstream ss;
    ss << "The Audio System encountered a fatal error: ";
    ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << error;
    ERROR_AND_DIE(ss.str().c_str());
}
