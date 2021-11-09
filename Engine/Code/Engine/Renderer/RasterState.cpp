#include "Engine/Renderer/RasterState.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

#include <algorithm>
#include <locale>

void RasterState::SetDebugName([[maybe_unused]] const std::string& name) const noexcept {
#ifdef RENDER_DEBUG
    _dx_state->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned int>(name.size()), name.data());
#endif
}

RasterState::RasterState(const RHIDevice* device, const XMLElement& element) noexcept
: RasterState(device, RasterDesc{element}) {
    /* DO NOTHING */
}

RasterState::RasterState(const RHIDevice* device, const RasterDesc& desc) noexcept
: _desc(desc) {
    if(!CreateRasterState(device, _desc)) {
        if(_dx_state) {
            _dx_state->Release();
            _dx_state = nullptr;
        }
        ERROR_AND_DIE("RasterState: dx Rasterizer failed to create.\n");
    }
}

RasterState::~RasterState() noexcept {
    if(_dx_state) {
        _dx_state->Release();
        _dx_state = nullptr;
    }
}

const RasterDesc& RasterState::GetDesc() const noexcept {
    return _desc;
}

ID3D11RasterizerState* RasterState::GetDxRasterState() noexcept {
    return _dx_state;
}

bool RasterState::CreateRasterState(const RHIDevice* device, const RasterDesc& raster_desc /*= RasterDesc()*/) noexcept {
    D3D11_RASTERIZER_DESC desc{};

    desc.FillMode = FillModeToD3DFillMode(raster_desc.fillmode);
    desc.CullMode = CullModeToD3DCullMode(raster_desc.cullmode);
    desc.FrontCounterClockwise = raster_desc.frontCounterClockwise;
    desc.AntialiasedLineEnable = raster_desc.antialiasedLineEnable;
    desc.DepthBias = raster_desc.depthBias;
    desc.DepthBiasClamp = raster_desc.depthBiasClamp;
    desc.SlopeScaledDepthBias = raster_desc.slopeScaledDepthBias;
    desc.DepthClipEnable = raster_desc.depthClipEnable;
    desc.ScissorEnable = raster_desc.scissorEnable;

    HRESULT hr = device->GetDxDevice()->CreateRasterizerState(&desc, &_dx_state);
    return SUCCEEDED(hr);
}

RasterDesc::RasterDesc(const XMLElement& element) noexcept {
    if(auto* xml_raster = element.FirstChildElement("raster")) {
        DataUtils::ValidateXmlElement(*xml_raster, "raster", "fill,cull", "", "windingorder,antialiasing,depthbias,depthclip,scissor,msaa");
        const auto* xml_fill = xml_raster->FirstChildElement("fill");
        std::string fill_str = "solid";
        fill_str = DataUtils::ParseXmlElementText(*xml_fill, fill_str);
        fillmode = FillModeFromString(fill_str);

        const auto* xml_cull = xml_raster->FirstChildElement("cull");
        std::string cull_str = "back";
        cull_str = DataUtils::ParseXmlElementText(*xml_cull, cull_str);
        cullmode = CullModeFromString(cull_str);

        antialiasedLineEnable = false;
        if(const auto* xml_antialiasing = xml_raster->FirstChildElement("antialiasing")) {
            DataUtils::ValidateXmlElement(*xml_antialiasing, "antialiasing", "", "");
            antialiasedLineEnable = DataUtils::ParseXmlElementText(*xml_antialiasing, antialiasedLineEnable);
        }

        depthBias = 0;
        depthBiasClamp = 0.0f;
        slopeScaledDepthBias = 0.0f;
        if(const auto* xml_depthbias = xml_raster->FirstChildElement("depthbias")) {
            DataUtils::ValidateXmlElement(*xml_depthbias, "depthbias", "", "value,clamp,slopescaled");
            depthBias = DataUtils::ParseXmlAttribute(*xml_depthbias, "value", depthBias);
            depthBiasClamp = DataUtils::ParseXmlAttribute(*xml_depthbias, "clamp", depthBiasClamp);
            slopeScaledDepthBias = DataUtils::ParseXmlAttribute(*xml_depthbias, "slopescaled", slopeScaledDepthBias);
        }

        depthClipEnable = true;
        if(const auto* xml_depthclip = xml_raster->FirstChildElement("depthclip")) {
            DataUtils::ValidateXmlElement(*xml_depthclip, "depthclip", "", "");
            depthClipEnable = DataUtils::ParseXmlElementText(*xml_depthclip, depthClipEnable);
        }

        scissorEnable = true;
        if(const auto* xml_scissor = xml_raster->FirstChildElement("scissor")) {
            DataUtils::ValidateXmlElement(*xml_scissor, "scissor", "", "");
            scissorEnable = DataUtils::ParseXmlElementText(*xml_scissor, scissorEnable);
        }

        multisampleEnable = false;
        if(const auto* xml_msaa = xml_raster->FirstChildElement("msaa")) {
            DataUtils::ValidateXmlElement(*xml_msaa, "msaa", "", "");
            multisampleEnable = DataUtils::ParseXmlElementText(*xml_msaa, multisampleEnable);
        }

        frontCounterClockwise = false;
        if(const auto* xml_windingorder = xml_raster->FirstChildElement("windingorder")) {
            DataUtils::ValidateXmlElement(*xml_windingorder, "windingorder", "", "");
            std::string value{"cw"};
            value = DataUtils::ParseXmlElementText(*xml_raster, value);
            auto windingOrder = WindingOrderFromString(value);
            switch(windingOrder) {
            case WindingOrder::CCW:
                frontCounterClockwise = true;
                break;
            case WindingOrder::CW:
                frontCounterClockwise = false;
                break;
            default:
                frontCounterClockwise = false;
                break;
            }
        }
    }
}
