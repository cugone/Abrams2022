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

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <algorithm>

void AudioSystem::EmitterListenerDSP_worker() noexcept {
    while(IsRunning()) {
        static thread_local auto updateAudioThisFrame = uint8_t{0u};
        std::unique_lock<std::mutex> lock(_cs);
        //Condition to wake up: not running or should update this frame.
        _signal.wait(lock, [this]() -> bool { return !_is_running || !!updateAudioThisFrame; });
        if(!!updateAudioThisFrame) {
            for(auto& active : _active_channels) {
                for(auto& emitter : _emitters) {
                    for(auto& listener : _listeners) {
                        //const auto old_freq = active->GetFrequency();
                        auto dsp = CalculateDSP(*emitter, *listener, _dsp_settings);
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
        std::scoped_lock<std::mutex> lock(_cs);
        running = _is_running;
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
, _max_channels(max_channels)
{
    InitializeAudioSystem();
}

AudioSystem::~AudioSystem() noexcept {
    _is_running = false;
    _signal.notify_one();

    _dsp_thread.join();

    for(auto& channel : _active_channels) {
        channel->Stop();
    }

    {
        bool done_cleanup = false;
        do {
            std::this_thread::yield();
            std::scoped_lock<std::mutex> lock(_cs);
            done_cleanup = _active_channels.empty();
        } while(!done_cleanup);
    }

    _active_channels.clear();
    _active_channels.shrink_to_fit();

    _idle_channels.clear();
    _idle_channels.shrink_to_fit();

    _sounds.clear();
    _wave_files.clear();

    if(_master_voice) {
        _master_voice->DestroyVoice();
        _master_voice = nullptr;
    }

    if(_xaudio2) {
        _xaudio2->UnregisterForCallbacks(&_engine_callback);

        _xaudio2->Release();
        _xaudio2 = nullptr;
    }

    ::CoUninitialize();
}

void AudioSystem::InitializeAudioSystem() noexcept {
    bool co_init_succeeded = SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    GUARANTEE_OR_DIE(co_init_succeeded, "Failed to setup Audio System.");
    bool xaudio2_create_succeeded = SUCCEEDED(::XAudio2Create(&_xaudio2));
    GUARANTEE_OR_DIE(xaudio2_create_succeeded, "Failed to create Audio System.");
}

void AudioSystem::Initialize() noexcept {
#ifdef AUDIO_DEBUG
    XAUDIO2_DEBUG_CONFIGURATION config{};
    config.LogFileline = true;
    config.LogFunctionName = true;
    config.LogThreadID = true;
    config.LogTiming = true;
    config.BreakMask = XAUDIO2_LOG_WARNINGS;
    config.TraceMask = XAUDIO2_LOG_DETAIL | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_FUNC_CALLS;
    _xaudio2->SetDebugConfiguration(&config);
#endif
    _xaudio2->CreateMasteringVoice(&_master_voice);

    XAUDIO2_VOICE_DETAILS details{};
    _master_voice->GetVoiceDetails(&details);
    _input_channels = details.InputChannels;

    DebuggerPrintf("Mastering voice expects %i input channels.\n", details.InputChannels);

    DWORD dwChannelMask;
    _master_voice->GetChannelMask(&dwChannelMask);

    ::X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, _x3daudio);

    _idle_channels.reserve(_max_channels);
    _active_channels.reserve(_max_channels);

    FileUtils::Wav::WavFormatChunk fmt{};
    fmt.formatId = 1;
    fmt.channelCount = 1;
    fmt.samplesPerSecond = 44100;
    fmt.bytesPerSecond = 88200;
    fmt.dataBlockSize = 2;
    fmt.bitsPerSample = 16;
    SetFormat(fmt);

    SetEngineCallback(&_engine_callback);
    _is_running = true;

    _dsp_thread = std::thread(&AudioSystem::EmitterListenerDSP_worker, this);
    ThreadUtils::SetThreadDescription(_dsp_thread, std::string{"AudioSystem Updater"});

    for(std::size_t i = 0; i < _max_channels; ++i) {
        _idle_channels.push_back(std::make_unique<Channel>(*this, AudioSystem::Channel::ChannelDesc{this}));
    }
}

const std::atomic_uint32_t& AudioSystem::GetOperationSetId() const noexcept {
    return _operationID;
}
const std::atomic_uint32_t& AudioSystem::IncrementAndGetOperationSetId() noexcept {
    IncrementOperationSetId();
    return GetOperationSetId();
}

void AudioSystem::IncrementOperationSetId() noexcept {
    _operationID++;
}

void AudioSystem::SubmitDeferredOperation(uint32_t operationSetId) noexcept {
    _xaudio2->CommitChanges(operationSetId);
}

void AudioSystem::SetEngineCallback(EngineCallback* callback) noexcept {
    if(&_engine_callback == callback) {
        return;
    }
    _xaudio2->UnregisterForCallbacks(&_engine_callback);
    _engine_callback = *callback;
    _xaudio2->RegisterForCallbacks(&_engine_callback);
}

const WAVEFORMATEXTENSIBLE& AudioSystem::GetFormat() const noexcept {
    return _audio_format_ex;
}

FileUtils::Wav::WavFormatChunk AudioSystem::GetLoadedWavFileFormat() const noexcept {
    FileUtils::Wav::WavFormatChunk fmt{};
    if(_wave_files.empty()) {
        return fmt;
    }
    return _wave_files.begin()->second->GetFormatChunk();
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
    dsp_settings.pMatrixCoefficients = settings.use_matrix_table ? new float[_input_channels * _max_channels] : nullptr;
    dsp_settings.pDelayTimes = settings.use_delay_array && _input_channels > 1 ? new float[_max_channels] : nullptr;
    dsp_settings.SrcChannelCount = 1;
    dsp_settings.DstChannelCount = _input_channels * static_cast<uint32_t>(_max_channels);

    ::X3DAudioCalculate(_x3daudio, &x3daudio_listener, &x3daudio_emitter, flags, &dsp_settings);
    
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
    /* DO NOTHING */
}

void AudioSystem::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {

}

void AudioSystem::Render() const noexcept {
    /* DO NOTHING */
}

void AudioSystem::EndFrame() noexcept {
    /* DO NOTHING */
}

bool AudioSystem::ProcessSystemMessage(const EngineMessage& /*msg*/) noexcept {
    return false;
}

void AudioSystem::SuspendAudio() noexcept {
    if(_xaudio2) {
        _xaudio2->StopEngine();
    }
}

void AudioSystem::ResumeAudio() noexcept {
    if(_xaudio2) {
        _xaudio2->StartEngine();
    }
}

void AudioSystem::SetFormat(const WAVEFORMATEXTENSIBLE& format) noexcept {
    _audio_format_ex = format;
}

void AudioSystem::SetFormat(const FileUtils::Wav::WavFormatChunk& format) noexcept {
    auto* fmt_buffer = reinterpret_cast<const unsigned char*>(&format);
    std::memcpy(&_audio_format_ex, fmt_buffer, sizeof(_audio_format_ex));
}

void AudioSystem::RegisterWavFilesFromFolder(std::filesystem::path folderpath, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(folderpath)) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        logger.LogErrorLine("Attempting to Register Wav Files from unknown path: " + FS::absolute(folderpath).string());
        return;
    }
    {
        std::error_code ec{};
        folderpath = FS::canonical(folderpath, ec);
        if(ec || !FileUtils::IsSafeReadPath(folderpath)) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("Attempting to Register Wav Files from inaccessible path: " + FS::absolute(folderpath).string());
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
    std::scoped_lock<std::mutex> lock(_cs);
    const auto found_iter = std::find_if(std::begin(_active_channels), std::end(_active_channels),
                                         [&channel](const std::unique_ptr<Channel>& c) { return c.get() == &channel; });
    _idle_channels.push_back(std::move(*found_iter));
    _active_channels.erase(found_iter);
}

void AudioSystem::Play(Sound& snd, SoundDesc desc /* = SoundDesc{}*/) noexcept {
    std::scoped_lock<std::mutex> lock(_cs);
    if(_idle_channels.empty()) {
        return;
    }
    _active_channels.push_back(std::move(_idle_channels.back()));
    _idle_channels.pop_back();
    auto& inserted_channel = _active_channels.back();
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
    Play(_sounds[id].first, SoundDesc{});
}

void AudioSystem::Play(const std::filesystem::path& filepath, const bool looping) noexcept {
    SoundDesc desc{};
    desc.loopCount = looping ? -1 : 0;
    Play(filepath, desc);
}

void AudioSystem::Play(const std::size_t id, const bool looping) noexcept {
    SoundDesc desc{};
    desc.loopCount = looping ? -1 : 0;
    Play(_sounds[id].first, desc);
}

void AudioSystem::SetDSPSettings(const AudioDSPSettings& newSettings) noexcept {
    _dsp_settings = newSettings;
}

void AudioSystem::Stop(const std::filesystem::path& filepath) noexcept {
    const auto& found = std::find_if(std::cbegin(_sounds), std::cend(_sounds), [&filepath](const auto& snd) { return snd.first == filepath; });
    if(found != std::cend(_sounds)) {
        for(auto& channel : found->second->GetChannels()) {
            channel->Stop();
            DeactivateChannel(*channel);
        }
    }
}

void AudioSystem::Stop(const std::size_t id) noexcept {
    auto& channel = _active_channels[id];
    channel->Stop();
    DeactivateChannel(*channel);
}

void AudioSystem::StopAll() noexcept {
    const auto& op_id = IncrementAndGetOperationSetId();
    for(auto& active_sound : _active_channels) {
        active_sound->Stop(op_id);
        DeactivateChannel(*active_sound);
    }
    SubmitDeferredOperation(op_id);
}

AudioSystem::Sound* AudioSystem::CreateSound(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        logger.LogErrorLine("Could not find file: " + filepath.string());
        return nullptr;
    }

    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return nullptr;
        }
    }
    filepath.make_preferred();
    const auto finder = [&filepath](const auto& a) { return a.first == filepath; };
    auto found_iter = std::find_if(std::begin(_sounds), std::end(_sounds), finder);
    if(found_iter == _sounds.end()) {
        _sounds.emplace_back(std::make_pair(filepath, std::move(std::make_unique<Sound>(*this, filepath))));
        found_iter = std::find_if(std::begin(_sounds), std::end(_sounds), finder);
    }
    return found_iter->second.get();
}

AudioSystem::Sound* AudioSystem::CreateSoundInstance(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        logger.LogErrorLine("Could not find file: " + filepath.string());
        return nullptr;
    }
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return nullptr;
        }
    }
    filepath.make_preferred();
    _sounds.emplace_back(std::make_pair(filepath, std::move(std::make_unique<Sound>(*this, filepath))));
    return _sounds.back().second.get();
}

void AudioSystem::RegisterWavFile(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(filepath)) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        logger.LogErrorLine("Attempting to register wav file that does not exist: " + filepath.string());
        return;
    }
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
        }
    }
    if(const auto found = std::find_if(std::cbegin(_wave_files), std::cend(_wave_files), [&filepath](const auto& wav) { return wav.first == filepath; }); found != std::cend(_wave_files)) {
        return;
    }

    if(const auto wav_result = [&]() {
           auto&& wav = std::make_unique<FileUtils::Wav>();
           if(const auto result = wav->Load(filepath); result == FileUtils::Wav::WAV_SUCCESS) {
               _wave_files.emplace_back(std::make_pair(filepath, std::move(wav)));
               return result;
           } else {
               return result;
           }
       }(); //IIIL
       wav_result != FileUtils::Wav::WAV_SUCCESS) {
        auto& logger = ServiceLocator::get<IFileLoggerService>();
        switch(wav_result) {
        case FileUtils::Wav::WAV_ERROR_NOT_A_WAV: {
            logger.LogErrorLine(filepath.string() + " is not a .wav file.");
            break;
        }
        case FileUtils::Wav::WAV_ERROR_BAD_FILE: {
            logger.LogErrorLine(filepath.string() + " is improperly formatted.");
            break;
        }
        default: {
            logger.LogErrorLine("Unknown error attempting to load " + filepath.string());
            break;
        }
        }
    }
}

void AudioSystem::Register3DAudioEmitter(Audio3DEmitter* emitter) noexcept {
    _emitters.emplace_back(emitter);
}

void AudioSystem::Register3DAudioListener(Audio3DListener* listener) noexcept {
    _listeners.emplace_back(listener);
}

void STDMETHODCALLTYPE AudioSystem::Channel::VoiceCallback::OnBufferEnd(void* pBufferContext) {
    Channel& channel = *static_cast<Channel*>(pBufferContext);
    channel.Stop();
    channel._sound->RemoveChannel(&channel);
    channel._sound = nullptr;
    channel._audio_system->DeactivateChannel(channel);
}

void STDMETHODCALLTYPE AudioSystem::Channel::VoiceCallback::OnLoopEnd(void* pBufferContext) {
    Channel& channel = *static_cast<Channel*>(pBufferContext);
    if(channel._desc.stopWhenFinishedLooping && channel._desc.loop_count != XAUDIO2_LOOP_INFINITE) {
        if(++channel._desc.repeat_count >= channel._desc.loop_count) {
            channel.Stop();
        }
    }
}

AudioSystem::Channel::Channel(AudioSystem& audioSystem, const ChannelDesc& desc) noexcept
: _audio_system(&audioSystem)
, _desc{desc} {
    static VoiceCallback vcb;
    _buffer.pContext = this;
    auto* fmt = reinterpret_cast<const WAVEFORMATEX*>(&(_audio_system->GetFormat()));
    _audio_system->_xaudio2->CreateSourceVoice(&_voice, fmt, 0, _desc.frequency_max, &vcb);
    //if(auto* group = _audio_system->GetChannelGroup(desc.groupName); group != nullptr) {
    //    group->AddChannel(this);
    //}
}

AudioSystem::Channel::~Channel() noexcept {
    if(_voice) {
        Stop();
        _voice->DestroyVoice();
        _voice = nullptr;
    }
}

void AudioSystem::Channel::SetDSPSettings(AudioDSPResults& settings) {
    if(this && _voice) {
        if(settings.pMatrixCoefficients && *settings.pMatrixCoefficients) {
            _voice->SetOutputMatrix(_audio_system->_master_voice, 1, _audio_system->_input_channels, *settings.pMatrixCoefficients);
            delete[] *settings.pMatrixCoefficients;
            *settings.pMatrixCoefficients = nullptr;
        }
        XAUDIO2_FILTER_PARAMETERS filterParameters{XAUDIO2_FILTER_TYPE::LowPassFilter, 2.0f * std::sin(MathUtils::M_1PI_6 * settings.lowPassFilterDirectCoefficient)};
        _voice->SetFilterParameters(&filterParameters);
    }
}

void AudioSystem::Channel::SetDSPSettings(AudioDSPResults& settings, uint32_t operationSetId) {
    if(_voice) {
        if(settings.pMatrixCoefficients && *settings.pMatrixCoefficients) {
            _voice->SetOutputMatrix(_audio_system->_master_voice, 1, _audio_system->_input_channels, *settings.pMatrixCoefficients, operationSetId);
            delete[] *settings.pMatrixCoefficients;
            *settings.pMatrixCoefficients = nullptr;
        }
        XAUDIO2_FILTER_PARAMETERS filterParameters{XAUDIO2_FILTER_TYPE::LowPassFilter, 2.0f * std::sin(MathUtils::M_1PI_6 * settings.lowPassFilterDirectCoefficient)};
        _voice->SetFilterParameters(&filterParameters, operationSetId);
    }
}

void AudioSystem::Channel::Play(Sound& snd) noexcept {
    snd.AddChannel(this);
    _sound = &snd;
    if(const auto* wav = snd.GetWav()) {
        _buffer.pAudioData = wav->GetDataBuffer();
        _buffer.AudioBytes = wav->GetDataBufferSize();
        _buffer.LoopCount = _desc.loop_count;
        _buffer.LoopBegin = 0;
        _buffer.LoopLength = 0;
        if(_desc.loop_count) {
            _buffer.LoopBegin = _desc.loop_beginSamples;
            _buffer.LoopLength = _desc.loop_endSamples - _desc.loop_beginSamples;
        }
        _voice->SubmitSourceBuffer(&_buffer, nullptr);
        _voice->SetVolume(_desc.volume);
        _voice->SetFrequencyRatio(_desc.frequency);
        _voice->Start();
    }
}

void AudioSystem::Channel::Play(Sound& snd, uint32_t operationSetId) noexcept {
    snd.AddChannel(this);
    _sound = &snd;
    if(const auto* wav = snd.GetWav()) {
        _buffer.pAudioData = wav->GetDataBuffer();
        _buffer.AudioBytes = wav->GetDataBufferSize();
        _buffer.LoopCount = _desc.loop_count;
        _buffer.LoopBegin = 0;
        _buffer.LoopLength = 0;
        if(_desc.loop_count) {
            _buffer.LoopBegin = _desc.loop_beginSamples;
            _buffer.LoopLength = _desc.loop_endSamples - _desc.loop_beginSamples;
        }
        _voice->SubmitSourceBuffer(&_buffer, nullptr);
        _voice->SetVolume(_desc.volume, operationSetId);
        _voice->SetFrequencyRatio(_desc.frequency, operationSetId);
        _voice->Start(0, operationSetId);
    }
}

void AudioSystem::Channel::Stop() noexcept {
    if(_voice) {
        _voice->Stop();
        _voice->FlushSourceBuffers();
    }
}

void AudioSystem::Channel::Stop(uint32_t operationSetId) noexcept {
    if(_voice) {
        _voice->Stop(0, operationSetId);
        _voice->FlushSourceBuffers();
    }
}

void AudioSystem::Channel::Pause() noexcept {
    if(_voice) {
        _voice->Stop();
    }
}

void AudioSystem::Channel::Pause(uint32_t operationSetId) noexcept {
    if(_voice) {
        _voice->Stop(0, operationSetId);
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
    _desc.stopWhenFinishedLooping = value;
}

void AudioSystem::Channel::SetLoopCount(int count) noexcept {
    if(count <= -1) {
        _desc.loop_count = XAUDIO2_LOOP_INFINITE;
    } else {
        count = std::clamp(count, 0, XAUDIO2_MAX_LOOP_COUNT);
        _desc.loop_count = count;
    }
}

uint32_t AudioSystem::Channel::GetLoopCount() const noexcept {
    return _desc.loop_count;
}

void AudioSystem::Channel::SetLoopRange(TimeUtils::FPSeconds start, TimeUtils::FPSeconds end) {
    SetLoopBegin(start);
    SetLoopEnd(end);
}

void AudioSystem::Channel::SetLoopBegin(TimeUtils::FPSeconds start) {
    const auto& fmt = _audio_system->GetLoadedWavFileFormat();
    _desc.loop_beginSamples = static_cast<uint32_t>(fmt.samplesPerSecond * start.count());
}

void AudioSystem::Channel::SetLoopEnd(TimeUtils::FPSeconds end) {
    const auto& fmt = _audio_system->GetLoadedWavFileFormat();
    _desc.loop_endSamples = static_cast<uint32_t>(fmt.samplesPerSecond * end.count());
}

void AudioSystem::Channel::SetVolume(float newVolume) noexcept {
    _desc.volume = newVolume;
}

void AudioSystem::Channel::SetFrequency(float newFrequency) noexcept {
    newFrequency = std::clamp(newFrequency, XAUDIO2_MIN_FREQ_RATIO, _desc.frequency_max);
    _desc.frequency = newFrequency;
}

float AudioSystem::Channel::GetVolume() const noexcept {
    return _desc.volume;
}

float AudioSystem::Channel::GetFrequency() const noexcept {
    return _desc.frequency;
}

AudioSystem::Sound::Sound(AudioSystem& audiosystem, std::filesystem::path filepath)
: _audio_system(&audiosystem) {
    namespace FS = std::filesystem;
    GUARANTEE_OR_DIE(FS::exists(filepath), "Attempting to create sound that does not exist.\n");
    {
        std::error_code ec{};
        filepath = FS::canonical(filepath, ec);
        if(ec || !FileUtils::IsSafeReadPath(filepath)) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
        }
    }
    filepath.make_preferred();
    const auto pred = [&filepath](const auto& wav) { return wav.first == filepath; };
    auto found = std::find_if(std::begin(_audio_system->_wave_files), std::end(_audio_system->_wave_files), pred);
    if(found == _audio_system->_wave_files.end()) {
        _audio_system->RegisterWavFile(filepath);
        found = std::find_if(std::begin(_audio_system->_wave_files), std::end(_audio_system->_wave_files), pred);
    }
    if(found != std::end(_audio_system->_wave_files)) {
        _my_id = _id++;
        _wave_file = found->second.get();
    }
}

void AudioSystem::Sound::AddChannel(Channel* channel) noexcept {
    std::scoped_lock<std::mutex> lock(_cs);
    _channels.push_back(channel);
}

void AudioSystem::Sound::RemoveChannel(Channel* channel) noexcept {
    std::scoped_lock<std::mutex> lock(_cs);
    _channels.erase(std::remove_if(std::begin(_channels), std::end(_channels),
                                   [channel](Channel* c) -> bool { return c == channel; }),
                    std::end(_channels));
}

const std::size_t AudioSystem::Sound::GetId() const noexcept {
    return _my_id;
}

const std::size_t AudioSystem::Sound::GetCount() noexcept {
    return _id;
}

const FileUtils::Wav* const AudioSystem::Sound::GetWav() const noexcept {
    return _wave_file;
}

const std::vector<AudioSystem::Channel*>& AudioSystem::Sound::GetChannels() const noexcept {
    return _channels;
}

void STDMETHODCALLTYPE AudioSystem::EngineCallback::OnCriticalError(HRESULT error) {
    std::ostringstream ss;
    ss << "The Audio System encountered a fatal error: ";
    ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << error;
    ERROR_AND_DIE(ss.str().c_str());
}
