#pragma once

#include "Engine/Audio/Audio3DCone.hpp"
#include "Engine/Math/Vector3.hpp"

class Audio3DEmitter {
public:
    Audio3DEmitter() noexcept = default;

    const Audio3DCone& Get3DCone() const noexcept;
    Audio3DCone& Get3DCone() noexcept;

    void SetOmniDirectional(bool newOmniDirectional) noexcept;
    bool IsOmniDirectional() const noexcept;

    uint32_t GetChannelCount() const noexcept;
    float GetChannelRadius() const noexcept;

    float GetInnerRadius() const noexcept;
    float GetInnerAngle() const noexcept;

    void SetConeOrientation(const Vector3& forward, const Vector3& up) noexcept;
    Vector3 GetConeForward() const noexcept;
    Vector3 GetConeUp() const noexcept;

    Vector3 position{};
    Vector3 velocity{};

protected:
private:
    Audio3DCone m_cone{};
    Vector3 m_coneForward{Vector3::Z_Axis};
    Vector3 m_coneUp{Vector3::Y_Axis};
    Vector2 m_innerRadiusAndAngle{};
    uint32_t m_channelCount{1u};
    float m_channelRadius{};
    bool m_omniDirectional{true};
};
