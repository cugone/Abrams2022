#include "Engine/Core/Riff.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include <iostream>
#include <sstream>

namespace FileUtils {

namespace RiffChunkID {
constexpr const bool IsValid(const char* id) noexcept {
    return (StringUtils::FourCC(id) == RiffChunkID::RIFF
            || StringUtils::FourCC(id) == RiffChunkID::LIST
            || StringUtils::FourCC(id) == RiffChunkID::INFO
            || StringUtils::FourCC(id) == RiffChunkID::WAVE
            || StringUtils::FourCC(id) == RiffChunkID::AVI);
}
} // namespace RiffChunkID

bool Riff::ParseDataIntoChunks(std::vector<uint8_t>& buffer) noexcept {
    std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    stream.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    buffer.clear();
    buffer.shrink_to_fit();
    stream.clear();
    stream.seekp(0);
    stream.seekg(0);
    detail::RiffHeader cur_header{};
    while(stream.read(reinterpret_cast<char*>(&cur_header), sizeof(cur_header))) {
        auto cur_chunk = detail::RiffChunk{};
        cur_chunk.header = cur_header;
        switch(StringUtils::FourCC(cur_header.fourcc)) {
        case RiffChunkID::RIFF: {
            auto subdata = detail::RiffSubChunk{};
            if(!stream.read(reinterpret_cast<char*>(&subdata.fourcc), 4)) {
                return false;
            }
            subdata.subdata_length = std::size_t{cur_header.length - uint32_t{4u}};
            subdata.subdata = std::move(std::make_unique<uint8_t[]>(subdata.subdata_length));
            if(!stream.read(reinterpret_cast<char*>(subdata.subdata.get()), subdata.subdata_length)) {
                return false;
            }
            cur_chunk.data = std::move(subdata);
            break;
        }
        case RiffChunkID::INFO: {
            auto subdata = detail::RiffSubChunk{};
            if(!stream.read(reinterpret_cast<char*>(&subdata.fourcc), 4)) {
                return false;
            }
            subdata.subdata_length = std::size_t{cur_header.length - uint32_t{4u}};
            subdata.subdata = std::move(std::make_unique<uint8_t[]>(subdata.subdata_length));
            if(!stream.read(reinterpret_cast<char*>(subdata.subdata.get()), subdata.subdata_length)) {
                return false;
            }
            {
                std::ostringstream err_ss{};
                std::string hdr{"RIFF INFO Chunk."};
                err_ss.write(hdr.c_str(), hdr.size());
                std::string len{"Length: "};
                len += std::to_string(cur_header.length);
                err_ss.write(len.c_str(), len.size());
                err_ss.write(reinterpret_cast<char*>(subdata.subdata.get()), subdata.subdata_length);
                DebuggerPrintf(err_ss.str().c_str());
            }
            cur_chunk.data = std::move(subdata);
            break;
        }
        case RiffChunkID::LIST: {
            auto subdata = detail::RiffSubChunk{};
            if(!stream.read(reinterpret_cast<char*>(&subdata.fourcc), 4)) {
                return false;
            }
            subdata.subdata_length = std::size_t{cur_header.length - uint32_t{4u}};
            subdata.subdata = std::move(std::make_unique<uint8_t[]>(subdata.subdata_length));
            auto subdata_head = subdata.subdata.get();
            if(!stream.read(reinterpret_cast<char*>(subdata_head), subdata.subdata_length)) {
                return false;
            }
            auto&& list_chunk = std::move(ReadListChunk(stream));
            if(list_chunk) {
                cur_chunk = std::move(list_chunk.value());
            }
            break;
        }
        default: {
            {
                std::ostringstream err_ss{};
                std::string hdr{"Unknown RIFF Chunk ID: "};
                hdr += StringUtils::FourCCToString(cur_header.fourcc);
                err_ss.write(hdr.c_str(), hdr.size());
                std::string len{" Length: "};
                len += std::to_string(cur_header.length);
                err_ss.write(len.c_str(), len.size());
                DebuggerPrintf(err_ss.str().c_str());
            }
            stream.seekp(cur_header.length, std::ios_base::cur);
            stream.seekg(cur_header.length, std::ios_base::cur);
            break;
        }
        }
        m_chunks.push_back(std::move(cur_chunk));
    }
    m_current_chunk = m_chunks.begin();
    return true;
}

detail::RiffChunk* Riff::GetNextChunk() const noexcept {
    if(m_current_chunk == m_chunks.end()) {
        return nullptr;
    }
    decltype(m_chunks)::iterator chunk = m_current_chunk;
    m_current_chunk++;
    return &(*chunk);
}

unsigned int Riff::Load(std::filesystem::path filename) noexcept {
    if(const auto& buffer = FileUtils::ReadBinaryBufferFromFile(filename)) {
        if(RIFF_SUCCESS == Load(buffer.value())) {
            return RIFF_SUCCESS;
        }
    }
    return RIFF_ERROR_INVALID_ARGUMENT;
}

unsigned int Riff::Load(const std::vector<unsigned char>& data) noexcept {
    std::vector<unsigned char> buffer = data;
    if(!ParseDataIntoChunks(buffer)) {
        return RIFF_ERROR_NOT_A_RIFF;
    }
    return RIFF_SUCCESS;
}

std::optional<FileUtils::detail::RiffChunk> Riff::ReadListChunk(std::stringstream& stream) noexcept {
    if(!stream) {
        return {};
    }
    detail::RiffHeader cur_header{};
    if(stream.read(reinterpret_cast<char*>(&cur_header), sizeof(cur_header))) {
        auto cur_chunk = detail::RiffChunk{};
        cur_chunk.header = cur_header;
        {
            auto subdata = detail::RiffSubChunk{};
            StringUtils::CopyFourCC(subdata.fourcc, cur_header.fourcc);
            uint32_t subdata_length = cur_header.length - 4;
            subdata.subdata = std::move(std::make_unique<uint8_t[]>(subdata_length));
            if(!stream.read(reinterpret_cast<char*>(subdata.subdata.get()), subdata_length)) {
                return {};
            }
            cur_chunk.data = std::move(subdata);
            return cur_chunk; //std::make_optional<detail::RiffChunk>(cur_chunk);
        }
    }
    return {};
}

} // namespace FileUtils