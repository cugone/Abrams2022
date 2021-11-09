#include "Engine/Renderer/ShaderProgram.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/InputLayout.hpp"
#include "Engine/Renderer/InputLayoutInstanced.hpp"

ShaderProgram::ShaderProgram(ShaderProgramDesc&& desc) noexcept
: _desc(std::move(desc)) {
    /* DO NOTHING */
}

ShaderProgramDesc&& ShaderProgram::GetDescription() noexcept {
    return std::move(_desc);
}

void ShaderProgram::SetDescription(ShaderProgramDesc&& description) noexcept {
    _desc = std::move(description);
}

const std::string& ShaderProgram::GetName() const noexcept {
    return _desc.name;
}

ID3DBlob* ShaderProgram::GetVSByteCode() const noexcept {
    return _desc.vs_bytecode;
}

ID3DBlob* ShaderProgram::GetHSByteCode() const noexcept {
    return _desc.hs_bytecode;
}

ID3DBlob* ShaderProgram::GetDSByteCode() const noexcept {
    return _desc.ds_bytecode;
}

ID3DBlob* ShaderProgram::GetGSByteCode() const noexcept {
    return _desc.gs_bytecode;
}

ID3DBlob* ShaderProgram::GetPSByteCode() const noexcept {
    return _desc.ps_bytecode;
}

ID3DBlob* ShaderProgram::GetCSByteCode() const noexcept {
    return _desc.cs_bytecode;
}

InputLayout* ShaderProgram::GetInputLayout() const noexcept {
    return _desc.input_layout.get();
}

InputLayoutInstanced* ShaderProgram::GetInputLayoutInstanced() const noexcept {
    return _desc.input_layout_instanced.get();
}

ID3D11VertexShader* ShaderProgram::GetVS() const noexcept {
    return _desc.vs;
}

bool ShaderProgram::HasVS() const noexcept {
    return GetVS() != nullptr;
}

ID3D11HullShader* ShaderProgram::GetHS() const noexcept {
    return _desc.hs;
}

bool ShaderProgram::HasHS() const noexcept {
    return GetHS() != nullptr;
}

ID3D11DomainShader* ShaderProgram::GetDS() const noexcept {
    return _desc.ds;
}

bool ShaderProgram::HasDS() const noexcept {
    return GetDS() != nullptr;
}

ID3D11GeometryShader* ShaderProgram::GetGS() const noexcept {
    return _desc.gs;
}

bool ShaderProgram::HasGS() const noexcept {
    return GetGS() != nullptr;
}

ID3D11PixelShader* ShaderProgram::GetPS() const noexcept {
    return _desc.ps;
}

bool ShaderProgram::HasPS() const noexcept {
    return GetPS() != nullptr;
}

ID3D11ComputeShader* ShaderProgram::GetCS() const noexcept {
    return _desc.cs;
}

bool ShaderProgram::HasCS() const noexcept {
    return GetCS() != nullptr;
}

ShaderProgramDesc::ShaderProgramDesc(ShaderProgramDesc&& other) noexcept {
    name = std::move(other.name);
    other.name = std::string{};

    input_layout = std::move(other.input_layout);
    other.input_layout = nullptr;

    input_layout_instanced = std::move(other.input_layout_instanced);
    other.input_layout_instanced = nullptr;

    vs = other.vs;
    vs_bytecode = other.vs_bytecode;
    other.vs = nullptr;
    other.vs_bytecode = nullptr;

    ps = other.ps;
    ps_bytecode = other.ps_bytecode;
    other.ps = nullptr;
    other.ps_bytecode = nullptr;

    hs = other.hs;
    hs_bytecode = other.hs_bytecode;
    other.hs = nullptr;
    other.hs_bytecode = nullptr;

    ds = other.ds;
    ds_bytecode = other.ds_bytecode;
    other.ds = nullptr;
    other.ds_bytecode = nullptr;

    gs = other.gs;
    gs_bytecode = other.gs_bytecode;
    other.gs = nullptr;
    other.gs_bytecode = nullptr;

    cs = other.cs;
    cs_bytecode = other.cs_bytecode;
    other.cs = nullptr;
    other.cs_bytecode = nullptr;
}

ShaderProgramDesc& ShaderProgramDesc::operator=(ShaderProgramDesc&& other) noexcept {
    name = std::move(other.name);
    other.name = std::string{};

    input_layout = std::move(other.input_layout);
    other.input_layout = nullptr;

    input_layout_instanced = std::move(other.input_layout_instanced);
    other.input_layout_instanced = nullptr;

    vs = other.vs;
    vs_bytecode = other.vs_bytecode;
    other.vs = nullptr;
    other.vs_bytecode = nullptr;

    ps = other.ps;
    ps_bytecode = other.ps_bytecode;
    other.ps = nullptr;
    other.ps_bytecode = nullptr;

    hs = other.hs;
    hs_bytecode = other.hs_bytecode;
    other.hs = nullptr;
    other.hs_bytecode = nullptr;

    ds = other.ds;
    ds_bytecode = other.ds_bytecode;
    other.ds = nullptr;
    other.ds_bytecode = nullptr;

    gs = other.gs;
    gs_bytecode = other.gs_bytecode;
    other.gs = nullptr;
    other.gs_bytecode = nullptr;

    cs = other.cs;
    cs_bytecode = other.cs_bytecode;
    other.cs = nullptr;
    other.cs_bytecode = nullptr;

    return *this;
}

ShaderProgramDesc::~ShaderProgramDesc() noexcept {
    name = std::string{};
    if(vs_bytecode) {
        vs_bytecode->Release();
        vs_bytecode = nullptr;
    }
    if(hs_bytecode) {
        hs_bytecode->Release();
        hs_bytecode = nullptr;
    }
    if(ds_bytecode) {
        ds_bytecode->Release();
        ds_bytecode = nullptr;
    }
    if(gs_bytecode) {
        gs_bytecode->Release();
        gs_bytecode = nullptr;
    }
    if(ps_bytecode) {
        ps_bytecode->Release();
        ps_bytecode = nullptr;
    }
    if(cs_bytecode) {
        cs_bytecode->Release();
        cs_bytecode = nullptr;
    }

    input_layout.reset();
    input_layout_instanced.reset();

    if(vs) {
        vs->Release();
        vs = nullptr;
    }
    if(hs) {
        hs->Release();
        hs = nullptr;
    }
    if(ds) {
        ds->Release();
        ds = nullptr;
    }
    if(gs) {
        gs->Release();
        gs = nullptr;
    }
    if(ps) {
        ps->Release();
        ps = nullptr;
    }
    if(cs) {
        cs->Release();
        cs = nullptr;
    }
}
