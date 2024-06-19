#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Camera2D.hpp"

#include <ratio>

class Renderer;
class InputSystem;

class OrthographicCameraController {
public:
    struct Options {
        bool lockInput{false};
        bool lockTranslation{false};
        bool lockZoom{false};
    };
    OrthographicCameraController(Options options = Options{}) noexcept;
    explicit OrthographicCameraController(float aspectRatio, Options options = Options{}) noexcept;
    
    template<intmax_t N, intmax_t D>
    explicit OrthographicCameraController(std::ratio<N, D> aspectRatio = MathUtils::WIDESCREEN_RATIO) noexcept
    : OrthographicCameraController(static_cast<float>(aspectRatio.num) / static_cast<float>(aspectRatio.den))
    { /* DO NOTHING */ }

    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    void SetupCameraShake(float maxShakeOffsetHorizontal, float maxShakeOffsetVertical, float maxShakeAngleDegrees);

    void SetAspectRatio(float aspectRatio) noexcept;
    [[nodiscard]] float GetAspectRatio() const noexcept;

    void ResetSpeedMultiplier() noexcept;
    void SetSpeedMultiplier(int newSpeedMultiplier) noexcept;
    int GetSpeedMultiplier() const noexcept;

    void ResetTranslationMultiplier() noexcept;
    void SetTranslationMultiplier(int newTranslationMultiplier) noexcept;
    int GetTranslationMultiplier() const noexcept;

    void SetPosition(const Vector2& newPosition) noexcept;
    void Translate(const Vector2& offset) noexcept;
    void TranslateTo(const Vector2& position, TimeUtils::FPSeconds t) noexcept;
    void SetRotationDegrees(float newRotation) noexcept;
    void SetRotationRadians(float newRotation) noexcept;
    void RotateDegrees(float offset) noexcept;
    void RotateRadians(float offset) noexcept;

    void ZoomIn();
    void ZoomOut();

    void ResetZoomLevelRange() noexcept;
    float GetZoomLevel() const noexcept;
    float GetZoomRatio() const noexcept;
    void SetZoomLevel(float zoom) noexcept;
    void SetZoomLevelRange(const Vector2& minmaxZoomLevel) noexcept;
    void SetMinZoomLevel(float minimumLevel) noexcept;
    void SetMaxZoomLevel(float maximumValue) noexcept;

    AABB2 GetBounds() const noexcept;

    [[nodiscard]] const Camera2D& GetCamera() const noexcept;
    [[nodiscard]] Camera2D& GetCamera() noexcept;

    [[nodiscard]] float GetShake() const noexcept;

    template<typename F>
    void DoCameraShake(F&& f) {
        m_Camera.trauma = std::invoke(std::forward<F>(f));
    }

    Vector2 ConvertScreenToWorldCoords(Vector2 screenCoords) const noexcept;
    Vector2 ConvertWorldToScreenCoords(Vector2 worldCoords) const noexcept;

    void SetModelViewProjection() noexcept;
    AABB2 CalcOrthoBounds() const noexcept;
    AABB2 CalcViewBounds() const noexcept;
    AABB2 CalcCullBounds() const noexcept;

    void SetModelViewProjectionBounds(Vector2 near_far_distances = Vector2{0.0f, 1000.0f}, Vector3 max_shake_offsets = Vector3{2.5f, 25.0f, 25.0f}) const noexcept;

protected:
private:

    float m_aspectRatio = MathUtils::M_16_BY_9_RATIO;
    float m_zoomLevel = 8.0f;
    float m_defaultMinZoomLevel = 8.0f;
    float m_defaultMaxZoomLevel = (std::numeric_limits<float>::max)();
    float m_minZoomLevel = 8.0f;
    float m_maxZoomLevel = (std::numeric_limits<float>::max)();
    mutable Camera2D m_Camera{};
    Camera2D m_ShakyCamera{};
    float m_maxShakeOffsetHorizontal{10.0f};
    float m_maxShakeOffsetVertical{10.0f};
    float m_maxShakeAngle{25.0f};
    float m_defaultTranslationSpeed = 5.0f;
    float m_translationSpeed = 5.0f;
    int m_defaultTranslationSpeedMultiplier = 1;
    int m_translationSpeedMultiplier = 1;
    int m_defaultCameraSpeedMultiplier = 1;
    int m_cameraSpeedMultiplier = 1;
    float m_defaultRotationSpeed = 180.0f;
    float m_rotationSpeed = 180.0f;
    float m_zoomSpeed = 8.0f;
    float m_maxZoomSpeed = 24.0f;
    Options m_options = Options{};
};
