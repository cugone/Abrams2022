#pragma once

#include <Xaudio2.h>
#include <x3daudio.h>

#pragma comment(lib, "Xaudio2.lib")

class Audio3DCone;
class Audio3DEmitter;
class Audio3DListener;

struct AudioDSPResults {
    float dopplerFactor{1.0f};
    float emitterToListenerAngleRadians{0.0f};
    float emitterToListenerDistance{0.0f};
    float emitterVelocityComponent{0.0f};
    float lowPassFilterDirectCoefficient{1.0f};
    float lowPassFilterReverbCoefficient{1.0f};
    float reverbLevel{};
    float** pMatrixCoefficients{};
};

struct AudioDSPSettings {
    uint8_t use_matrix_table : 1;              //enable matrix coefficient table calculation
    uint8_t use_delay_array : 1;               //enable delay time array calculation (stereo final mix only)
    uint8_t use_LPF_directpath : 1;            //enable LPF direct-path coefficient calculation
    uint8_t use_LPF_reverbpath : 1;            //enable LPF reverb-path coefficient calculation
    uint8_t use_reverb : 1;                    // enable reverb send level calculation
    uint8_t use_doppler : 1;                   // enable doppler shift factor calculation
    uint8_t use_emitter_to_listener_angle : 1; // enable emitter-to-listener interior angle calculation
    uint8_t silence_center_channel : 1;        // do not position to front center speaker, signal positioned to remaining speakers instead, front center destination channel will be zero in returned matrix coefficient table, valid only for matrix calculations with final mix formats that have a front center channel
    uint8_t redirect_all_sources_to_LFE : 1;   // apply equal mix of all source channels to LFE destination channel, valid only for matrix calculations with sources that have no LFE channel and final mix formats that have an LFE channel
};

uint32_t AudioDSPSettingsToX3DAudioDSPFlags(const AudioDSPSettings& settings) noexcept;
X3DAUDIO_CONE GetDefaultOmniDirectionalX3DAudioCone() noexcept;
X3DAUDIO_CONE GetDefaultDirectionalX3DAudioCone() noexcept;
X3DAUDIO_CONE Audio3DConeToX3DAudioCone(const Audio3DCone& cone) noexcept;
