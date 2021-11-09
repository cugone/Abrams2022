#include "Engine/Math/Frustum.hpp"

#include "Engine/Math/Matrix4.hpp"
#include "Engine/Renderer/Camera3D.hpp"

#include <type_traits>

Frustum Frustum::CreateFromViewProjectionMatrix(const Matrix4& viewProjection, float aspectRatio, float vfovDegrees, const Vector3& forward, float near, float far, bool normalize) noexcept {
    return Frustum(viewProjection, aspectRatio, vfovDegrees, forward, near, far, normalize);
}

Frustum Frustum::CreateFromCamera(const Camera3D& camera, bool normalize) noexcept {
    const auto& ar = camera.GetAspectRatio();
    const auto& vp = camera.GetViewProjectionMatrix();
    const auto& fov = camera.CalcFovYDegrees();
    const auto& near = camera.GetNearDistance();
    const auto& far = camera.GetFarDistance();
    const auto& forward = camera.GetForward();
    return CreateFromViewProjectionMatrix(vp, ar, fov, forward, near, far, normalize);
}

Frustum::Frustum(const Matrix4& viewProjectionMatrix, float aspectRatio, float vfovDegrees, const Vector3& forward, float near, float far, bool normalize) noexcept {
    CalcPoints(vfovDegrees, aspectRatio, forward, near, far);

    const auto x = viewProjectionMatrix.GetXComponents();
    const auto y = viewProjectionMatrix.GetYComponents();
    const auto z = viewProjectionMatrix.GetZComponents();
    const auto w = viewProjectionMatrix.GetWComponents();
    {
        const auto a_l = x.w + x.x;
        const auto b_l = y.w + y.x;
        const auto c_l = z.w + z.x;
        const auto d_l = w.w + w.x;
        auto result = Plane3{Vector3{a_l, b_l, c_l}, d_l};
        if(normalize) {
            result.Normalize();
        }
        SetLeft(result);
    }
    {
        const auto a_r = x.w - x.x;
        const auto b_r = y.w - y.x;
        const auto c_r = z.w - z.x;
        const auto d_r = w.w - w.x;
        auto result = Plane3{Vector3{a_r, b_r, c_r}, d_r};
        if(normalize) {
            result.Normalize();
        }
        SetRight(result);
    }
    {
        const auto a_b = x.w + x.y;
        const auto b_b = y.w + y.y;
        const auto c_b = z.w + z.y;
        const auto d_b = w.w + w.y;
        auto result = Plane3{Vector3{a_b, b_b, c_b}, d_b};
        if(normalize) {
            result.Normalize();
        }
        SetBottom(result);
    }
    {
        const auto a_t = x.w - x.y;
        const auto b_t = y.w - y.y;
        const auto c_t = z.w - z.y;
        const auto d_t = w.w - w.y;
        auto result = Plane3{Vector3{a_t, b_t, c_t}, d_t};
        if(normalize) {
            result.Normalize();
        }
        SetTop(result);
    }
    {
        const auto a_n = z.x;
        const auto b_n = z.y;
        const auto c_n = z.z;
        const auto d_n = z.w;
        auto result = Plane3{Vector3{a_n, b_n, c_n}, d_n};
        if(normalize) {
            result.Normalize();
        }
        SetNear(result);
    }
    {
        const auto a_f = w.x - z.x;
        const auto b_f = w.y - z.y;
        const auto c_f = w.z - z.z;
        const auto d_f = w.w - z.w;
        auto result = Plane3{Vector3{a_f, b_f, c_f}, d_f};
        if(normalize) {
            result.Normalize();
        }
        SetFar(result);
    }
}

void Frustum::SetLeft(const Plane3& left) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Left)] = left;
}

void Frustum::SetRight(const Plane3& right) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Right)] = right;
}

void Frustum::SetTop(const Plane3& top) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Top)] = top;
}

void Frustum::SetBottom(const Plane3& bottom) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Bottom)] = bottom;
}

void Frustum::SetNear(const Plane3& near) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Near)] = near;
}

void Frustum::SetFar(const Plane3& far) noexcept {
    _planes[static_cast<std::size_t>(PlaneDirection::Far)] = far;
}

void Frustum::CalcPoints(float aspectRatio, float vfovDegrees, const Vector3& forward, float near, float far) noexcept {
    const auto aspect_ratio = aspectRatio;
    const auto fov_vertical_degrees = vfovDegrees;
    const auto near_distance = near;
    const auto far_distance = far;
    const auto near_view_half_height = near_distance * std::tan(0.5f * fov_vertical_degrees);
    const auto near_view_half_width = aspect_ratio * near_view_half_height;
    const auto far_view_half_height = far_distance * std::tan(0.5f * fov_vertical_degrees);
    const auto far_view_half_width = aspect_ratio * far_view_half_height;

    _points[0] = forward * Vector3{-near_view_half_width, -near_view_half_height, near_distance};
    _points[1] = forward * Vector3{-near_view_half_width, near_view_half_height, near_distance};
    _points[2] = forward * Vector3{near_view_half_width, near_view_half_height, near_distance};
    _points[3] = forward * Vector3{near_view_half_width, -near_view_half_height, near_distance};

    _points[4] = forward * Vector3{-far_view_half_width, -far_view_half_height, far_distance};
    _points[5] = forward * Vector3{-far_view_half_width, far_view_half_height, far_distance};
    _points[6] = forward * Vector3{far_view_half_width, far_view_half_height, far_distance};
    _points[7] = forward * Vector3{far_view_half_width, -far_view_half_height, far_distance};
}

const Plane3& Frustum::GetLeft() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Left)];
}

const Plane3& Frustum::GetRight() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Right)];
}

const Plane3& Frustum::GetTop() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Top)];
}

const Plane3& Frustum::GetBottom() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Bottom)];
}

const Plane3& Frustum::GetNear() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Near)];
}

const Plane3& Frustum::GetFar() const noexcept {
    return _planes[static_cast<std::size_t>(PlaneDirection::Far)];
}

const Vector3& Frustum::GetNearBottomLeft() const noexcept {
    return _points[0];
}

const Vector3& Frustum::GetNearTopLeft() const noexcept {
    return _points[1];
}

const Vector3& Frustum::GetNearTopRight() const noexcept {
    return _points[2];
}

const Vector3& Frustum::GetNearBottomRight() const noexcept {
    return _points[3];
}

const Vector3& Frustum::GetFarBottomLeft() const noexcept {
    return _points[4];
}

const Vector3& Frustum::GetFarTopLeft() const noexcept {
    return _points[5];
}

const Vector3& Frustum::GetFarTopRight() const noexcept {
    return _points[6];
}

const Vector3& Frustum::GetFarBottomRight() const noexcept {
    return _points[7];
}
