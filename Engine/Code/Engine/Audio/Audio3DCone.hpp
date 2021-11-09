#pragma once

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vector2.hpp"

//Defaults to copied values from of X3DAudioDefault_DirectionalCone

class Audio3DCone {
public:

    static Audio3DCone DefaultDirectionalCone;

    Vector2 GetInnerOuterVolumeLevels() const noexcept;
    void SetVolumeLevels(float inner, float outer) noexcept;
    void SetVolumeLevels(const Vector2& innerOuterLevels) noexcept;

    Vector2 GetInnerOuterAnglesDegrees() const noexcept;
    void SetInnerOuterAnglesDegrees(float innerAngleDegrees, float outerAngleDegrees) noexcept;
    void SetInnerOuterAnglesDegrees(const Vector2& innerOuterAngleDegrees) noexcept;

    Vector2 GetInnerOuterAnglesRadians() const noexcept;
    void SetInnerOuterAnglesRadians(float innerAngleRadians, float outerAngleRadians) noexcept;
    void SetInnerOuterAnglesRadians(const Vector2& innerOuterAngleRadians) noexcept;

    Vector2 GetInnerOuterLPF() const noexcept;
    void SetInnerOuterLPF(float innerLPF, float outerLPF) noexcept;
    void SetInnerOuterLPF(const Vector2& innerOuterLPF) noexcept;

    Vector2 GetInnerOuterReverb() const noexcept;
    void SetInnerOuterReverb(float innerReverb, float outerReverb) noexcept;
    void SetInnerOuterReverb(const Vector2& innerOuterReverb) noexcept;

protected:
private:

    Vector2 m_innerOuterAngleRadians{MathUtils::M_PI_2, MathUtils::M_PI};
    Vector2 m_innerOuterVolume{1.0f, 0.708f};
    Vector2 m_innerOuterLPF{0.0f, 0.25f};
    Vector2 m_innerOuterReverb{0.708f, 1.0f};
};
