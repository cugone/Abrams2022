#include "Engine/Audio/Audio3DListener.hpp"

void Audio3DListener::SetOmniDirectional(bool newOmniDirectional) noexcept {
    m_omniDirectional = newOmniDirectional;
}

bool Audio3DListener::IsOmniDirectional() const noexcept {
    return m_omniDirectional;
}

void Audio3DListener::SetCone(const Audio3DCone& cone) noexcept {
    m_cone = cone;
}

const Audio3DCone& Audio3DListener::GetCone() const noexcept {
    return m_cone;
}

Audio3DCone& Audio3DListener::GetCone() noexcept {
    return m_cone;
}

void Audio3DListener::SetConeOrientation(const Vector3& forward, const Vector3& up) noexcept {
    m_coneForward = forward.GetNormalize();
    m_coneUp = (up - MathUtils::Project(up, m_coneForward)).GetNormalize();
}

const Vector3& Audio3DListener::GetConeUp() const noexcept {
    return m_coneUp;
}

Vector3& Audio3DListener::GetConeUp() noexcept {
    return m_coneUp;
}

const X3DAUDIO_LISTENER& Audio3DListener::GetX3DAudioListener() const noexcept {
    return m_x3dAudioListener;
}

const X3DAUDIO_CONE& Audio3DListener::GetX3DAudioCone() const noexcept {
    return m_x3dCone;
}

void Audio3DListener::UpdateX3DAudio() noexcept {
    m_x3dAudioListener.OrientFront = X3DAUDIO_VECTOR{GetConeForward().GetAsFloatArray()};
    m_x3dAudioListener.OrientTop = X3DAUDIO_VECTOR{GetConeUp().GetAsFloatArray()};
    m_x3dAudioListener.Position = X3DAUDIO_VECTOR{position.GetAsFloatArray()};
    m_x3dAudioListener.Velocity = X3DAUDIO_VECTOR{velocity.GetAsFloatArray()};
    m_x3dAudioListener.pCone = IsOmniDirectional() ? nullptr : &m_x3dCone;
}

void Audio3DListener::UpdateX3DCone() noexcept {
    m_x3dCone = Audio3DConeToX3DAudioCone(GetCone());
}

const Vector3& Audio3DListener::GetConeForward() const noexcept {
    return m_coneForward;
}

Vector3& Audio3DListener::GetConeForward() noexcept {
    return m_coneForward;
}
