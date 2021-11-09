#include "Engine/Audio/XAudio.hpp"

#include "Engine/Audio/Audio3DCone.hpp"
#include "Engine/Audio/Audio3DListener.hpp"
#include "Engine/Audio/Audio3DEmitter.hpp"

uint32_t AudioDSPSettingsToX3DAudioDSPFlags(const AudioDSPSettings& settings) noexcept {
    uint32_t result{0u};
    if(settings.use_matrix_table) {
        result |= X3DAUDIO_CALCULATE_MATRIX;
    }
    if(settings.use_delay_array) {
        result |= X3DAUDIO_CALCULATE_DELAY;
    }
    if(settings.use_LPF_directpath) {
        result |= X3DAUDIO_CALCULATE_LPF_DIRECT;
    }
    if(settings.use_LPF_reverbpath) {
        result |= X3DAUDIO_CALCULATE_LPF_REVERB;
    }
    if(settings.use_reverb) {
        result |= X3DAUDIO_CALCULATE_REVERB;
    }
    if(settings.use_doppler) {
        result |= X3DAUDIO_CALCULATE_DOPPLER;
    }
    if(settings.use_emitter_to_listener_angle) {
        result |= X3DAUDIO_CALCULATE_EMITTER_ANGLE;
    }
    if(settings.silence_center_channel) {
        result |= X3DAUDIO_CALCULATE_ZEROCENTER;
    }
    if(settings.redirect_all_sources_to_LFE) {
        result |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
    }
    return result;
}

X3DAUDIO_CONE GetDefaultOmniDirectionalX3DAudioCone() noexcept {
    X3DAUDIO_CONE default_omniDirectional{};
    default_omniDirectional.OuterAngle = MathUtils::M_2PI;
    default_omniDirectional.InnerAngle = MathUtils::M_2PI;
    default_omniDirectional.OuterVolume = 0.0f;
    default_omniDirectional.InnerVolume = 1.0f;
    default_omniDirectional.OuterReverb = 0.0f;
    default_omniDirectional.InnerReverb = 1.0f;
    default_omniDirectional.OuterLPF = 0.0f;
    default_omniDirectional.InnerLPF = 1.0f;
    return default_omniDirectional;
}

X3DAUDIO_CONE GetDefaultDirectionalX3DAudioCone() noexcept {
    return X3DAudioDefault_DirectionalCone;
}

X3DAUDIO_CONE Audio3DConeToX3DAudioCone(const Audio3DCone& cone) noexcept {
    X3DAUDIO_CONE xcone{};
    xcone.InnerAngle = cone.GetInnerOuterAnglesRadians().x;
    xcone.OuterAngle = cone.GetInnerOuterAnglesRadians().y;
    xcone.InnerVolume = cone.GetInnerOuterVolumeLevels().x;
    xcone.OuterVolume = cone.GetInnerOuterVolumeLevels().y;
    xcone.InnerReverb = cone.GetInnerOuterReverb().x;
    xcone.OuterReverb = cone.GetInnerOuterReverb().y;
    xcone.InnerLPF = cone.GetInnerOuterLPF().x;
    xcone.OuterLPF = cone.GetInnerOuterLPF().y;
    return xcone;
}
