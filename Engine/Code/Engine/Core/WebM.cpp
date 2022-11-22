#include "Engine/Core/WebM.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

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
    virtual ~MyWebMCallback() = default;

    webm::Status OnElementBegin(const webm::ElementMetadata& metadata, webm::Action* action) override {
        return webm::Callback::OnElementBegin(metadata, action);
    }
    webm::Status OnUnknownElement(const webm::ElementMetadata& metadata, webm::Reader* reader, std::uint64_t* bytes_remaining) override {
        return webm::Callback::OnUnknownElement(metadata, reader, bytes_remaining);
    }
    webm::Status OnEbml(const webm::ElementMetadata& metadata, const webm::Ebml& ebml) override {
        if(metadata.id == webm::Id::kDocType) {
            if(ebml.doc_type.value() != "webm") {
                if(auto* filelogger = ServiceLocator::get<IFileLoggerService>(); filelogger != nullptr) {
                    filelogger->LogErrorLine(std::format("File: {} is not a webm file.", m_parent_webm->GetFilepath().string()));
                    filelogger->Flush();
                }
                return webm::Status(webm::Status::kInvalidElementValue);
            }
        }
        return webm::Callback::OnEbml(metadata, ebml);
    }
    webm::Status OnVoid(const webm::ElementMetadata& metadata, webm::Reader* reader, std::uint64_t* bytes_remaining) override {
        return webm::Callback::OnVoid(metadata, reader, bytes_remaining);
    }
    webm::Status OnSegmentBegin(const webm::ElementMetadata& metadata, webm::Action* action) override {
        return webm::Callback::OnSegmentBegin(metadata, action);
    }
    webm::Status OnSeek(const webm::ElementMetadata& metadata, const webm::Seek& seek) override {
        return webm::Callback::OnSeek(metadata, seek);
    }
    webm::Status OnInfo(const webm::ElementMetadata& metadata, const webm::Info& info) override {
        m_parent_webm->SetDuration(TimeUtils::FPMilliseconds{info.duration.value()});
        return webm::Callback::OnInfo(metadata, info);
    }
    webm::Status OnClusterBegin(const webm::ElementMetadata& metadata, const webm::Cluster& cluster, webm::Action* action) override {
        return webm::Callback::OnClusterBegin(metadata, cluster, action);
    }
    webm::Status OnSimpleBlockBegin(const webm::ElementMetadata& metadata, const webm::SimpleBlock& simple_block, webm::Action* action) override {
        return webm::Callback::OnSimpleBlockBegin(metadata, simple_block, action);
    }
    webm::Status OnSimpleBlockEnd(const webm::ElementMetadata& metadata, const webm::SimpleBlock& simple_block) override {
        return webm::Callback::OnSimpleBlockEnd(metadata, simple_block);
    }
    webm::Status OnBlockGroupBegin(const webm::ElementMetadata& metadata, webm::Action* action) override {
        return webm::Callback::OnBlockGroupBegin(metadata, action);
    }
    webm::Status OnBlockBegin(const webm::ElementMetadata& metadata, const webm::Block& block, webm::Action* action) override {
        return webm::Callback::OnBlockBegin(metadata, block, action);
    }
    webm::Status OnBlockEnd(const webm::ElementMetadata& metadata, const webm::Block& block) override {
        return webm::Callback::OnBlockEnd(metadata, block);
    }
    webm::Status OnBlockGroupEnd(const webm::ElementMetadata& metadata, const webm::BlockGroup& block_group) override {
        return webm::Callback::OnBlockGroupEnd(metadata, block_group);
    }
    webm::Status OnFrame(const webm::FrameMetadata& metadata, webm::Reader* reader, std::uint64_t* bytes_remaining) override {
        const auto frame_size = metadata.size;
        std::vector<std::uint8_t> buffer{};
        buffer.resize(frame_size);
        std::size_t actually_read{0u};
        if(const auto status = reader->Read(buffer.size(), buffer.data(), &actually_read); status.completed_ok()) {
            return webm::Status(webm::Status::kOkCompleted);
        } else {
            return webm::Callback::OnFrame(metadata, reader, bytes_remaining);
        }
    }
    webm::Status OnClusterEnd(const webm::ElementMetadata& metadata, const webm::Cluster& cluster) override {
        return webm::Callback::OnClusterEnd(metadata, cluster);
    }
    webm::Status OnTrackEntry(const webm::ElementMetadata& metadata, const webm::TrackEntry& track_entry) override {
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
        }
        return webm::Callback::OnTrackEntry(metadata, track_entry);
    }
    webm::Status OnCuePoint(const webm::ElementMetadata& metadata, const webm::CuePoint& cue_point) override {
        return webm::Callback::OnCuePoint(metadata, cue_point);
    }
    webm::Status OnEditionEntry(const webm::ElementMetadata& metadata, const webm::EditionEntry& edition_entry) override {
        return webm::Callback::OnEditionEntry(metadata, edition_entry);
    }
    webm::Status OnTag(const webm::ElementMetadata& metadata, const webm::Tag& tag) override {
        return webm::Callback::OnTag(metadata, tag);
    }
    webm::Status OnSegmentEnd(const webm::ElementMetadata& metadata) override {
        return webm::Callback::OnSegmentEnd(metadata);
    }
    FileUtils::WebM* m_parent_webm{nullptr};
};

namespace FileUtils {

WebM::WebM(std::filesystem::path filesystem) noexcept {
    const auto err_str = std::format("Failed to load WebM file: {}", filesystem.string());
    GUARANTEE_OR_DIE(Load(filesystem), err_str.c_str());
}

bool WebM::Load(std::filesystem::path filepath) noexcept {

    if(auto buffer = FileUtils::ReadBinaryBufferFromFile(filepath); buffer.has_value()) {
        m_path = filepath;
        webm::BufferReader reader{buffer.value()};
        MyWebMCallback callback{};
        callback.m_parent_webm = this;
        webm::WebmParser parser{};
        if(auto status = parser.Feed(&callback, &reader); status.is_parsing_error()) {
            return false;
        } else {
            return status.completed_ok();
        }
    }
    return false;
}

std::filesystem::path WebM::GetFilepath() const noexcept {
    return m_path;
}

void WebM::SetPixelDimensions(uint64_t width, uint64_t height) noexcept {
    m_width = width;
    m_height = height;
}

void WebM::SetDuration(TimeUtils::FPSeconds newDuration) noexcept {
    m_length = newDuration;
}

} // namespace FileUtils
