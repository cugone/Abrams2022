#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

class InputLayout;

struct Vertex3DInstanced {
    // clang-format off
    Vertex3DInstanced(const Vector3& pos = Vector3::Zero
            ,const Rgba& color = Rgba::White
            ,const Vector2& tex_coords = Vector2::Zero
            ,const Vector3& normal = Vector3::Z_Axis
            ,const Vector3& tangent = Vector3::Zero
            ,const Vector3& bitangent = Vector3::Zero) noexcept
    : position(pos)
    , color(color.GetAsFloats())
    , texcoords(tex_coords)
    , normal(normal)
    , tangent(tangent)
    , bitangent(bitangent) {
        /* DO NOTHING */
    }
    // clang-format on
    Vertex3DInstanced(const Vertex3DInstanced& other) = default;
    Vertex3DInstanced(Vertex3DInstanced&& other) = default;
    Vertex3DInstanced& operator=(const Vertex3DInstanced& other) = default;
    Vertex3DInstanced& operator=(Vertex3DInstanced&& other) = default;
    Vector3 position = Vector3::Zero;
    Vector4 color = Vector4::One;
    Vector2 texcoords = Vector2::Zero;
    Vector3 normal = Vector3::Z_Axis;
    Vector3 tangent = Vector3::Zero;
    Vector3 bitangent = Vector3::Zero;

protected:
private:
};
