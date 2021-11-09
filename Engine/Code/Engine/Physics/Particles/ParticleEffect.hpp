#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Physics/Particles/ParticleEmitter.hpp"

#include <string>
#include <vector>

class ParticleEffect {
public:
    explicit ParticleEffect(std::string definitionName) noexcept;
    explicit ParticleEffect(const XMLElement& element) noexcept;
    ParticleEffect() noexcept = default;
    ParticleEffect(const ParticleEffect& other) noexcept = default;
    ParticleEffect& operator=(const ParticleEffect& rhs) noexcept = default;
    ParticleEffect(ParticleEffect&& other) noexcept = default;
    ParticleEffect& operator=(ParticleEffect&& rhs) noexcept = default;
    ~ParticleEffect() noexcept = default;

    bool PlayOnce() const;
    void PlayOnce(bool value);
    void SetPlay(bool value);
    bool IsDestroyedOnFinish() const;
    bool IsPlaying() const;
    bool IsFinished() const;

    void Stop();
    void BeginFrame();
    void Update(float time, float deltaSeconds);
    void Render() const;
    void EndFrame();

    const std::string& GetName() const;

    const std::vector<ParticleEmitter>& GetEmitters() const;
    std::vector<ParticleEmitter>& GetEmitters();

    Vector3 position{Vector3::Zero};
    Vector3 velocity{Vector3::Zero};

protected:
private:
    std::string _definitionName{};
    std::vector<ParticleEmitter> _emitters{};
    bool _is_playing{false};
    bool _destroy_on_finish{false};

    void LoadFromXml(const XMLElement& element);
    float GetLongestLifetime() const;
};
