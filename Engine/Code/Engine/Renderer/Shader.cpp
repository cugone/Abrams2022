#include "Engine/Renderer/Shader.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/InputLayout.hpp"
#include "Engine/Renderer/InputLayoutInstanced.hpp"
#include "Engine/Renderer/RasterState.hpp"
#include "Engine/Renderer/Sampler.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"

#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <system_error>

//Creates 3D3Blob from a data buffer. Takes ownership of the buffer.
ID3DBlob* CreateD3DBlobFromBuffer(std::vector<uint8_t>& buffer, std::string_view error_msg) noexcept;

Shader::Shader(ShaderProgram* shaderProgram /*= nullptr*/, DepthStencilState* depthStencil /*= nullptr*/, RasterState* rasterState /*= nullptr*/, BlendState* blendState /*= nullptr*/, Sampler* sampler /*= nullptr*/) noexcept
: m_shader_program(shaderProgram)
, m_depth_stencil_state(depthStencil)
, m_raster_state(rasterState)
, m_blend_state(blendState)
, m_sampler(sampler) {
    m_name += "_" + std::to_string(m_defaultNameId++);
}

Shader::Shader(const XMLElement& element) noexcept
{
    m_name += "_" + std::to_string(m_defaultNameId++);

    LoadFromXml(element);
}

const std::string& Shader::GetName() const noexcept {
    return m_name;
}

ShaderProgram* Shader::GetShaderProgram() const noexcept {
    return m_shader_program;
}

RasterState* Shader::GetRasterState() const noexcept {
    return m_raster_state;
}

DepthStencilState* Shader::GetDepthStencilState() const noexcept {
    return m_depth_stencil_state.get();
}

BlendState* Shader::GetBlendState() const noexcept {
    return m_blend_state.get();
}

Sampler* Shader::GetSampler() const noexcept {
    return m_sampler;
}

std::vector<std::reference_wrapper<ConstantBuffer>> Shader::GetConstantBuffers() const noexcept {
    std::vector<std::reference_wrapper<ConstantBuffer>> cbufferRefs{};
    cbufferRefs.reserve(m_cbuffers.size());
    for(auto& ptr : m_cbuffers) {
        cbufferRefs.push_back(std::ref(*ptr));
    }
    cbufferRefs.shrink_to_fit();
    return cbufferRefs;
}

std::vector<std::reference_wrapper<ConstantBuffer>> Shader::GetComputeConstantBuffers() const noexcept {
    std::vector<std::reference_wrapper<ConstantBuffer>> cbufferRefs{};
    cbufferRefs.reserve(m_ccbuffers.size());
    for(auto& ptr : m_ccbuffers) {
        cbufferRefs.push_back(std::ref(*ptr));
    }
    cbufferRefs.shrink_to_fit();
    return cbufferRefs;
}

ID3DBlob* CreateD3DBlobFromBuffer(std::vector<uint8_t>& buffer, std::string_view error_msg) noexcept {
    ID3DBlob* blob = nullptr;
    if(auto hr = ::D3DCreateBlob(buffer.size(), &blob); (FAILED(hr))) {
        DebuggerPrintf(StringUtils::FormatWindowsMessage(hr));
        ERROR_AND_DIE(error_msg.data());
    }
    std::memcpy(blob->GetBufferPointer(), buffer.data(), blob->GetBufferSize());
    buffer.clear();
    buffer.shrink_to_fit();
    return blob;
}

bool Shader::LoadFromXml(const XMLElement& element) noexcept {
    namespace FS = std::filesystem;
    DataUtils::ValidateXmlElement(element, "shader", "shaderprogram", "name", "depth,stencil,blends,raster,sampler,cbuffers");

    m_name = DataUtils::ParseXmlAttribute(element, std::string("name"), m_name);

    auto* xml_SP = element.FirstChildElement("shaderprogram");
    DataUtils::ValidateXmlElement(*xml_SP, "shaderprogram", "", "src", "pipelinestages");

    FS::path p;
    {
        const std::string sp_src = DataUtils::ParseXmlAttribute(*xml_SP, "src", std::string{});
        GUARANTEE_OR_DIE(!sp_src.empty(), "shaderprogram element has empty src attribute.");
        p = FS::path(sp_src);
    }
    if(!StringUtils::StartsWith(p.string(), "__")) {
        std::error_code ec{};
        p = FS::canonical(p, ec);
        if(ec) {
            std::cout << ec.message();
            return false;
        }
    }
    p.make_preferred();
    auto* renderer = ServiceLocator::get<IRendererService>();
    if(nullptr == (m_shader_program = renderer->GetShaderProgram(p.string()))) {
        const bool is_cso = p.has_extension() && StringUtils::ToLowerCase(p.extension().string()) == ".cso";
        GUARANTEE_OR_DIE(is_cso, "ShaderProgram source path must be a compiled shader '.cso' file.");
        const auto error_msg = std::format("Intrinsic ShaderProgram referenced in Shader file \"{}\" does not already exist.", m_name);
        GUARANTEE_OR_DIE(!StringUtils::StartsWith(p.string(), "__"), error_msg.c_str());
        if(is_cso) {
            ShaderProgramDesc desc{};
            desc.name = m_name;
            auto& device = *renderer->GetDevice();
            DataUtils::ForEachChildElement(element, "shaderprogram", [this, &desc, &device](const XMLElement& elem) {
                const auto sp_src = DataUtils::ParseXmlAttribute(elem, "src", std::string{});
                auto p = FS::path(sp_src);
                std::error_code ec;
                p = FS::canonical(p, ec);
                GUARANTEE_OR_DIE(!ec, "Compiled shader source path is invalid:\n" + p.string());
                const auto has_filename = p.has_filename();
                GUARANTEE_OR_DIE(has_filename, "Compiled shader source path is not a file.");
                const auto filename = p.stem();
                const auto fn_str = filename.string();
                const auto is_vs = has_filename && fn_str.ends_with("_VS");
                const auto is_hs = has_filename && fn_str.ends_with("_HS");
                const auto is_ds = has_filename && fn_str.ends_with("_DS");
                const auto is_gs = has_filename && fn_str.ends_with("_GS");
                const auto is_ps = has_filename && fn_str.ends_with("_PS");
                const auto is_cs = has_filename && fn_str.ends_with("_CS");
                const auto has_valid_staged_filename = is_vs || is_hs || is_ds || is_gs || is_ps || is_cs;
                GUARANTEE_OR_DIE(has_valid_staged_filename, "Compiled shader source filename must end in '_VS' '_HS' '_DS' '_GS' '_PS' or '_CS'");
                auto buffer = FileUtils::ReadBinaryBufferFromFile(p);
                if(is_vs && buffer.has_value()) {
                    desc.vs_bytecode = CreateD3DBlobFromBuffer(buffer, "VS Blob creation failed.");
                    device.CreateVertexShader(desc);
                    desc.input_layout = RHIDevice::CreateInputLayoutFromByteCode(device, desc.vs_bytecode);
                } else if(is_hs && buffer.has_value()) {
                    desc.hs_bytecode = CreateD3DBlobFromBuffer(buffer, "HS Blob creation failed.");
                    device.CreateHullShader(desc);
                } else if(is_ds && buffer.has_value()) {
                    desc.ds_bytecode = CreateD3DBlobFromBuffer(buffer, "DS Blob creation failed.");
                    device.CreateDomainShader(desc);
                } else if(is_gs && buffer.has_value()) {
                    desc.gs_bytecode = CreateD3DBlobFromBuffer(buffer, "GS Blob creation failed.");
                    device.CreateGeometryShader(desc);
                } else if(is_ps && buffer.has_value()) {
                    desc.ps_bytecode = CreateD3DBlobFromBuffer(buffer, "PS Blob creation failed.");
                    device.CreatePixelShader(desc);
                } else if(is_cs && buffer.has_value()) {
                    desc.cs_bytecode = CreateD3DBlobFromBuffer(buffer, "CS Blob creation failed.");
                    device.CreateComputeShader(desc);
                } else {
                    ERROR_AND_DIE("Could not determine shader type. Filename must end in _VS, _PS, _HS, _DS, _GS, or _CS.");
                }
            });
            auto sp = renderer->CreateShaderProgramFromDesc(std::move(desc));
            auto* sp_ptr = sp.get();
            renderer->RegisterShaderProgram(m_name, std::move(sp));
            m_shader_program = sp_ptr;
        }
    }
    m_cbuffers = std::move(RHIDevice::CreateConstantBuffersFromShaderProgram(*renderer->GetDevice(), m_shader_program));
    m_ccbuffers = std::move(RHIDevice::CreateComputeConstantBuffersFromShaderProgram(*renderer->GetDevice(), m_shader_program));
    m_depth_stencil_state = std::make_unique<DepthStencilState>(renderer->GetDevice(), element);
    m_blend_state = std::make_unique<BlendState>(renderer->GetDevice(), element);

    m_raster_state = renderer->GetRasterState("__default");
    if(auto* xml_raster = element.FirstChildElement("raster")) {
        std::string rs_src = DataUtils::ParseXmlAttribute(*xml_raster, "src", std::string{});
        if(auto* found_raster = renderer->GetRasterState(rs_src)) {
            m_raster_state = found_raster;
        } else {
            CreateAndRegisterNewRasterFromXml(element);
        }
    }

    m_sampler = renderer->GetSampler("__default");
    if(auto* xml_sampler = element.FirstChildElement("sampler")) {
        std::string s_src = DataUtils::ParseXmlAttribute(*xml_sampler, "src", std::string{});
        if(auto* found_sampler = renderer->GetSampler(s_src)) {
            m_sampler = found_sampler;
        } else {
            CreateAndRegisterNewSamplerFromXml(element);
        }
    }
    return true;
}

PipelineStage Shader::ParseTargets(const XMLElement& element) noexcept {
    auto targets = PipelineStage::None;
    if(DataUtils::HasChild(element, "vertex")) {
        targets |= PipelineStage::Vs;
    }
    if(DataUtils::HasChild(element, "hull")) {
        targets |= PipelineStage::Hs;
    }
    if(DataUtils::HasChild(element, "domain")) {
        targets |= PipelineStage::Ds;
    }
    if(DataUtils::HasChild(element, "geometry")) {
        targets |= PipelineStage::Gs;
    }
    if(DataUtils::HasChild(element, "pixel")) {
        targets |= PipelineStage::Ps;
    }
    if(DataUtils::HasChild(element, "compute")) {
        targets |= PipelineStage::Cs;
    }
    ValidatePipelineStages(targets);
    return targets;
}

std::string Shader::ParseEntrypointList(const XMLElement& element) noexcept {
    std::string entrypointList{};
    if(auto* xml_vertex = element.FirstChildElement("vertex")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_vertex, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    if(auto* xml_hull = element.FirstChildElement("hull")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_hull, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    if(auto* xml_domain = element.FirstChildElement("domain")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_domain, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    if(auto* xml_geometry = element.FirstChildElement("geometry")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_geometry, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    if(auto* xml_pixel = element.FirstChildElement("pixel")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_pixel, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    if(auto* xml_compute = element.FirstChildElement("compute")) {
        auto entrypoint = DataUtils::ParseXmlAttribute(*xml_compute, "entrypoint", std::string{});
        entrypoint += ",";
        entrypointList += entrypoint;
    } else {
        entrypointList += ",";
    }
    return entrypointList;
}

void Shader::ValidatePipelineStages(const PipelineStage& targets) noexcept {
    bool result = false;
    if(targets == PipelineStage::None) {
        result = false;
    } else if(targets == PipelineStage::All) {
        result = true;
    } else {
        bool has_vs = (targets & PipelineStage::Vs) == PipelineStage::Vs;
        bool has_ps = (targets & PipelineStage::Ps) == PipelineStage::Ps;
        bool has_hs = (targets & PipelineStage::Hs) == PipelineStage::Hs;
        bool has_ds = (targets & PipelineStage::Ds) == PipelineStage::Ds;
        bool has_cs = (targets & PipelineStage::Cs) == PipelineStage::Cs;
        bool has_gs = (targets & PipelineStage::Gs) == PipelineStage::Gs;
        bool valid_vsps = !(has_vs ^ has_ps);
        bool valid_hsds = !(has_hs ^ has_ds);
        bool valid_cs = has_cs;
        bool valid_gs = has_gs;
        result = valid_cs || valid_gs || valid_vsps || valid_hsds;
    }
    const auto error_msg = std::format("Error in shader file: \"{}\": Pipeline stages must include at least compute stage, geometry stage, or both vertex and pixel stages, or both hull and domain stages.", m_name);
    GUARANTEE_OR_DIE(result, error_msg.c_str());
}

void Shader::CreateAndRegisterNewSamplerFromXml(const XMLElement& element) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    auto new_sampler = std::make_unique<Sampler>(renderer->GetDevice(), element);
    std::string ns = m_name + "_sampler";
    m_sampler = new_sampler.get();
    renderer->RegisterSampler(ns, std::move(new_sampler));
}

void Shader::CreateAndRegisterNewRasterFromXml(const XMLElement& element) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    auto new_raster_state = std::make_unique<RasterState>(renderer->GetDevice(), element);
    std::string nr = m_name + "_raster";
    m_raster_state = new_raster_state.get();
    renderer->RegisterRasterState(nr, std::move(new_raster_state));
}
