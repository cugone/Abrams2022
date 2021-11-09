#include "Engine/Audio/Audio3DCone.hpp"

#include "Engine/Math/MathUtils.hpp"

Audio3DCone Audio3DCone::DefaultDirectionalCone = Audio3DCone{};

Vector2 Audio3DCone::GetInnerOuterVolumeLevels() const noexcept {
    return m_innerOuterVolume;
}

void Audio3DCone::SetVolumeLevels(float inner, float outer) noexcept {
    SetVolumeLevels(Vector2{inner, outer});
}

void Audio3DCone::SetVolumeLevels(const Vector2& innerOuterLevels) noexcept {
    m_innerOuterVolume = innerOuterLevels;
}

Vector2 Audio3DCone::GetInnerOuterAnglesDegrees() const noexcept {
    return Vector2{MathUtils::ConvertRadiansToDegrees(m_innerOuterAngleRadians.x), MathUtils::ConvertRadiansToDegrees(m_innerOuterAngleRadians.y)};
}

Vector2 Audio3DCone::GetInnerOuterAnglesRadians() const noexcept {
    return m_innerOuterAngleRadians;
}

void Audio3DCone::SetInnerOuterAnglesDegrees(float innerAngleDegrees, float outerAngleDegrees) noexcept {
    SetInnerOuterAnglesDegrees(Vector2{innerAngleDegrees, outerAngleDegrees});
}

void Audio3DCone::SetInnerOuterAnglesDegrees(const Vector2& innerOuterAngleDegrees) noexcept {
    SetInnerOuterAnglesRadians(Vector2{MathUtils::ConvertDegreesToRadians(innerOuterAngleDegrees.x), MathUtils::ConvertDegreesToRadians(innerOuterAngleDegrees.y)});
}

void Audio3DCone::SetInnerOuterAnglesRadians(float innerAngleRadians, float outerAngleRadians) noexcept {
    SetInnerOuterAnglesRadians(Vector2{innerAngleRadians, outerAngleRadians});
}

void Audio3DCone::SetInnerOuterAnglesRadians(const Vector2& innerOuterAngleRadians) noexcept {
    m_innerOuterAngleRadians = innerOuterAngleRadians;
}

Vector2 Audio3DCone::GetInnerOuterLPF() const noexcept {
    return m_innerOuterLPF;
}

void Audio3DCone::SetInnerOuterLPF(float innerLPF, float outerLPF) noexcept {
    SetInnerOuterLPF(Vector2{innerLPF, outerLPF});
}

void Audio3DCone::SetInnerOuterLPF(const Vector2& innerOuterLPF) noexcept {
    m_innerOuterLPF = innerOuterLPF;
}

Vector2 Audio3DCone::GetInnerOuterReverb() const noexcept {
    return m_innerOuterReverb;
}

void Audio3DCone::SetInnerOuterReverb(float innerReverb, float outerReverb) noexcept {
    SetInnerOuterReverb(Vector2{innerReverb, outerReverb});
}

void Audio3DCone::SetInnerOuterReverb(const Vector2& innerOuterReverb) noexcept {
    m_innerOuterReverb = innerOuterReverb;
}
