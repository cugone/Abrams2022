#include "Engine/Renderer/Camera2D.hpp"

#include <algorithm>

void Camera2D::SetupView(const Vector2& leftBottom, const Vector2& rightTop, const Vector2& nearFar /*= Vector2(0.0f, 1.0f)*/, float aspectRatio /*= MathUtils::M_16_BY_9_RATIO*/) noexcept {
    leftBottom_view = leftBottom;
    rightTop_view = rightTop;
    aspect_ratio = aspectRatio;
    nearFar_distance = nearFar;
    CalcViewMatrix();
    CalcProjectionMatrix();
    CalcViewProjectionMatrix();
}

void Camera2D::CalcViewProjectionMatrix() noexcept {
    view_projection_matrix = Matrix4::MakeViewProjection(view_matrix, projection_matrix);
    inv_view_projection_matrix = Matrix4::CalculateInverse(view_projection_matrix);
}

void Camera2D::CalcProjectionMatrix() noexcept {
    projection_matrix = Matrix4::CreateDXOrthographicProjection(leftBottom_view.x, rightTop_view.x, leftBottom_view.y, rightTop_view.y, nearFar_distance.x, nearFar_distance.y);
    inv_projection_matrix = Matrix4::CalculateInverse(projection_matrix);
}

void Camera2D::CalcViewMatrix() noexcept {
    Matrix4 vT = Matrix4::CreateTranslationMatrix(-position);
    Matrix4 vR = Matrix4::Create2DRotationDegreesMatrix(orientation_degrees);
    view_matrix = Matrix4::MakeRT(vR, vT);
    inv_view_matrix = Matrix4::CalculateInverse(view_matrix);
}

void Camera2D::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    trauma -= trauma_recovery_rate * deltaSeconds.count();
    trauma = std::clamp(trauma, 0.0f, 1.0f);
}

const Vector2& Camera2D::GetPosition() const noexcept {
    return position;
}

void Camera2D::SetPosition(const Vector3& newPosition) noexcept {
    SetPosition(Vector2(newPosition));
}

void Camera2D::SetPosition(const Vector2& newPosition) noexcept {
    position = newPosition;
}

void Camera2D::Translate(const Vector3& displacement) noexcept {
    Translate(Vector2{displacement.x, displacement.y});
}

void Camera2D::Translate(const Vector2& displacement) noexcept {
    position += displacement;
}

float Camera2D::GetOrientationDegrees() const noexcept {
    return orientation_degrees;
}

void Camera2D::SetOrientationDegrees(float newAngleDegrees) noexcept {
    orientation_degrees = newAngleDegrees;
}

void Camera2D::ApplyOrientationDegrees(float addAngleDegrees) noexcept {
    orientation_degrees += addAngleDegrees;
}

float Camera2D::GetOrientation() const noexcept {
    return MathUtils::ConvertDegreesToRadians(orientation_degrees);
}

void Camera2D::SetOrientation(float newAngleRadians) noexcept {
    orientation_degrees = MathUtils::ConvertRadiansToDegrees(newAngleRadians);
}

void Camera2D::ApplyOrientation(float addAngleRadians) noexcept {
    orientation_degrees += MathUtils::ConvertRadiansToDegrees(addAngleRadians);
}

float Camera2D::GetAspectRatio() const noexcept {
    return aspect_ratio;
}

float Camera2D::GetInverseAspectRatio() const noexcept {
    return 1.0f / aspect_ratio;
}

float Camera2D::GetNearDistance() const noexcept {
    return nearFar_distance.x;
}

float Camera2D::GetFarDistance() const noexcept {
    return nearFar_distance.y;
}

const Matrix4& Camera2D::GetViewMatrix() const noexcept {
    return view_matrix;
}

const Matrix4& Camera2D::GetProjectionMatrix() const noexcept {
    return projection_matrix;
}

const Matrix4& Camera2D::GetViewProjectionMatrix() const noexcept {
    return view_projection_matrix;
}

const Matrix4& Camera2D::GetInverseViewMatrix() const noexcept {
    return inv_view_matrix;
}

const Matrix4& Camera2D::GetInverseProjectionMatrix() const noexcept {
    return inv_projection_matrix;
}

const Matrix4& Camera2D::GetInverseViewProjectionMatrix() const noexcept {
    return inv_view_projection_matrix;
}

const RenderTargetStack::Node& Camera2D::GetRenderTarget() const noexcept {
    return _render_target;
}

RenderTargetStack::Node& Camera2D::GetRenderTarget() noexcept {
    return _render_target;
}

Vector2 Camera2D::GetViewDimensions() const noexcept {
    return Vector2{GetViewWidth(), GetViewHeight()};
}

float Camera2D::GetViewHeight() const noexcept {
    return leftBottom_view.y - rightTop_view.y;
}

float Camera2D::GetViewWidth() const noexcept {
    return rightTop_view.x - leftBottom_view.x;
}

float Camera2D::GetShake() const noexcept {
    return trauma * trauma;
}

/* static */
Vector2 Camera2D::WindowToWorldPoint(const Camera2D& camera, const Vector2& screenPoint) noexcept {
    return camera.WindowToWorldPoint(screenPoint);
}

Vector2 Camera2D::WindowToWorldPoint(const Vector2& screenPoint) const noexcept {
    const auto view_dims = GetViewDimensions();
    float x = 2.0f * screenPoint.x / view_dims.x - 1.0f;
    float y = 2.0f * screenPoint.y / view_dims.y - 1.0f;
    const auto screenPos = Vector4{x, -y, 0.0f, 1.0f};
    const auto& s2w = GetInverseViewProjectionMatrix();
    const auto worldPos = s2w * screenPos;
    return Vector2{worldPos.x, worldPos.y};
}
