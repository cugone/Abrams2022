#include "Engine/Audio/Audio3DEmitter.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"

#include <utility>

const Audio3DCone& Audio3DEmitter::Get3DCone() const noexcept {
    return m_cone;
}

Audio3DCone& Audio3DEmitter::Get3DCone() noexcept {
    return m_cone;
}

void Audio3DEmitter::SetOmniDirectional(bool newOmniDirectional) noexcept {
    m_omniDirectional = newOmniDirectional;
}

bool Audio3DEmitter::IsOmniDirectional() const noexcept {
    return m_omniDirectional;
}

uint32_t Audio3DEmitter::GetChannelCount() const noexcept {
    return m_channelCount;
}

float Audio3DEmitter::GetChannelRadius() const noexcept {
    return m_channelRadius;
}

float Audio3DEmitter::GetInnerRadius() const noexcept {
    return m_innerRadiusAndAngle.x;
}

float Audio3DEmitter::GetInnerAngle() const noexcept {
    return m_innerRadiusAndAngle.y;
}

void Audio3DEmitter::SetConeOrientation(const Vector3& forward, const Vector3& up) noexcept {
    m_coneForward = forward.GetNormalize();
    m_coneUp = (up - MathUtils::Project(up, m_coneForward)).GetNormalize();
}

Vector3 Audio3DEmitter::GetConeForward() const noexcept {
    return m_coneForward;
}

Vector3 Audio3DEmitter::GetConeUp() const noexcept {
    return m_coneUp;
}
