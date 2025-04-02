#include "Engine/Renderer/InputLayoutInstanced.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

InputLayoutInstanced::InputLayoutInstanced(const RHIDevice& parent_device) noexcept
: m_parent_device(parent_device) {
    /* DO NOTHING */
}

void InputLayoutInstanced::AddElement(std::size_t memberByteOffset, const ImageFormat& format, const char* semantic, unsigned int inputSlot /*= 0*/, bool isVertexData /*= true*/, unsigned int instanceDataStepRate /*= 0*/) noexcept {
    D3D11_INPUT_ELEMENT_DESC e_desc{};
    e_desc.Format = ImageFormatToDxgiFormat(format);
    e_desc.InputSlotClass = isVertexData ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;
    e_desc.InstanceDataStepRate = isVertexData ? 0 : instanceDataStepRate;
    e_desc.SemanticName = semantic;
    e_desc.SemanticIndex = 0;
    e_desc.InputSlot = inputSlot;
    e_desc.AlignedByteOffset = static_cast<unsigned int>(memberByteOffset);
    m_elements.push_back(e_desc);
}

void InputLayoutInstanced::AddElement(const D3D11_INPUT_ELEMENT_DESC& desc) noexcept {
    m_elements.push_back(desc);
}

void InputLayoutInstanced::CreateInputLayout(void* byte_code, std::size_t byte_code_length) noexcept {
    m_elements.shrink_to_fit();
    if(m_dx_input_layout) {
        m_dx_input_layout->Release();
        m_dx_input_layout = nullptr;
    }
    auto* dx_device = m_parent_device.GetDxDevice();
    HRESULT hr = dx_device->CreateInputLayout(m_elements.data(), static_cast<unsigned int>(m_elements.size()), byte_code, byte_code_length, m_dx_input_layout.GetAddressOf());
    bool succeeded = SUCCEEDED(hr);
    GUARANTEE_OR_DIE(succeeded, "Create Input Layout failed.");
}

ID3D11InputLayout* InputLayoutInstanced::GetDxInputLayout() const noexcept {
    return m_dx_input_layout.Get();
}

void InputLayoutInstanced::PopulateInputLayoutUsingReflection(ID3D11ShaderReflection& vertexReflection) noexcept {
    D3D11_SHADER_DESC desc{};
    vertexReflection.GetDesc(&desc);
    unsigned int input_count = desc.InputParameters;
    unsigned int last_input_slot = 16;
    for(auto i = 0u; i < input_count; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC input_desc{};
        vertexReflection.GetInputParameterDesc(i, &input_desc);
        AddElement(CreateInputElementFromSignature(input_desc, last_input_slot));
    }
}

D3D11_INPUT_ELEMENT_DESC InputLayoutInstanced::CreateInputElementFromSignature(D3D11_SIGNATURE_PARAMETER_DESC& input_desc, unsigned int& last_input_slot) noexcept {
    D3D11_INPUT_ELEMENT_DESC elem{};
    elem.InputSlot = 1;
    elem.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
    elem.InstanceDataStepRate = 0;

    elem.SemanticName = input_desc.SemanticName;
    elem.SemanticIndex = input_desc.SemanticIndex;
    elem.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    last_input_slot = elem.InputSlot;

    constexpr auto r_mask = D3D_COMPONENT_MASK_X;
    constexpr auto rg_mask = D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y;
    constexpr auto rgb_mask = D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z;
    constexpr auto rgba_mask = D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z | D3D_COMPONENT_MASK_W;
    constexpr auto one_channel_uint = ImageFormatToDxgiFormat(ImageFormat::R32_UInt);
    constexpr auto one_channel_sint = ImageFormatToDxgiFormat(ImageFormat::R32_SInt);
    constexpr auto one_channel_float = ImageFormatToDxgiFormat(ImageFormat::R32_Float);
    constexpr auto two_channel_uint = ImageFormatToDxgiFormat(ImageFormat::R32G32_UInt);
    constexpr auto two_channel_sint = ImageFormatToDxgiFormat(ImageFormat::R32G32_SInt);
    constexpr auto two_channel_float = ImageFormatToDxgiFormat(ImageFormat::R32G32_Float);
    constexpr auto three_channel_uint = ImageFormatToDxgiFormat(ImageFormat::R32G32B32_UInt);
    constexpr auto three_channel_sint = ImageFormatToDxgiFormat(ImageFormat::R32G32B32_SInt);
    constexpr auto three_channel_float = ImageFormatToDxgiFormat(ImageFormat::R32G32B32_Float);
    constexpr auto four_channel_uint = ImageFormatToDxgiFormat(ImageFormat::R32G32B32A32_UInt);
    constexpr auto four_channel_sint = ImageFormatToDxgiFormat(ImageFormat::R32G32B32A32_SInt);
    constexpr auto four_channel_float = ImageFormatToDxgiFormat(ImageFormat::R32G32B32A32_Float);
    if(input_desc.Mask == r_mask) {
        switch(input_desc.ComponentType) {
        case D3D_REGISTER_COMPONENT_UINT32: elem.Format = one_channel_uint; break;
        case D3D_REGISTER_COMPONENT_SINT32: elem.Format = one_channel_sint; break;
        case D3D_REGISTER_COMPONENT_FLOAT32: elem.Format = one_channel_float; break;
        }
    } else if(input_desc.Mask <= rg_mask) {
        switch(input_desc.ComponentType) {
        case D3D_REGISTER_COMPONENT_UINT32: elem.Format = two_channel_uint; break;
        case D3D_REGISTER_COMPONENT_SINT32: elem.Format = two_channel_sint; break;
        case D3D_REGISTER_COMPONENT_FLOAT32: elem.Format = two_channel_float; break;
        }
    } else if(input_desc.Mask <= rgb_mask) {
        switch(input_desc.ComponentType) {
        case D3D_REGISTER_COMPONENT_UINT32: elem.Format = three_channel_uint; break;
        case D3D_REGISTER_COMPONENT_SINT32: elem.Format = three_channel_sint; break;
        case D3D_REGISTER_COMPONENT_FLOAT32: elem.Format = three_channel_float; break;
        }
    } else if(input_desc.Mask <= rgba_mask) {
        switch(input_desc.ComponentType) {
        case D3D_REGISTER_COMPONENT_UINT32: elem.Format = four_channel_uint; break;
        case D3D_REGISTER_COMPONENT_SINT32: elem.Format = four_channel_sint; break;
        case D3D_REGISTER_COMPONENT_FLOAT32: elem.Format = four_channel_float; break;
        }
    }
    return elem;
}
