#include "Engine/Renderer/Camera.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

void Camera::SetProjectionMode(ProjectionMode newProjectionMode) noexcept {
    projection_mode = newProjectionMode;
    RecalculateProjectionMatrix();
    RecalculateViewMatrix();
}

void Camera::RecalculateProjectionMatrix() noexcept {
    switch(projection_mode) {
    case ProjectionMode::Orthographic:
        projection_matrix = Matrix4::CreateDXOrthographicProjection(ortho_width * -0.5f, ortho_width * 0.5f,  (ortho_width / aspect_ratio) * 0.5f, (ortho_width / aspect_ratio) * -0.5f, near_distance, far_distance);
        inv_projection_matrix = Matrix4::CalculateInverse(projection_matrix);
        break;
    case ProjectionMode::Perspective:
        projection_matrix = Matrix4::CreateDXPerspectiveProjection(fov, aspect_ratio, near_distance, far_distance);
        inv_projection_matrix = Matrix4::CalculateInverse(projection_matrix);
        break;
    default:
        /* DO NOTHING */
        break;
    }
}

void Camera::RecalculateViewMatrix() noexcept {
    const Matrix4 vT = Matrix4::CreateTranslationMatrix(-position);
    const Matrix4 vR = [this]()-> const Matrix4 {
        switch(projection_mode) {
        case ProjectionMode::Orthographic:
            return Matrix4::Create3DZRotationDegreesMatrix(rotation.roll);
        case ProjectionMode::Perspective:
            return Matrix4::CreateYZXRotationMatrixDegrees(rotation.pitch, rotation.yaw, rotation.roll);
        default:
            return Matrix4::I;
        }

    }(); //IIIL
    view_matrix = Matrix4::MakeRT(vR, vT);
    inv_view_matrix = Matrix4::CalculateInverse(view_matrix);
    view_projection_matrix = Matrix4::MakeViewProjection(view_matrix, projection_matrix);
    inv_view_projection_matrix = Matrix4::CalculateInverse(view_projection_matrix);
}

ProjectionMode Camera::GetProjectionMode() const noexcept {
    return projection_mode;
}

void Camera::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    trauma -= trauma_recovery_rate * deltaSeconds.count();
    if(trauma < 0.0f) {
        trauma = 0.0f;
    }
}

const Vector3& Camera::GetPosition() const noexcept {
    return position;
}

void Camera::SetPosition(const Vector3& newPosition) noexcept {
    position = newPosition;
    RecalculateViewMatrix();
}

void Camera::SetPosition(const Vector2& newPosition) noexcept {
    SetPosition(Vector3{newPosition, 0.0f});
}

void Camera::Translate(const Vector3& displacement) noexcept {
    SetPosition(position + displacement);
}

void Camera::Translate(const Vector2& displacement) noexcept {
    Translate(Vector3{displacement, 0.0f});
}

float Camera::GetAspectRatio() const noexcept {
    return aspect_ratio;
}

float Camera::GetInverseAspectRatio() const noexcept {
    return 1.0f / aspect_ratio;
}

float Camera::GetNearDistance() const noexcept {
    return near_distance;
}

float Camera::GetFarDistance() const noexcept {
    return far_distance;
}

const Matrix4& Camera::GetViewMatrix() const noexcept {
    return view_matrix;
}

const Matrix4& Camera::GetProjectionMatrix() const noexcept {
    return projection_matrix;
}

const Matrix4& Camera::GetViewProjectionMatrix() const noexcept {
    return view_projection_matrix;
}

const Matrix4& Camera::GetInverseViewMatrix() const noexcept {
    return inv_view_matrix;
}

const Matrix4& Camera::GetInverseProjectionMatrix() const noexcept {
    return inv_projection_matrix;
}

const Matrix4& Camera::GetInverseViewProjectionMatrix() const noexcept {
    return inv_view_projection_matrix;
}

float Camera::GetShake() const noexcept {
    return trauma * trauma;
}

const Matrix4& Camera::GetRotationMatrix() const noexcept {
    return rotation_matrix;
}

Matrix4 Camera::CreateBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    //TODO: May have to change R to 3DZ
    return Matrix4::MakeSRT(rotationMatrix, Matrix4::Create3DYRotationDegreesMatrix(180.0f), inv_view_matrix.GetRotation());
}

Matrix4 Camera::CreateReverseBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    return Matrix4::MakeRT(rotationMatrix, inv_view_matrix.GetRotation());
}

Rotator Camera::GetRotation() const noexcept {
    return rotation;
}

Vector3 Camera::GetEulerAngles() const noexcept {
    return Vector3{GetPitch(), GetYaw(), GetRoll()};
}

void Camera::SetEulerAnglesDegrees(const Vector3& eulerAnglesDegrees) noexcept {
    rotation.pitch = eulerAnglesDegrees.y;
    rotation.roll = eulerAnglesDegrees.x;
    rotation.yaw = eulerAnglesDegrees.z;
}

void Camera::SetForwardFromTarget(const Vector3& lookAtPosition) noexcept {
    Vector3 forward = (lookAtPosition - position).GetNormalize();
    Vector3 right = MathUtils::CrossProduct(GetUp(), forward);
    Vector3 up = MathUtils::CrossProduct(forward, right);
    Matrix4 m;
    m.SetIBasis(Vector4(forward, 0.0f));
    m.SetJBasis(Vector4(right, 0.0f));
    m.SetKBasis(Vector4(up, 0.0f));
    rotation = Rotator{Quaternion(m)};
}

Vector3 Camera::GetRight() const noexcept {
    const auto forward = GetForward();
    const auto up = GetUp();
    return MathUtils::CrossProduct(up, forward);
}

Vector3 Camera::GetUp() const noexcept {
    return Matrix4::CreateYZXRotationMatrixDegrees(rotation.pitch, rotation.yaw, rotation.roll).GetUp();
}

Vector3 Camera::GetForward() const noexcept {
    return Matrix4::CreateYZXRotationMatrixDegrees(rotation.pitch, rotation.yaw, rotation.roll).GetForward();
}

float Camera::GetYaw() const noexcept {
    return rotation.yaw;
}

float Camera::GetPitch() const noexcept {
    return rotation.pitch;
}

float Camera::GetRoll() const noexcept {
    return rotation.roll;
}
