#include "Engine/Core/OrthographicCameraController.hpp"

#include "Engine/Input/KeyCode.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <algorithm>


OrthographicCameraController::OrthographicCameraController() noexcept
    : OrthographicCameraController(1.777778f)
{}

OrthographicCameraController::OrthographicCameraController(float aspectRatio) noexcept
: m_aspectRatio(aspectRatio) {
    /* DO NOTHING */
}

void OrthographicCameraController::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto& input = ServiceLocator::get<IInputService>();
    if(input.IsKeyDown(KeyCode::RButton)) {
        const auto up = -Vector2::Y_Axis * static_cast<float>(m_translationSpeedMultiplier * m_cameraSpeedMultiplier) * m_translationSpeed * deltaSeconds.count();
        const auto down = -up;
        const auto left = -Vector2::X_Axis * static_cast<float>(m_translationSpeedMultiplier * m_cameraSpeedMultiplier) * m_translationSpeed * deltaSeconds.count();
        const auto right = -left;
        if(input.IsKeyDown(KeyCode::W)) {
            Translate(up);
        } else if(input.IsKeyDown(KeyCode::S)) {
            Translate(down);
        }
        if(input.IsKeyDown(KeyCode::A)) {
            Translate(left);
        } else if(input.IsKeyDown(KeyCode::D)) {
            Translate(right);
        }
        if(input.WasMouseWheelJustScrolledUp()) {
            ZoomIn();
        } else if(input.WasMouseWheelJustScrolledDown()) {
            ZoomOut();
        }
    }
    m_zoomLevel = std::clamp(m_zoomLevel, m_minZoomLevel, m_maxZoomLevel);
    m_Camera.Update(deltaSeconds);
    m_ShakyCamera = m_Camera;
    m_ShakyCamera.position.x += m_Camera.GetShake() * m_maxShakeOffsetHorizontal * MathUtils::GetRandomNegOneToOne<float>();
    m_ShakyCamera.position.y += m_Camera.GetShake() * m_maxShakeOffsetVertical * MathUtils::GetRandomNegOneToOne<float>();
    m_ShakyCamera.orientation_degrees += m_Camera.GetShake() * m_maxShakeAngle * MathUtils::GetRandomNegOneToOne<float>();
    m_Camera.SetupView(Vector2{-m_aspectRatio * m_zoomLevel, m_zoomLevel}, Vector2{m_aspectRatio * m_zoomLevel, -m_zoomLevel}, Vector2{0.0f, 1.0f}, m_aspectRatio);
    m_ShakyCamera.SetupView(Vector2{-m_aspectRatio * m_zoomLevel, m_zoomLevel}, Vector2{m_aspectRatio * m_zoomLevel, -m_zoomLevel}, Vector2{0.0f, 1.0f}, m_aspectRatio);
    ServiceLocator::get<IRendererService>().SetCamera(m_ShakyCamera);
}

void OrthographicCameraController::SetupCameraShake(float maxShakeOffsetHorizontal, float maxShakeOffsetVertical, float maxShakeAngleDegrees) {
    m_maxShakeOffsetHorizontal = maxShakeOffsetHorizontal;
    m_maxShakeOffsetVertical = maxShakeOffsetVertical;
    m_maxShakeAngle = maxShakeAngleDegrees;
}

void OrthographicCameraController::SetAspectRatio(float aspectRatio) noexcept {
    m_aspectRatio = aspectRatio;
}

float OrthographicCameraController::GetAspectRatio() const noexcept {
    return m_aspectRatio;
}

void OrthographicCameraController::ResetSpeedMultiplier() noexcept {
    m_cameraSpeedMultiplier = m_defaultCameraSpeedMultiplier;
}

int OrthographicCameraController::GetSpeedMultiplier() const noexcept {
    return m_cameraSpeedMultiplier;
}

void OrthographicCameraController::SetSpeedMultiplier(int newSpeedMultiplier) noexcept {
    m_cameraSpeedMultiplier = newSpeedMultiplier;
}

void OrthographicCameraController::ResetTranslationMultiplier() noexcept {
    m_translationSpeedMultiplier = m_defaultTranslationSpeedMultiplier;
}

void OrthographicCameraController::SetTranslationMultiplier(int newTranslationMultiplier) noexcept {
    m_translationSpeedMultiplier = newTranslationMultiplier;
}

int OrthographicCameraController::GetTranslationMultiplier() const noexcept {
    return m_translationSpeedMultiplier;
}

void OrthographicCameraController::SetPosition(const Vector2& newPosition) noexcept {
    m_Camera.SetPosition(newPosition);
}

void OrthographicCameraController::SetRotationDegrees(float newRotation) noexcept {
    m_Camera.SetOrientationDegrees(newRotation);
}

void OrthographicCameraController::SetRotationRadians(float newRotation) noexcept {
    m_Camera.SetOrientation(newRotation);
}

void OrthographicCameraController::ZoomOut() {
    m_zoomLevel += m_zoomSpeed;
}

void OrthographicCameraController::ZoomIn() {
    m_zoomLevel -= m_zoomSpeed;
}

void OrthographicCameraController::Translate(const Vector2& offset) noexcept {
    m_Camera.Translate(offset / m_zoomLevel);
}

void OrthographicCameraController::TranslateTo(const Vector2& position, TimeUtils::FPSeconds t) noexcept {
    const auto& current_position = m_Camera.GetPosition();
    m_Camera.SetPosition(MathUtils::Interpolate(current_position, position, t.count()));
}

void OrthographicCameraController::RotateDegrees(float offset) noexcept {
    m_Camera.ApplyOrientationDegrees(offset);
}

void OrthographicCameraController::RotateRadians(float offset) noexcept {
    m_Camera.ApplyOrientation(offset);
}

void OrthographicCameraController::ResetZoomLevelRange() noexcept {
    SetZoomLevelRange(Vector2{m_defaultMinZoomLevel, m_defaultMaxZoomLevel});
}

float OrthographicCameraController::GetZoomLevel() const noexcept {
    return m_zoomLevel;
}

float OrthographicCameraController::GetZoomRatio() const noexcept {
    return MathUtils::RangeMap(m_zoomLevel, m_minZoomLevel, m_maxZoomLevel, 0.0f, 1.0f);
}

void OrthographicCameraController::SetZoomLevel(float zoom) noexcept {
    m_zoomLevel = zoom;
}

void OrthographicCameraController::SetZoomLevelRange(const Vector2& minmaxZoomLevel) noexcept {
    SetMinZoomLevel(minmaxZoomLevel.x);
    SetMaxZoomLevel(minmaxZoomLevel.y);
}

void OrthographicCameraController::SetMinZoomLevel(float minimumLevel) noexcept {
    m_minZoomLevel = (std::max)(1.0f, minimumLevel);
}

void OrthographicCameraController::SetMaxZoomLevel(float maximumValue) noexcept {
    m_maxZoomLevel = maximumValue;
}

AABB2 OrthographicCameraController::GetBounds() const noexcept {
    return AABB2{m_Camera.GetPosition(), m_Camera.GetViewWidth(), m_Camera.GetViewWidth()};
}

const Camera2D& OrthographicCameraController::GetCamera() const noexcept {
    return m_Camera;
}

Camera2D& OrthographicCameraController::GetCamera() noexcept {
    return m_Camera;
}

float OrthographicCameraController::GetShake() const noexcept {
    return m_Camera.trauma * m_Camera.trauma;
}

Vector2 OrthographicCameraController::ConvertScreenToWorldCoords(Vector2 screenCoords) const noexcept {
    const auto& renderer = ServiceLocator::get<IRendererService>();
    return renderer.ConvertScreenToWorldCoords(m_Camera, screenCoords);
}

Vector2 OrthographicCameraController::ConvertWorldToScreenCoords(Vector2 worldCoords) const noexcept {
    const auto& renderer = ServiceLocator::get<IRendererService>();
    return renderer.ConvertWorldToScreenCoords(m_Camera, worldCoords);
}
