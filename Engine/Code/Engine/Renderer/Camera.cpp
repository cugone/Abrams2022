#include "Engine/Renderer/Camera.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

void Camera::SetProjectionMode(ProjectionMode newProjectionMode) noexcept {
    m_projection_mode = newProjectionMode;
    RecalculateProjectionMatrix();
    RecalculateViewMatrix();
}

void Camera::RecalculateProjectionMatrix() noexcept {
    switch(m_projection_mode) {
    case ProjectionMode::Orthographic:
        m_projection_matrix = Matrix4::CreateDXOrthographicProjection(m_ortho_width * -0.5f, m_ortho_width * 0.5f,  (m_ortho_width / m_aspect_ratio) * 0.5f, (m_ortho_width / m_aspect_ratio) * -0.5f, m_near_distance, m_far_distance);
        m_inv_projection_matrix = Matrix4::CalculateInverse(m_projection_matrix);
        break;
    case ProjectionMode::Perspective:
        m_projection_matrix = Matrix4::CreateDXPerspectiveProjection(m_fov, m_aspect_ratio, m_near_distance, m_far_distance);
        m_inv_projection_matrix = Matrix4::CalculateInverse(m_projection_matrix);
        break;
    default:
        /* DO NOTHING */
        break;
    }
}

void Camera::RecalculateViewMatrix() noexcept {
    const Matrix4 vT = Matrix4::CreateTranslationMatrix(-m_position);
    const Matrix4 vR = [this]()-> const Matrix4 {
        switch(m_projection_mode) {
        case ProjectionMode::Orthographic:
            return Matrix4::Create3DZRotationDegreesMatrix(m_rotation.roll);
        case ProjectionMode::Perspective:
            return Matrix4::CreateYZXRotationMatrixDegrees(m_rotation.pitch, m_rotation.yaw, m_rotation.roll);
        default:
            return Matrix4::I;
        }

    }(); //IIIL
    m_view_matrix = Matrix4::MakeRT(vR, vT);
    m_inv_view_matrix = Matrix4::CalculateInverse(m_view_matrix);
    m_view_projection_matrix = Matrix4::MakeViewProjection(m_view_matrix, m_projection_matrix);
    m_inv_view_projection_matrix = Matrix4::CalculateInverse(m_view_projection_matrix);
}

ProjectionMode Camera::GetProjectionMode() const noexcept {
    return m_projection_mode;
}

void Camera::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    trauma -= trauma_recovery_rate * deltaSeconds.count();
    if(trauma < 0.0f) {
        trauma = 0.0f;
    }
}

const Vector3& Camera::GetPosition() const noexcept {
    return m_position;
}

void Camera::SetPosition(const Vector3& newPosition) noexcept {
    m_position = newPosition;
    RecalculateViewMatrix();
}

void Camera::SetPosition(const Vector2& newPosition) noexcept {
    SetPosition(Vector3{newPosition, 0.0f});
}

void Camera::Translate(const Vector3& displacement) noexcept {
    SetPosition(m_position + displacement);
}

void Camera::Translate(const Vector2& displacement) noexcept {
    Translate(Vector3{displacement, 0.0f});
}

float Camera::GetAspectRatio() const noexcept {
    return m_aspect_ratio;
}

float Camera::GetInverseAspectRatio() const noexcept {
    return 1.0f / m_aspect_ratio;
}

float Camera::GetNearDistance() const noexcept {
    return m_near_distance;
}

float Camera::GetFarDistance() const noexcept {
    return m_far_distance;
}

const Matrix4& Camera::GetViewMatrix() const noexcept {
    return m_view_matrix;
}

const Matrix4& Camera::GetProjectionMatrix() const noexcept {
    return m_projection_matrix;
}

const Matrix4& Camera::GetViewProjectionMatrix() const noexcept {
    return m_view_projection_matrix;
}

const Matrix4& Camera::GetInverseViewMatrix() const noexcept {
    return m_inv_view_matrix;
}

const Matrix4& Camera::GetInverseProjectionMatrix() const noexcept {
    return m_inv_projection_matrix;
}

const Matrix4& Camera::GetInverseViewProjectionMatrix() const noexcept {
    return m_inv_view_projection_matrix;
}

float Camera::GetShake() const noexcept {
    return trauma * trauma;
}

const Matrix4& Camera::GetRotationMatrix() const noexcept {
    return m_rotation_matrix;
}

Matrix4 Camera::CreateBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    //TODO: May have to change R to 3DZ
    return Matrix4::MakeSRT(rotationMatrix, Matrix4::Create3DYRotationDegreesMatrix(180.0f), m_inv_view_matrix.GetRotation());
}

Matrix4 Camera::CreateReverseBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    return Matrix4::MakeRT(rotationMatrix, m_inv_view_matrix.GetRotation());
}

Rotator Camera::GetRotation() const noexcept {
    return m_rotation;
}

Vector3 Camera::GetEulerAngles() const noexcept {
    return Vector3{GetPitch(), GetYaw(), GetRoll()};
}

void Camera::SetEulerAnglesDegrees(const Vector3& eulerAnglesDegrees) noexcept {
    m_rotation.pitch = eulerAnglesDegrees.y;
    m_rotation.roll = eulerAnglesDegrees.x;
    m_rotation.yaw = eulerAnglesDegrees.z;
}

void Camera::SetForwardFromTarget(const Vector3& lookAtPosition) noexcept {
    Vector3 forward = (lookAtPosition - m_position).GetNormalize();
    Vector3 right = MathUtils::CrossProduct(GetUp(), forward);
    Vector3 up = MathUtils::CrossProduct(forward, right);
    Matrix4 m;
    m.SetIBasis(Vector4(forward, 0.0f));
    m.SetJBasis(Vector4(right, 0.0f));
    m.SetKBasis(Vector4(up, 0.0f));
    m_rotation = Rotator{Quaternion(m)};
}

Vector3 Camera::GetRight() const noexcept {
    const auto forward = GetForward();
    const auto up = GetUp();
    return MathUtils::CrossProduct(up, forward);
}

Vector3 Camera::GetUp() const noexcept {
    return Matrix4::CreateYZXRotationMatrixDegrees(m_rotation.pitch, m_rotation.yaw, m_rotation.roll).GetUp();
}

Vector3 Camera::GetForward() const noexcept {
    return Matrix4::CreateYZXRotationMatrixDegrees(m_rotation.pitch, m_rotation.yaw, m_rotation.roll).GetForward();
}

float Camera::GetYaw() const noexcept {
    return m_rotation.yaw;
}

float Camera::GetPitch() const noexcept {
    return m_rotation.pitch;
}

float Camera::GetRoll() const noexcept {
    return m_rotation.roll;
}
