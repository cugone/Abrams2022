#pragma once

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Renderer/Vertex3DInstanced.hpp"
#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/DrawInstruction.hpp"

#include <vector>

class MeshInstanced {
public:
    MeshInstanced() = default;
    MeshInstanced(const MeshInstanced& other) = default;
    MeshInstanced(MeshInstanced&& other) = default;
    MeshInstanced& operator=(const MeshInstanced& other) = default;
    MeshInstanced& operator=(MeshInstanced&& other) = default;
    ~MeshInstanced() = default;

    class Builder {
    public:
        // clang-format off
        enum class Primitive {
            Point
            , Line
            , Triangle
            , TriangleStrip
            , Quad
        };
        // clang-format on
        Builder() = default;
        Builder(const Builder& other) = default;
        Builder(Builder&& other) = default;
        Builder& operator=(const Builder& other) = default;
        Builder& operator=(Builder&& other) = default;
        ~Builder() = default;

        std::vector<Vertex3D> verticies{};
        std::vector<unsigned int> indicies{};
        std::vector<DrawInstruction> draw_instructions{};

        void Begin(const PrimitiveType& type) noexcept;
        void Begin(const PrimitiveType& type, std::size_t indexStart) noexcept;
        void End(Material* mat = nullptr) noexcept;
        void Clear() noexcept;

        void SetTangent(const Vector3& tangent) noexcept;
        void SetBitangent(const Vector3& bitangent) noexcept;
        void SetNormal(const Vector3& normal) noexcept;
        void SetAlpha(unsigned char value) noexcept;
        void SetAlpha(float value) noexcept;
        void SetColor(const Rgba& color) noexcept;
        void SetColor(const Vector4& color) noexcept;
        void SetUV(const Vector2& uv) noexcept;

        std::size_t AddVertex(const Vector3& position) noexcept;
        std::size_t AddVertex(const Vector2& position) noexcept;
        std::size_t AddIndicies(const Primitive& type) noexcept;

    private:
        Vertex3D _vertex_prototype{};
        DrawInstruction _current_draw_instruction{};
    };

    static void Render(const MeshInstanced::Builder& builder, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) noexcept;
    void Render(const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) const noexcept;

protected:
    MeshInstanced::Builder m_builder{};

private:
};
