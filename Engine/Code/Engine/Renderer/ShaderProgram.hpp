#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

#include <memory>

class InputLayout;
class InputLayoutInstanced;

struct ShaderProgramDesc {
    std::string name{"UNNAMED SHADER PROGRAM"};
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3DBlob* vs_bytecode = nullptr;
    ID3DBlob* ps_bytecode = nullptr;
    std::unique_ptr<InputLayout> input_layout = nullptr;
    std::unique_ptr<InputLayoutInstanced> input_layout_instanced = nullptr;
    ID3D11HullShader* hs = nullptr;
    ID3DBlob* hs_bytecode = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3DBlob* ds_bytecode = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3DBlob* gs_bytecode = nullptr;
    ID3D11ComputeShader* cs = nullptr;
    ID3DBlob* cs_bytecode = nullptr;
    ShaderProgramDesc() = default;
    ShaderProgramDesc(ShaderProgramDesc&& other) noexcept;
    ShaderProgramDesc& operator=(ShaderProgramDesc&& other) noexcept;
    ShaderProgramDesc(const ShaderProgramDesc& other) = delete;
    ShaderProgramDesc& operator=(const ShaderProgramDesc& other) = delete;
    ~ShaderProgramDesc() noexcept;
};

class ShaderProgram {
public:
    explicit ShaderProgram(ShaderProgramDesc&& desc) noexcept;
    ShaderProgram(ShaderProgram&& other) = default;
    ShaderProgram& operator=(ShaderProgram&& other) = default;
    ShaderProgram(const ShaderProgram& other) = delete;
    ShaderProgram& operator=(const ShaderProgram& other) = delete;
    ~ShaderProgram() = default;

    [[nodiscard]] ShaderProgramDesc&& GetDescription() noexcept;
    void SetDescription(ShaderProgramDesc&& description) noexcept;

    [[nodiscard]] const std::string& GetName() const noexcept;
    [[nodiscard]] ID3DBlob* GetVSByteCode() const noexcept;
    [[nodiscard]] ID3DBlob* GetHSByteCode() const noexcept;
    [[nodiscard]] ID3DBlob* GetDSByteCode() const noexcept;
    [[nodiscard]] ID3DBlob* GetGSByteCode() const noexcept;
    [[nodiscard]] ID3DBlob* GetPSByteCode() const noexcept;
    [[nodiscard]] ID3DBlob* GetCSByteCode() const noexcept;
    [[nodiscard]] InputLayout* GetInputLayout() const noexcept;
    [[nodiscard]] InputLayoutInstanced* GetInputLayoutInstanced() const noexcept;
    [[nodiscard]] ID3D11VertexShader* GetVS() const noexcept;
    [[nodiscard]] bool HasVS() const noexcept;
    [[nodiscard]] ID3D11HullShader* GetHS() const noexcept;
    [[nodiscard]] bool HasHS() const noexcept;
    [[nodiscard]] ID3D11DomainShader* GetDS() const noexcept;
    [[nodiscard]] bool HasDS() const noexcept;
    [[nodiscard]] ID3D11GeometryShader* GetGS() const noexcept;
    [[nodiscard]] bool HasGS() const noexcept;
    [[nodiscard]] ID3D11PixelShader* GetPS() const noexcept;
    [[nodiscard]] bool HasPS() const noexcept;
    [[nodiscard]] ID3D11ComputeShader* GetCS() const noexcept;
    [[nodiscard]] bool HasCS() const noexcept;

protected:
private:
    ShaderProgramDesc _desc{};
};