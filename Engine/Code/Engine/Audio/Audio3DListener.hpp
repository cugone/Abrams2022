#pragma once

#include "Engine/Audio/Audio3DCone.hpp"
#include "Engine/Audio/XAudio.hpp"

#include "Engine/Math/Vector3.hpp"

class Audio3DListener {
public:
    void SetOmniDirectional(bool newOmniDirectional) noexcept;
    bool IsOmniDirectional() const noexcept;

    void SetCone(const Audio3DCone& cone) noexcept;
    const Audio3DCone& GetCone() const noexcept;
    Audio3DCone& GetCone() noexcept;
    
    void SetConeOrientation(const Vector3& forward, const Vector3& up) noexcept;

    const Vector3& GetConeForward() const noexcept;
    Vector3& GetConeForward() noexcept;

    const Vector3& GetConeUp() const noexcept;
    Vector3& GetConeUp() noexcept;

    const X3DAUDIO_LISTENER& GetX3DAudioListener() const noexcept;
    const X3DAUDIO_CONE& GetX3DAudioCone() const noexcept;

    Vector3 position{};
    Vector3 velocity{};
protected:
private:
    void UpdateX3DAudio() noexcept;
    void UpdateX3DCone() noexcept;

    X3DAUDIO_LISTENER m_x3dAudioListener{};
    X3DAUDIO_CONE m_x3dCone{};
    Audio3DCone m_cone{Audio3DCone::DefaultDirectionalCone};
    Vector3 m_coneForward{};
    Vector3 m_coneUp{};
    bool m_omniDirectional{true};
};
