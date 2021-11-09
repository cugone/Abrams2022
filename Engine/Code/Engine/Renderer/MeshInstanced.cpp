#include "Engine/Renderer/MeshInstanced.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"

#include "Engine/Services/ServiceLocator.hpp"

#include <type_traits>
#include <vector>

void MeshInstanced::Builder::Begin(const PrimitiveType& type) noexcept {
    Begin(type, indicies.size());
}

void MeshInstanced::Builder::Begin(const PrimitiveType& type, std::size_t indexStart) noexcept {
    _current_draw_instruction.type = type;
    _current_draw_instruction.indexStart = indexStart;
}

void MeshInstanced::Builder::End(Material* mat /* = nullptr */) noexcept {
    _current_draw_instruction.material = mat;
    _current_draw_instruction.indexCount = indicies.size() - _current_draw_instruction.indexStart;
    if(!draw_instructions.empty()) {
        auto& last_inst = draw_instructions.back();
        if(!mat) {
            _current_draw_instruction.material = last_inst.material;
        }
        if(last_inst == _current_draw_instruction) {
            ++last_inst.count;
            last_inst.indexCount += _current_draw_instruction.indexCount;
        } else {
            draw_instructions.push_back(_current_draw_instruction);
        }
    } else {
        draw_instructions.push_back(_current_draw_instruction);
    }
}

void MeshInstanced::Builder::Clear() noexcept {
    verticies.clear();
    indicies.clear();
    draw_instructions.clear();
}

void MeshInstanced::Builder::SetTangent(const Vector3& tangent) noexcept {
    _vertex_prototype.tangent = tangent;
}

void MeshInstanced::Builder::SetBitangent(const Vector3& bitangent) noexcept {
    _vertex_prototype.bitangent = bitangent;
}

void MeshInstanced::Builder::SetNormal(const Vector3& normal) noexcept {
    _vertex_prototype.normal = normal;
}

void MeshInstanced::Builder::SetAlpha(unsigned char value) noexcept {
    SetAlpha(value / 255.0f);
}

void MeshInstanced::Builder::SetAlpha(float value) noexcept {
    _vertex_prototype.color.w = value;
}

void MeshInstanced::Builder::SetColor(const Rgba& color) noexcept {
    auto&& [r, g, b, a] = color.GetAsFloats();
    SetColor(Vector4{r, g, b, a});
}

void MeshInstanced::Builder::SetColor(const Vector4& color) noexcept {
    _vertex_prototype.color = color;
}

void MeshInstanced::Builder::SetUV(const Vector2& uv) noexcept {
    _vertex_prototype.texcoords = uv;
}

std::size_t MeshInstanced::Builder::AddVertex(const Vector3& position) noexcept {
    _vertex_prototype.position = position;
    verticies.push_back(_vertex_prototype);
    return verticies.size() - 1;
}

std::size_t MeshInstanced::Builder::AddVertex(const Vector2& position) noexcept {
    return AddVertex(Vector3{position, 0.0f});
}

std::size_t MeshInstanced::Builder::AddIndicies(const Primitive& type) noexcept {
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

void MeshInstanced::Render(const MeshInstanced::Builder& builder, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) noexcept {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    for(const auto& draw_inst : builder.draw_instructions) {
        renderer.SetMaterial(draw_inst.material);
        if(draw_inst.material) {
            auto cbs = draw_inst.material->GetShader()->GetConstantBuffers();
            auto ccbs = draw_inst.material->GetShader()->GetComputeConstantBuffers();
            const auto cb_size = cbs.size();
            for(int i = 0; i < cb_size; ++i) {
                renderer.SetConstantBuffer(renderer.GetConstantBufferStartIndex() + i, &(cbs.begin() + i)->get());
            }
            renderer.DrawIndexedInstanced(draw_inst.type, builder.verticies, vbio, builder.indicies, instanceCount);
            for(int i = 0; i < cb_size; ++i) {
                renderer.SetConstantBuffer(renderer.GetConstantBufferStartIndex() + i, nullptr);
            }
        }
    }
}

void MeshInstanced::Render(const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) const noexcept {
    Render(m_builder, vbio, instanceCount);
}
