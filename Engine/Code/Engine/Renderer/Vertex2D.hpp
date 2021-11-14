#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

class InputLayout;

struct Vertex2D {
    // clang-format off
    Vertex2D(const Vector3& pos = Vector3::Zero
        , const Rgba& color = Rgba::White
        , const Vector2& tex_coords = Vector2::Zero) noexcept
        : position(pos)
        , texcoords(tex_coords) {
        const auto&& [r, g, b, a] = color.GetAsFloats();
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    // clang-format on
    Vertex2D(const Vertex2D& other) = default;
    Vertex2D(Vertex2D&& other) = default;
    Vertex2D& operator=(const Vertex2D& other) = default;
    Vertex2D& operator=(Vertex2D&& other) = default;
    Vector3 position = Vector3::Zero;
    Vector4 color = Vector4::One;
    Vector2 texcoords = Vector2::Zero;

protected:
private:
};
