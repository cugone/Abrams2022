#include "Engine/Renderer/Mesh.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"

#include "Engine/Services/ServiceLocator.hpp"

#include <type_traits>
#include <vector>

Mesh::Mesh(const Builder& builder) noexcept
: m_builder{builder} {
    /* DO NOTHING */
}

Mesh::Mesh(Builder&& builder) noexcept {
    m_builder.verticies = std::move(builder.verticies);
    m_builder.indicies = std::move(builder.indicies);
    m_builder.draw_instructions = std::move(builder.draw_instructions);
}

Mesh::Builder::Builder(const std::vector<Vertex3D>& verts, const std::vector<unsigned int>& indcs) noexcept
: verticies{verts}
, indicies{indcs} {
    /* DO NOTHING */
}

void Mesh::Builder::Begin(const PrimitiveType& type) noexcept {
    m_current_draw_instruction.type = type;
    m_current_draw_instruction.indexStart = indicies.size();
}

void Mesh::Builder::Begin(const PrimitiveType& type, std::size_t indexStart) noexcept {
    m_current_draw_instruction.type = type;
    m_current_draw_instruction.indexStart = indexStart;
}

void Mesh::Builder::End(Material* mat /* = nullptr */) noexcept {
    m_current_draw_instruction.material = mat;
    m_current_draw_instruction.indexCount = indicies.size() - m_current_draw_instruction.indexStart;
    if(!draw_instructions.empty()) {
        auto& last_inst = draw_instructions.back();
        if(!mat) {
            m_current_draw_instruction.material = last_inst.material;
        }
        if(last_inst == m_current_draw_instruction) {
            ++last_inst.count;
            last_inst.indexCount += m_current_draw_instruction.indexCount;
        } else {
            draw_instructions.push_back(m_current_draw_instruction);
        }
    } else {
        draw_instructions.push_back(m_current_draw_instruction);
    }
}

void Mesh::Builder::Clear() noexcept {
    verticies.clear();
    indicies.clear();
    draw_instructions.clear();
}

bool Mesh::Builder::IsEmpty() const noexcept {
    return verticies.empty() && indicies.empty() && draw_instructions.empty();
}

void Mesh::Builder::SetTangent(const Vector3& tangent) noexcept {
    m_vertex_prototype.tangent = tangent;
}

void Mesh::Builder::SetBitangent(const Vector3& bitangent) noexcept {
    m_vertex_prototype.bitangent = bitangent;
}

void Mesh::Builder::SetNormal(const Vector3& normal) noexcept {
    m_vertex_prototype.normal = normal;
}

void Mesh::Builder::SetAlpha(unsigned char value) noexcept {
    SetAlpha(value / 255.0f);
}

void Mesh::Builder::SetAlpha(float value) noexcept {
    m_vertex_prototype.color.w = value;
}


void Mesh::Builder::SetColor(const Rgba& color) noexcept {
    auto&& [r, g, b, a] = color.GetAsFloats();
    SetColor(Vector4{r, g, b, a});
}

void Mesh::Builder::SetColor(const Vector4& color) noexcept {
    m_vertex_prototype.color = color;
}

void Mesh::Builder::SetUV(const Vector2& uv) noexcept {
    m_vertex_prototype.texcoords = uv;
}

std::size_t Mesh::Builder::AddVertex(const Vector3& position) noexcept {
    m_vertex_prototype.position = position;
    verticies.push_back(m_vertex_prototype);
    return verticies.size() - 1;
}

std::size_t Mesh::Builder::AddVertex(const Vector2& position) noexcept {
    return AddVertex(Vector3{position, 0.0f});
}

std::size_t Mesh::Builder::AddIndicies(const Primitive& type) noexcept {
    switch(type) {
    case Primitive::Point:
        indicies.push_back(static_cast<unsigned int>(verticies.size()) - 1u);
        break;
    case Primitive::Line: {
        const auto v_s = verticies.size();
        indicies.push_back(static_cast<unsigned int>(v_s) - 2);
        indicies.push_back(static_cast<unsigned int>(v_s) - 1);
        break;
    }
    case Primitive::Triangle: {
        const auto v_s = verticies.size();
        indicies.push_back(static_cast<unsigned int>(v_s) - 3u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 2u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 1u);
        break;
    }
    case Primitive::TriangleStrip: {
        const auto v_s = verticies.size();
        indicies.push_back(static_cast<unsigned int>(v_s) - 4u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 3u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 2u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 1u);
        break;
    }
    case Primitive::Quad: {
        const auto v_s = verticies.size();
        indicies.push_back(static_cast<unsigned int>(v_s) - 4u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 3u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 2u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 4u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 2u);
        indicies.push_back(static_cast<unsigned int>(v_s) - 1u);
        break;
    }
    default:
        break;
    }
    return indicies.size() - 1;
}

void Mesh::Render(const Mesh::Builder& builder) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    for(const auto& draw_inst : builder.draw_instructions) {
        renderer->SetMaterial(draw_inst.material);
        if(draw_inst.material) {
            auto cbs = draw_inst.material->GetShader()->GetConstantBuffers();
            auto ccbs = draw_inst.material->GetShader()->GetComputeConstantBuffers();
            const auto cb_size = cbs.size();
            for(int i = 0; i < cb_size; ++i) {
                renderer->SetConstantBuffer(renderer->GetConstantBufferStartIndex() + i, &(cbs.begin() + i)->get());
            }
            renderer->DrawIndexed(draw_inst.type, builder.verticies, builder.indicies, draw_inst.indexCount, draw_inst.indexStart, draw_inst.baseVertexLocation);
            for(int i = 0; i < cb_size; ++i) {
                renderer->SetConstantBuffer(renderer->GetConstantBufferStartIndex() + i, nullptr);
            }
        }
    }
}

void Mesh::Render() const noexcept {
    Render(m_builder);
}
