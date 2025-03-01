#include "Engine/Core/WebM.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/RHI/RHIDevice.hpp"

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Texture2D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IAudioService.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include <fstream>
#include <concepts>
#include <format>

#include <Thirdparty/webm/webm_parser/include/webm/webm_parser.h>
#include <Thirdparty/webm/webm_parser/include/webm/reader.h>
#include <Thirdparty/webm/webm_parser/include/webm/buffer_reader.h>
#include <Thirdparty/webm/webm_parser/include/webm/file_reader.h>
#include <Thirdparty/webm/webm_parser/include/webm/istream_reader.h>
#include <Thirdparty/webm/webm_parser/include/webm/callback.h>


class MyWebMCallback : public webm::Callback {
public:
    MyWebMCallback() = default;
    explicit MyWebMCallback(FileUtils::WebM* parent_webm)
    : m_parent_webm(parent_webm)
    {
        /* DO NOTHING */
    }
    virtual ~MyWebMCallback() = default;

    webm::Status OnEbml(const webm::ElementMetadata& metadata, const webm::Ebml& ebml) override {
        if(metadata.id == webm::Id::kDocType) {
            if(ebml.doc_type.value() != "webm") {
                if(auto* filelogger = ServiceLocator::get<IFileLoggerService>(); filelogger != nullptr) {
                    filelogger->LogErrorLine(std::format("File: {} is not a webm file.", m_parent_webm->GetFilepath()));
                    filelogger->Flush();
                }
                return webm::Status(webm::Status::kInvalidElementValue);
            }
        }
        return webm::Status(webm::Status::kOkCompleted);
    }
    webm::Status OnInfo(const webm::ElementMetadata& metadata, const webm::Info& info) override {
        m_parent_webm->SetDuration(TimeUtils::FPMilliseconds{info.duration.value()});
        return webm::Callback::OnInfo(metadata, info);
    }
    webm::Status OnFrame(const webm::FrameMetadata& metadata, webm::Reader* reader, std::uint64_t* bytes_remaining) override {
        const auto frame_size = metadata.size;
        std::vector<std::uint8_t> encoded_buffer{};
        encoded_buffer.resize(frame_size);
        std::uint64_t actually_read{0u};
        webm::Status status{};
        std::size_t offset{actually_read};
        do {
            status = reader->Read(encoded_buffer.size(), encoded_buffer.data() + offset, &actually_read);
            offset += actually_read;
            *bytes_remaining -= actually_read;
        } while(status.code == webm::Status::kOkPartial);
        m_parent_webm->BindEncodedBufferToGpu(encoded_buffer);
        m_parent_webm->AddFrame();
        return webm::Status{webm::Status::Code::kOkCompleted};
    }
    webm::Status OnTrackEntry(const webm::ElementMetadata& metadata, const webm::TrackEntry& track_entry) override {
        if(const auto& ces_elem = track_entry.content_encodings; ces_elem.is_present()) {
            const auto& ces = ces_elem.value();
            for(auto& encoding_elem : ces.encodings) {
                if(const auto& encoding = encoding_elem.value(); encoding_elem.is_present()) {
                    if(const webm::ContentEncodingType& type = encoding.type.value(); encoding.type.is_present()) {
                        auto* l = ServiceLocator::get<IFileLoggerService>();
                        switch(type) {
                        case webm::ContentEncodingType::kCompression:
                        {
                            l->LogAndFlush(std::format("WebM file {} is compressed. Compression is not yet supported\n", m_parent_webm->GetFilepath(), m_parent_webm->GetDimensions().first, m_parent_webm->GetDimensions().second));
                            return webm::Status{webm::Status::Code::kInvalidElementValue};
                        }
                        case webm::ContentEncodingType::kEncryption:
                        {
                            l->LogAndFlush(std::format("WebM file {} is encrypted. Encryption is not supported.\n", m_parent_webm->GetFilepath(), m_parent_webm->GetDimensions().first, m_parent_webm->GetDimensions().second));
                            return webm::Status{webm::Status::Code::kInvalidElementValue};
                        }
                        default:
                        {
                            l->LogAndFlush(std::format("WebM file {} neither compressed nor encrypted.\n", m_parent_webm->GetFilepath(), m_parent_webm->GetDimensions().first, m_parent_webm->GetDimensions().second));
                            break;
                        }
                        }
                    }
                }
            }
        }
        if(const auto& video = track_entry.video; video.is_present()) {
            uint64_t pixel_width{0u};
            if(const auto& width = video.value().pixel_width; width.is_present()) {
                pixel_width = width.value();
            }
            uint64_t pixel_height{0u};
            if(const auto& height = video.value().pixel_height; height.is_present()) {
                pixel_height = height.value();
            }
            m_parent_webm->SetPixelDimensions(pixel_width, pixel_height);
            return webm::Status(webm::Status::kOkCompleted);
        }
        if(const auto& audio = track_entry.audio; audio.is_present()) {
            //Load audio for track.
            const auto sampleHz = audio.value().sampling_frequency.value();
            const auto bit_depth = audio.value().bit_depth.value();
            const auto channels = audio.value().channels.value();
            const auto output_sampleHz = audio.value().output_frequency.value();
            auto* audio_service = ServiceLocator::get<IAudioService>();
            if(auto* audio_system = dynamic_cast<AudioSystem*>(audio_service); audio_system != nullptr) {
                FileUtils::detail::WavFormatChunk format{};
                format.formatId = 1;
                format.bitsPerSample = static_cast<decltype(format.bitsPerSample)>(bit_depth);
                format.channelCount = static_cast<decltype(format.channelCount)>(channels);
                format.samplesPerSecond = static_cast<decltype(format.samplesPerSecond)>(sampleHz);
                format.bytesPerSecond = format.samplesPerSecond * format.channelCount * format.bitsPerSample / 8u;
                format.dataBlockSize = format.channelCount * format.bitsPerSample / 8u;
                GUARANTEE_OR_DIE(output_sampleHz == sampleHz, "audio output sample does not match file bit sample.");
                audio_system->SetFormat(format);
                //audio_system->Play();
                
            }
            return webm::Status(webm::Status::kOkCompleted);
        }
        return webm::Callback::OnTrackEntry(metadata, track_entry);
    }
protected:
private:
    FileUtils::WebM* m_parent_webm{nullptr};
};

namespace FileUtils {

WebM::WebM(std::filesystem::path filepath) noexcept {
    const auto err_str = std::format("Failed to load WebM file: {}", filepath);
    GUARANTEE_OR_DIE(Load(filepath), err_str.c_str());
}

bool WebM::Load(std::filesystem::path filepath) noexcept {
    if(auto buffer = FileUtils::ReadBinaryBufferFromFile(filepath); buffer.has_value()) {
        m_path = filepath;
        webm::BufferReader reader{buffer.value()};
        auto callback = MyWebMCallback(this);
        webm::WebmParser parser{};
        webm::Status status{};
        //TODO: Start Here
        do {
            status = parser.Feed(&callback, &reader);
            if(status.is_parsing_error()) {
                return false;
            }
            if(status.completed_ok()) {
                return true;
            }
        } while(status.code == webm::Status::kOkPartial);
    }
    return false;
}

std::filesystem::path WebM::GetFilepath() const noexcept {
    return m_path;
}

void WebM::SetPixelDimensions(uint64_t width, uint64_t height) noexcept {
    GUARANTEE_OR_DIE(((width & uint64_t{1u}) == uint64_t{0u}) && ((height & uint64_t{1u}) == uint64_t{0u}), "Width and Height of WebM dimensions must be even.");
    m_width = width;
    m_height = height;
    auto* r = ServiceLocator::get<IRendererService>();
    std::vector<Rgba> data(m_width * m_height, Rgba::Magenta);
    const auto widthAsUInt = static_cast<unsigned int>(m_width);
    const auto heightAsUInt = static_cast<unsigned int>(m_height);

    D3D11_TEXTURE2D_DESC tex_desc{};

    tex_desc.Width = widthAsUInt;
    tex_desc.Height = heightAsUInt;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    const auto bindUsage = BufferBindUsage::Decoder | BufferBindUsage::Shader_Resource;
    const auto bufferUsage = BufferUsage::Dynamic;
    tex_desc.Usage = BufferUsageToD3DUsage(bufferUsage);
    tex_desc.Format = ImageFormatToDxgiFormat(ImageFormat::Nv12);
    tex_desc.BindFlags = BufferBindUsageToD3DBindFlags(bindUsage);
    tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(bufferUsage);
    //Force specific usages for unordered access
    if(!!(bindUsage & BufferBindUsage::Unordered_Access)) {
        tex_desc.Usage = BufferUsageToD3DUsage(BufferUsage::Gpu);
        tex_desc.CPUAccessFlags = CPUAccessFlagFromUsage(BufferUsage::Staging);
    }
    //Staging textures can't be bound to the graphics pipeline.
    if(!!(bufferUsage & BufferUsage::Staging)) {
        tex_desc.BindFlags = 0;
    }
    tex_desc.MiscFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;

    // Setup Initial Data
    D3D11_SUBRESOURCE_DATA subresource_data = {};

    subresource_data.pSysMem = data.data();
    subresource_data.SysMemPitch = widthAsUInt * sizeof(Rgba);
    subresource_data.SysMemSlicePitch = static_cast<unsigned long long>(widthAsUInt) * static_cast<unsigned long long>(heightAsUInt) * sizeof(Rgba);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> dx_tex{};

    //If IMMUTABLE or not multi-sampled, must use initial data.
    bool isMultiSampled = tex_desc.SampleDesc.Count != 1 || tex_desc.SampleDesc.Quality != 0;
    bool isImmutable = !!(bufferUsage & BufferUsage::Static);
    bool mustUseInitialData = isImmutable || !isMultiSampled;

    auto* device = r->GetDevice();
    auto* dx_device = device->GetDxDevice();
    {
        HRESULT hr_create = dx_device->CreateTexture2D(&tex_desc, (mustUseInitialData ? &subresource_data : nullptr), &dx_tex);
        const auto err_msg = std::format("Failed to create output texture for WebM file. Requested dimensions: {}x{}", width, height);
        GUARANTEE_OR_DIE(SUCCEEDED(hr_create), err_msg.c_str());
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvY_desc{};
    srvY_desc.Format = DXGI_FORMAT_R8_UNORM;
    srvY_desc.Texture2D.MipLevels = 1;
    srvY_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    {
        auto hr_srvY = dx_device->CreateShaderResourceView(dx_tex.Get(), &srvY_desc, &m_srvY);
        const auto err_msg = std::format("Failed to create Y-plane shader resource for WebM file.");
        GUARANTEE_OR_DIE(SUCCEEDED(hr_srvY), err_msg.c_str());
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvUV_desc{};
    srvUV_desc.Format = DXGI_FORMAT_R8G8_UNORM;
    srvUV_desc.Texture2D.MipLevels = 1;
    srvUV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    {
        auto hr_srvUV = dx_device->CreateShaderResourceView(dx_tex.Get(), &srvUV_desc, &m_srvUV);
        const auto err_msg = std::format("Failed to create UV-plane shader resource for WebM file.");
        GUARANTEE_OR_DIE(SUCCEEDED(hr_srvUV), err_msg.c_str());
    }

    //Success beyond this point.
    dx_tex.Swap(m_decodedFrame);

}

void WebM::SetDuration(TimeUtils::FPSeconds newDuration) noexcept {
    m_length = newDuration;
}

void WebM::AddFrame() noexcept {
    ++m_frameCount;
}

void WebM::BindEncodedBufferToGpu(const std::vector<uint8_t>& encodedFrame) noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    if(auto* renderer = dynamic_cast<Renderer*>(r); renderer != nullptr) {
        auto* dc = renderer->GetDeviceContext();
        auto* dx_dc = dc->GetDxContext();
        auto* dx_resource = m_decodedFrame.Get();
        D3D11_MAPPED_SUBRESOURCE mapped_resource{};
        if(auto hr = dx_dc->Map(dx_resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource); SUCCEEDED(hr)) {
            D3D11_TEXTURE2D_DESC desc{};
            m_decodedFrame->GetDesc(&desc);
            const auto width = desc.Width;
            const auto height = desc.Height;
            const auto* src = encodedFrame.data();
            const auto* dst = (unsigned int*)mapped_resource.pData;
            const auto byte_width = width * sizeof(Rgba);

            //Width and height must be even.
            //Direct3D 11 staging resources and initData parameters for this format use
            //(rowPitch * (height + (height / 2))) bytes.
            //The first (SysMemPitch * height) bytes are the Y plane,
            //the remaining (SysMemPitch * (height / 2)) bytes are the UV plane.

            if(mapped_resource.RowPitch == byte_width) {
                std::memcpy(mapped_resource.pData, src, encodedFrame.size() * sizeof(Rgba));
                dx_dc->Unmap(dx_resource, 0);
            } else {
                for(unsigned int i = 0u; i < height; i++) {
                    std::memcpy(mapped_resource.pData, src, width); // copy one row at a time because msr.RowPitch may be != (width * 4)
                    dst += mapped_resource.RowPitch >> 2;    // msr.RowPitch is in bytes so for 32-bit data we divide by 4 (or downshift by 2, same thing)
                    src += width;                // assumes pitch of source data is equal to width * 4
                }
                dx_dc->Unmap(dx_resource, 0);
            }
        }
    }
    

    //TODO: Decode webM data on GPU
    //auto* r = ServiceLocator::get<IRendererService>();
    //auto* m = r->GetMaterial("__webm");
    //r->SetConstantBuffer();
    //r->SetModelMatrix();
    //m->SetTextureSlot(Material::TextureID::Diffuse, t);
    //const auto parent = m_path.parent_path();
    //const auto p = parent / std::filesystem::path{std::format("{}_Frame{}.bin", m_path.filename(), m_frameCount)};
    //if(FileUtils::WriteBufferToFile(const_cast<uint8_t*>(encodedFrame.data()), encodedFrame.size(), p)) {
    //    auto* l = ServiceLocator::get<IFileLoggerService>();
    //    l->LogAndFlush(std::format("Wrote {} bytes to: {}\n", encodedFrame.size(), p));
    //}
}

void WebM::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {

}

void WebM::Render([[maybe_unused]] const Matrix4& transform /*= Matrix4::I*/) const noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    r->SetMaterial("__2D");
    r->SetTexture(m_currentFrame.get());
    r->DrawQuad2D(transform);
}

auto WebM::GetDimensions() const noexcept -> const std::pair<uint64_t, uint64_t> {
    return {m_width, m_height};
}

} // namespace FileUtils
