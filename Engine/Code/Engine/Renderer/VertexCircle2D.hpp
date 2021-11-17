#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

class InputLayout;

struct VertexCircle2D {
    // clang-format off
    VertexCircle2D(const Vector3& pos = Vector3::Zero
        , const Rgba& color = Rgba::White
        , const Vector2& thickness_fade = Vector2{1.0f, 0.00025f}) noexcept
        : position(pos)
        , thickness_fade(thickness_fade) {
        const auto&& [r, g, b, a] = color.GetAsFloats();
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    // clang-format on
    VertexCircle2D(const VertexCircle2D& other) = default;
    VertexCircle2D(VertexCircle2D&& other) = default;
    VertexCircle2D& operator=(const VertexCircle2D& other) = default;
    VertexCircle2D& operator=(VertexCircle2D&& other) = default;

    Vector3 position{Vector3::Zero};
    Vector4 color{Vector4::One};
    Vector2 thickness_fade{ 1.0f, 0.00025f };

protected:
private:
};
