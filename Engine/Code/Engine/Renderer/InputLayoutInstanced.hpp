#pragma once

#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

#include <vector>

class RHIDevice;

class InputLayoutInstanced {
public:
    explicit InputLayoutInstanced(const RHIDevice& parent_device) noexcept;
    ~InputLayoutInstanced() = default;

    void AddElement(std::size_t memberByteOffset, const ImageFormat& format, const char* semantic, unsigned int inputSlot = 0, bool isVertexData = false, unsigned int instanceDataStepRate = 0) noexcept;
    void AddElement(const D3D11_INPUT_ELEMENT_DESC& desc) noexcept;
    void CreateInputLayout(void* byte_code, std::size_t byte_code_length) noexcept;
    [[nodiscard]] ID3D11InputLayout* GetDxInputLayout() const noexcept;
    void PopulateInputLayoutUsingReflection(ID3D11ShaderReflection& vertexReflection) noexcept;

protected:
private:
    [[nodiscard]] D3D11_INPUT_ELEMENT_DESC CreateInputElementFromSignature(D3D11_SIGNATURE_PARAMETER_DESC& input_desc, unsigned int& last_input_slot) noexcept;

    std::vector<D3D11_INPUT_ELEMENT_DESC> _elements{};
    Microsoft::WRL::ComPtr<ID3D11InputLayout> _dx_input_layout{};
    const RHIDevice& _parent_device;
};