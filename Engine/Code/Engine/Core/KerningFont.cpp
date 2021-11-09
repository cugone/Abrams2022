#include "Engine/Core/KerningFont.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Material.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <sstream>

float KerningFont::CalculateTextWidth(const KerningFont& font, const std::string& text, float scale /*= 1.0f*/) noexcept {
    if(text.find('\n') != std::string::npos) {
        return CalculateLongestMultiline(font, text, scale);
    }

    auto cursor_x = 0.0f;

    for(auto char_iter = text.begin(); char_iter != text.end(); /* DO NOTHING */) {
        const auto current_char_def = font.GetCharDef(*char_iter);
        const auto previous_char = char_iter++;
        if(char_iter != text.end()) {
            auto kern_value = 0.0f;
            if(const auto kern_iter = font._kernmap.find(std::make_pair(*previous_char, *char_iter)); kern_iter != font._kernmap.end()) {
                kern_value = static_cast<float>(kern_iter->second);
            }
            cursor_x += current_char_def.xadvance + kern_value;
        } else {
            KerningFont::CharDef previous_char_def = font.GetCharDef(*previous_char);
            cursor_x += previous_char_def.xadvance;
        }
    }

    return cursor_x * scale;
}

float KerningFont::CalculateTextWidth(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    return CalculateTextWidth(*this, text, scale);
}

float KerningFont::CalculateTextHeight(const KerningFont& font, const std::string& text, float scale /*= 1.0f*/) noexcept {
    return (1.0f + static_cast<float>(std::count(text.begin(), text.end(), '\n'))) * font.GetLineHeight() * scale;
}

float KerningFont::CalculateTextHeight(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    return CalculateTextHeight(*this, text, scale);
}

float KerningFont::GetLineHeight() const noexcept {
    return static_cast<float>(_common.line_height);
}

float KerningFont::GetLineHeightAsUV() const noexcept {
    return GetLineHeight() / static_cast<float>(_common.scale.y);
}

const std::string& KerningFont::GetName() const noexcept {
    return _name;
}

KerningFont::CharDef KerningFont::GetCharDef(int ch) const noexcept {
    auto chardef_iter = _charmap.find(ch);
    if(chardef_iter == _charmap.end()) {
        chardef_iter = _charmap.find(-1);
        if(chardef_iter == _charmap.end()) {
            return CharDef{};
        }
    }
    return chardef_iter->second;
}

const KerningFont::CommonDef& KerningFont::GetCommonDef() const noexcept {
    return _common;
}

const KerningFont::InfoDef& KerningFont::GetInfoDef() const noexcept {
    return _info;
}

const std::vector<std::string>& KerningFont::GetImagePaths() const noexcept {
    return _image_paths;
}

const std::filesystem::path& KerningFont::GetFilePath() const noexcept {
    return _filepath;
}

bool KerningFont::LoadFromFile(std::filesystem::path filepath) noexcept {
    {
        namespace FS = std::filesystem;
        const auto path_exists = FS::exists(filepath);
        if(!path_exists) {
            DebuggerPrintf("Failed to read file: %s \nDoes not exist.\n", _filepath.string().c_str());
            return false;
        }
        filepath = FS::canonical(filepath);
        filepath.make_preferred();
        const auto is_not_directory = !FS::is_directory(filepath);
        const auto is_file = FS::is_regular_file(filepath);
        const auto is_fnt = filepath.has_extension() && StringUtils::ToLowerCase(filepath.extension().string()) == ".fnt";
        const auto is_valid = path_exists && is_not_directory && is_file && is_fnt;
        if(!is_valid) {
            DebuggerPrintf("%s is not a BMFont file.\n", filepath.string().c_str());
            return false;
        }
        if(_is_loaded) {
            DebuggerPrintf("%s is already loaded.\n", filepath.string().c_str());
            return false;
        }
        _filepath = filepath;
    }
    if(const auto& buffer = FileUtils::ReadBinaryBufferFromFile(_filepath.string()); buffer.has_value()) {
        if(buffer->size() < 4) {
            DebuggerPrintf("%s is not a BMFont file.\n", _filepath.string().c_str());
            return false;
        }
        return LoadFromBuffer(*buffer);
    } else {
        DebuggerPrintf("Failed to read file: %s \n", _filepath.string().c_str());
        return false;
    }
}

bool KerningFont::LoadFromBuffer(const std::vector<uint8_t>& buffer) noexcept {
    std::vector<uint8_t> out_buffer(buffer);
    const auto is_binary = out_buffer[0] == 66 && out_buffer[1] == 77 && out_buffer[2] == 70;
    const auto is_text = out_buffer[0] == 105 && out_buffer[1] == 110 && out_buffer[2] == 102 && out_buffer[3] == 111;
    if(is_binary) {
        _is_loaded = LoadFromBinary(out_buffer);
    } else if(is_text) {
        _is_loaded = LoadFromText(out_buffer);
    } else {
        _is_loaded = LoadFromXml(out_buffer);
    }
    return _is_loaded;
}

Material* KerningFont::GetMaterial() const noexcept {
    return _material;
}

void KerningFont::SetMaterial(Material* mat) noexcept {
    _material = mat;
}

int KerningFont::GetKerningValue(int first, int second) const noexcept {
    const auto iter = _kernmap.find(std::make_pair(first, second));
    if(iter != _kernmap.end()) {
        return (*iter).second;
    }
    return 0;
}

float KerningFont::CalculateLongestMultiline(const KerningFont& font, const std::string& text, float scale /*= 1.0f*/) noexcept {
    const auto lines = StringUtils::Split(text, '\n', false);
    const auto max_iter = std::max_element(std::begin(lines), std::end(lines), [](const std::string& a, const std::string& b) { return a.size() < b.size(); });
    const auto length = CalculateTextWidth(font, *max_iter, scale);
    return length;
}

float KerningFont::CalculateLongestMultiline(const std::string& text, float scale /*= 1.0f*/) const noexcept {
    return CalculateLongestMultiline(*this, text, scale);
}

bool KerningFont::LoadFromText(std::vector<unsigned char>& buffer) noexcept {
    auto bufferAsStr = std::string(buffer.begin(), buffer.end());
    bufferAsStr = StringUtils::ReplaceAll(bufferAsStr, "\r\n", "\n");
    buffer.clear();
    buffer.shrink_to_fit();
    auto kerning_count = 0u;
    {
        std::istringstream ss;
        ss.str(bufferAsStr);
        std::string cur_line{};
        while(std::getline(ss, cur_line)) {
            if(cur_line.empty()) {
                continue;
            }
            if(IsInfoLine(cur_line)) {
                if(!ParseInfoLine(cur_line)) {
                    return false;
                }
            } else if(IsCommonLine(cur_line)) {
                if(!ParseCommonLine(cur_line)) {
                    return false;
                }
            } else if(IsPageLine(cur_line)) {
                if(!ParsePageLine(cur_line)) {
                    return false;
                }
            } else if(IsCharsLine(cur_line)) {
                if(!ParseCharsLine(cur_line)) {
                    return false;
                }
            } else if(IsCharLine(cur_line)) {
                if(!ParseCharLine(cur_line)) {
                    return false;
                }
            } else if(IsKerningsLine(cur_line)) {
                if(!ParseKerningsLine(cur_line)) {
                    return false;
                }
            } else if(IsKerningLine(cur_line)) {
                if(!ParseKerningLine(cur_line)) {
                    return false;
                }
                ++kerning_count;
            }
        }
    }
    if(!kerning_count) {
        DebuggerPrintf("No kerning pairs found in font \"%s\"\n", _name.c_str());
    }
    return true;
}

bool KerningFont::IsInfoLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "info");
}

bool KerningFont::IsCommonLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "common");
}
bool KerningFont::IsPageLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "page");
}

bool KerningFont::IsCharsLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "chars");
}

bool KerningFont::IsCharLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "char");
}

bool KerningFont::IsKerningsLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "kernings");
}

bool KerningFont::IsKerningLine(const std::string& cur_line) noexcept {
    return StringUtils::StartsWith(cur_line, "kerning");
}

bool KerningFont::ParseInfoLine(const std::string& infoLine) noexcept {
    const auto& key_values = StringUtils::Split(infoLine, ' ', true);
    std::string key{};
    std::string value{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "face") {
                _info.face = value;
            }
            if(key == "size") {
                _info.em_size = std::stoi(value);
            }
            if(key == "bold") {
                _info.is_bold = std::stoi(value) != 0;
            }
            if(key == "italic") {
                _info.is_italic = std::stoi(value) != 0;
            }
            if(key == "charset") {
                _info.charset = value;
            }
            if(key == "unicode") {
                _info.charset = std::stoi(value) != 0;
            }
            if(key == "stretchH") {
                _info.stretch_height = std::stof(value) / 100.0f;
            }
            if(key == "smooth") {
                _info.is_smoothed = std::stoi(value) != 0;
            }
            if(key == "aa") {
                _info.is_aliased = std::stoi(value);
            }
            if(key == "padding") {
                const auto& pads = StringUtils::Split(value, ',', true);
                _info.padding.x = std::stoi(pads[0]);
                _info.padding.y = std::stoi(pads[1]);
                _info.padding.z = std::stoi(pads[2]);
                _info.padding.w = std::stoi(pads[3]);
            }
            if(key == "spacing") {
                const auto& spaces = StringUtils::Split(value, ',', true);
                _info.spacing.x = std::stoi(spaces[0]);
                _info.spacing.y = std::stoi(spaces[1]);
            }
            if(key == "outline") {
                _info.outline = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
    }
    _name = _info.face + std::to_string(_info.em_size);
    return true;
}

bool KerningFont::ParseCommonLine(const std::string& commonLine) noexcept {
    const auto& key_values = StringUtils::Split(commonLine, ' ', true);
    std::string key{};
    std::string value{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "lineHeight") {
                _common.line_height = std::stoi(value);
            }
            if(key == "base") {
                _common.base = std::stoi(value);
            }
            if(key == "scaleW") {
                _common.scale.x = std::stoi(value);
            }
            if(key == "scaleH") {
                _common.scale.y = std::stoi(value);
            }
            if(key == "pages") {
                _common.page_count = std::stoi(value);
            }
            if(key == "packed") {
                _common.is_packed = std::stoi(value) != 0;
            }
            if(key == "alphaChnl") {
                _common.alpha_channel = std::stoi(value);
            }
            if(key == "redChnl") {
                _common.red_channel = std::stoi(value);
            }
            if(key == "greenChnl") {
                _common.green_channel = std::stoi(value);
            }
            if(key == "blueChnl") {
                _common.blue_channel = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
    }
    _image_paths.resize(_common.page_count);
    return true;
}

bool KerningFont::ParsePageLine(const std::string& pageLine) noexcept {
    namespace FS = std::filesystem;
    const auto& key_values = StringUtils::Split(pageLine, ' ', true);
    std::string key{};
    std::string value{};
    std::size_t id = 0;
    std::string file{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "id") {
                id = std::stoi(value);
            }
            if(key == "file") {
                if(*value.begin() == '"') {
                    value.erase(value.begin());
                }
                if(value.back() == '"') {
                    value.pop_back();
                }
                file = value;
            }
        } catch(...) {
            return false;
        }
    }
    _image_paths[id] = file;
    return true;
}

bool KerningFont::ParseCharsLine(const std::string& charsLine) noexcept {
    const auto& key_values = StringUtils::Split(charsLine, ' ', true);
    std::string key{};
    std::string value{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "count") {
                _char_count = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
    }
    return true;
}

bool KerningFont::ParseCharLine(const std::string& charLine) noexcept {
    const auto& key_values = StringUtils::Split(charLine, ' ', true);
    std::string key{};
    std::string value{};
    CharDef def{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "id") {
                def.id = std::stoi(value);
            }
            if(key == "x") {
                def.position.x = std::stoi(value);
            }
            if(key == "y") {
                def.position.y = std::stoi(value);
            }
            if(key == "width") {
                def.dimensions.x = std::stoi(value);
            }
            if(key == "height") {
                def.dimensions.y = std::stoi(value);
            }
            if(key == "xoffset") {
                def.offsets.x = std::stoi(value);
            }
            if(key == "yoffset") {
                def.offsets.y = std::stoi(value);
            }
            if(key == "xadvance") {
                def.xadvance = std::stoi(value);
            }
            if(key == "page") {
                def.page_id = std::stoi(value);
            }
            if(key == "chnl") {
                def.channel_id = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
    }
    _charmap.insert_or_assign(def.id, def);
    return true;
}

bool KerningFont::ParseKerningsLine(const std::string& kerningsLine) noexcept {
    const auto& key_values = StringUtils::Split(kerningsLine, ' ', true);
    std::string key{};
    std::string value{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "count") {
                _kerns_count = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
    }
    return true;
}

bool KerningFont::ParseKerningLine(const std::string& kerningLine) noexcept {
    const auto& key_values = StringUtils::Split(kerningLine, ' ', true);
    std::string key{};
    std::string value{};
    KerningDef def{};
    for(const auto& key_value : key_values) {
        const auto& cur_key_value = StringUtils::Split(key_value, '=', true);
        if(cur_key_value.size() < 2) {
            continue;
        }
        key = cur_key_value[0];
        value = cur_key_value[1];
        try {
            if(key == "first") {
                def.first = std::stoi(value);
            }
            if(key == "second") {
                def.second = std::stoi(value);
            }
            if(key == "amount") {
                def.amount = std::stoi(value);
            }
        } catch(...) {
            return false;
        }
        _kernmap.insert_or_assign(std::make_pair(def.first, def.second), def.amount);
    }
    return true;
}

bool KerningFont::LoadFromXml(std::vector<unsigned char>& buffer) noexcept {
    tinyxml2::XMLDocument doc;
    std::string file(buffer.begin(), buffer.end());
    file = StringUtils::ReplaceAll(file, "\r\n", "\n");
    buffer.clear();
    buffer.shrink_to_fit();
    if(const auto& result = doc.Parse(file.c_str(), file.size()); result != tinyxml2::XML_SUCCESS) {
        return false;
    }

    const auto* xml_root = doc.RootElement();
    if(xml_root == nullptr) {
        return false;
    }

    DataUtils::ValidateXmlElement(*xml_root, "font", "info,common,pages,chars", "", "kernings");

    if(const auto* xml_info = xml_root->FirstChildElement("info")) {
        DataUtils::ValidateXmlElement(*xml_info, "info", "", "face,size,bold,italic,charset,unicode,stretchH,smooth,aa,padding,spacing,outline");

        _info.face = DataUtils::ParseXmlAttribute(*xml_info, "face", _info.face);
        _info.em_size = DataUtils::ParseXmlAttribute(*xml_info, "size", _info.em_size);
        _info.is_bold = DataUtils::ParseXmlAttribute(*xml_info, "bold", _info.is_bold);
        _info.is_italic = DataUtils::ParseXmlAttribute(*xml_info, "italic", _info.is_italic);
        _info.charset = DataUtils::ParseXmlAttribute(*xml_info, "charset", _info.charset);
        _info.is_unicode = DataUtils::ParseXmlAttribute(*xml_info, "unicode", _info.is_unicode);
        _info.stretch_height = DataUtils::ParseXmlAttribute(*xml_info, "stretchH", _info.stretch_height) / 100.0f;
        _info.is_smoothed = DataUtils::ParseXmlAttribute(*xml_info, "smooth", _info.is_smoothed);
        _info.is_aliased = DataUtils::ParseXmlAttribute(*xml_info, "aa", _info.is_aliased);
        {
            _name = StringUtils::ReplaceAll(_info.face, " ", "") + std::to_string(_info.em_size);
        }
        {
            std::string padding_str{};
            const auto& padding = StringUtils::Split(DataUtils::ParseXmlAttribute(*xml_info, "padding", padding_str));
            ASSERT_OR_DIE(padding.size() == 4, "FONT FORMAT INFO PADDING CHANGED");
            _info.padding.x = std::stoi(padding[0]);
            _info.padding.y = std::stoi(padding[1]);
            _info.padding.z = std::stoi(padding[2]);
            _info.padding.w = std::stoi(padding[3]);
        }
        {
            std::string spacing_str{};
            const auto& spacing = StringUtils::Split(DataUtils::ParseXmlAttribute(*xml_info, "spacing", spacing_str));
            ASSERT_OR_DIE(spacing.size() == 2, "FONT FORMAT INFO SPACING CHANGED");
            _info.spacing.x = std::stoi(spacing[0]);
            _info.spacing.y = std::stoi(spacing[1]);
        }
        _info.outline = DataUtils::ParseXmlAttribute(*xml_info, "outline", _info.outline);
    }

    if(const auto* xml_common = xml_root->FirstChildElement("common")) {
        DataUtils::ValidateXmlElement(*xml_common, "common", "", "lineHeight,base,scaleW,scaleH,pages,packed,alphaChnl,redChnl,greenChnl,blueChnl");
        _common.line_height = DataUtils::ParseXmlAttribute(*xml_common, "lineHeight", _common.line_height);
        _common.base = DataUtils::ParseXmlAttribute(*xml_common, "base", _common.base);
        _common.scale.x = DataUtils::ParseXmlAttribute(*xml_common, "scaleW", _common.scale.x);
        _common.scale.y = DataUtils::ParseXmlAttribute(*xml_common, "scaleH", _common.scale.y);
        _common.page_count = DataUtils::ParseXmlAttribute(*xml_common, "pages", _common.page_count);
        _common.is_packed = DataUtils::ParseXmlAttribute(*xml_common, "packed", _common.is_packed);
        _common.alpha_channel = DataUtils::ParseXmlAttribute(*xml_common, "alphaChnl", _common.alpha_channel);
        _common.red_channel = DataUtils::ParseXmlAttribute(*xml_common, "redChnl", _common.red_channel);
        _common.green_channel = DataUtils::ParseXmlAttribute(*xml_common, "greenChnl", _common.green_channel);
        _common.blue_channel = DataUtils::ParseXmlAttribute(*xml_common, "blueChnl", _common.blue_channel);
    }
    if(const auto* xml_pages = xml_root->FirstChildElement("pages")) {
        DataUtils::ValidateXmlElement(*xml_pages, "pages", "page", "");
        _image_paths.resize(_common.page_count);
        { //Scope constraint
            namespace FS = std::filesystem;
            DataUtils::ForEachChildElement(*xml_pages, "page",
                                           [this](const XMLElement& elem) {
                                               DataUtils::ValidateXmlElement(elem, "page", "", "id,file");
                                               unsigned int page_id = DataUtils::ParseXmlAttribute(elem, "id", 0u);
                                               auto page_file = DataUtils::ParseXmlAttribute(elem, "file", std::string{});
                                               _image_paths[page_id] = page_file;
                                           });
        }
    }
    if(const auto* xml_chars = xml_root->FirstChildElement("chars")) {
        DataUtils::ValidateXmlElement(*xml_chars, "chars", "char", "count");

        _char_count = DataUtils::ParseXmlAttribute(*xml_chars, "count", _char_count);
        DataUtils::ForEachChildElement(*xml_chars, "char",
                                       [this](const XMLElement& elem) {
                                           DataUtils::ValidateXmlElement(elem, "char", "", "id,x,y,width,height,xoffset,yoffset,xadvance,page,chnl");
                                           int id = DataUtils::ParseXmlAttribute(elem, "id", 0u);
                                           CharDef t;
                                           t.position.x = DataUtils::ParseXmlAttribute(elem, "x", t.position.x);
                                           t.position.y = DataUtils::ParseXmlAttribute(elem, "y", t.position.y);
                                           t.dimensions.x = DataUtils::ParseXmlAttribute(elem, "width", t.dimensions.x);
                                           t.dimensions.y = DataUtils::ParseXmlAttribute(elem, "height", t.dimensions.y);
                                           t.offsets.x = DataUtils::ParseXmlAttribute(elem, "xoffset", t.offsets.x);
                                           t.offsets.y = DataUtils::ParseXmlAttribute(elem, "yoffset", t.offsets.y);
                                           t.xadvance = DataUtils::ParseXmlAttribute(elem, "xadvance", t.xadvance);
                                           t.page_id = DataUtils::ParseXmlAttribute(elem, "page", t.page_id);
                                           t.channel_id = DataUtils::ParseXmlAttribute(elem, "chnl", t.channel_id);

                                           _charmap.insert_or_assign(id, t);
                                       });
    }
    if(const auto* xml_kernings = xml_root->FirstChildElement("kernings")) {
        DataUtils::ValidateXmlElement(*xml_kernings, "kernings", "kerning", "count");
        _kerns_count = DataUtils::ParseXmlAttribute(*xml_kernings, "count", _char_count);
        DataUtils::ForEachChildElement(*xml_kernings, "kerning",
                                       [this](const XMLElement& elem) {
                                           DataUtils::ValidateXmlElement(elem, "kerning", "", "first,second,amount");
                                           KerningDef def{};
                                           def.first = DataUtils::ParseXmlAttribute(elem, "first", def.first);
                                           def.second = DataUtils::ParseXmlAttribute(elem, "second", def.second);
                                           def.amount = DataUtils::ParseXmlAttribute(elem, "amount", def.amount);

                                           _kernmap.insert_or_assign(std::make_pair(def.first, def.second), def.amount);
                                       });
    } else {
        DebuggerPrintf("No kerning pairs found in font \"%s\"\n", _name.c_str());
    }
    return true;
}

bool KerningFont::LoadFromBinary(std::vector<unsigned char>& buffer) noexcept {
    //See https://www.angelcode.com/products/bmfont/doc/file_format.html#bin
    //for specifics regarding layout

    struct BMFBinaryHeader {
        char id[3];
        uint8_t version{};
    };

    struct BMFBinaryInfo {
        int16_t font_size{};
        uint8_t suibfr3_bitfield{};
        uint8_t char_set{};
        uint16_t stretch_height{};
        uint8_t aa{};
        uint8_t padding_up{};
        uint8_t padding_right{};
        uint8_t padding_down{};
        uint8_t padding_left{};
        uint8_t spacing_h{};
        uint8_t spacing_v{};
        uint8_t outline{};
    };

    struct BMFBinaryCommon {
        uint16_t line_height{};
        uint16_t base{};
        uint16_t scale_w{};
        uint16_t scale_h{};
        uint16_t pages{};
        uint8_t r7p_bitfield{};
        uint8_t alpha_channel{};
        uint8_t red_channel{};
        uint8_t green_channel{};
        uint8_t blue_channel{};
    };

    struct BMFBinaryChars {
        uint32_t id{};
        uint16_t x{};
        uint16_t y{};
        uint16_t width{};
        uint16_t height{};
        uint16_t x_offset{};
        uint16_t y_offset{};
        uint16_t x_advance{};
        uint8_t page{};
        uint8_t channel{};
    };

    struct BMFBinaryPages {
        char* page_names{};
    };

    struct BMFBinaryKerning {
        uint32_t first{};
        uint32_t second{};
        int16_t amount{};
    };

    BMFBinaryHeader header{};
    BMFBinaryInfo info{};
    BMFBinaryCommon common{};
    BMFBinaryChars chars{};
    BMFBinaryPages pages{};
    BMFBinaryKerning kerning{};
    std::stringstream bss(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    bss.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    buffer.clear();
    buffer.shrink_to_fit();

    bss.clear();
    bss.seekg(0);
    bss.seekp(0);
    uint8_t block_id{};
    uint32_t block_size{};
    constexpr const uint8_t CURRENT_BMF_VERSION = 3;
    if(bss.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        if(!(header.id[0] == 'B' && header.id[1] == 'M' && header.id[2] == 'F')) {
            DebuggerPrintf("%s is not a BMFont file.\n", _filepath.string().c_str());
            return false;
        }
        if(header.version != CURRENT_BMF_VERSION) {
            DebuggerPrintf("BMF VERSION NUMBER HAS CHANGED. See https://www.angelcode.com/products/bmfont/doc/file_format.html#bin for updates!\n");
            return false;
        }
    }
    int successful_block_reads = 0;
    constexpr const uint8_t BLOCK_ID_INFO = 1;
    constexpr const uint8_t BLOCK_ID_COMMON = 2;
    constexpr const uint8_t BLOCK_ID_PAGES = 3;
    constexpr const uint8_t BLOCK_ID_CHARS = 4;
    constexpr const uint8_t BLOCK_ID_KERNINGS = 5;
    while(bss.read(reinterpret_cast<char*>(&block_id), sizeof(block_id))) {
        switch(block_id) {
        case BLOCK_ID_INFO: {
            ++successful_block_reads;
            bss.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            bss.read(reinterpret_cast<char*>(&info), sizeof(info));
            std::string font_name(block_size - sizeof(info), '\0');
            bss.read(font_name.data(), font_name.size());
            if(info.font_size < 0) {
                DebuggerPrintf("%s uses \"Match char height\" option which will result in negative font sizes.\n", _filepath.string().c_str());
            }
            _info.charset = info.char_set;
            _info.em_size = info.font_size;
            _info.face = font_name.c_str();
            _info.is_aliased = info.aa != 0;
            _info.is_smoothed = (info.suibfr3_bitfield & 0b10000000) != 0;
            _info.is_unicode = (info.suibfr3_bitfield & 0b01000000) != 0;
            _info.is_italic = (info.suibfr3_bitfield & 0b00100000) != 0;
            _info.is_bold = (info.suibfr3_bitfield & 0b00010000) != 0;
            _info.is_fixedHeight = (info.suibfr3_bitfield & 0b00001000) != 0;
            _info.outline = info.outline;
            _info.padding = IntVector4(info.padding_up, info.padding_right, info.padding_down, info.padding_left);
            _info.spacing = IntVector2(info.spacing_h, info.spacing_v);
            _info.stretch_height = info.stretch_height / 100.0f;
            _name = StringUtils::ReplaceAll(_info.face, " ", "") + std::to_string(_info.em_size);
            break;
        }
        case BLOCK_ID_COMMON: {
            ++successful_block_reads;
            bss.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            bss.read(reinterpret_cast<char*>(&common), block_size);
            _common.alpha_channel = common.alpha_channel;
            _common.base = common.base;
            _common.blue_channel = common.blue_channel;
            _common.green_channel = common.green_channel;
            _common.is_packed = (common.r7p_bitfield & 0b00000001) != 0;
            _common.line_height = common.line_height;
            _common.page_count = common.pages;
            _image_paths.resize(_common.page_count);
            _common.red_channel = common.red_channel;
            _common.scale = IntVector2(common.scale_w, common.scale_h);
            break;
        }
        case BLOCK_ID_PAGES: {
            ++successful_block_reads;
            bss.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            uint8_t page_name_length = (uint8_t)(((uint32_t)block_size / (uint32_t)_common.page_count) - (uint32_t)1);
            for(std::size_t i = 0; i < static_cast<std::size_t>(_common.page_count); ++i) {
                _image_paths[i].resize(page_name_length);
                bss.read(_image_paths[i].data(), _image_paths[i].size() + 1);
            }
            break;
        }
        case BLOCK_ID_CHARS: {
            ++successful_block_reads;
            bss.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            uint32_t chars_size = sizeof(chars);
            uint32_t char_count = block_size / chars_size;
            for(uint32_t i = 0; i < char_count; ++i) {
                if(!bss.read(reinterpret_cast<char*>(&chars), chars_size)) {
                    DebuggerPrintf("%s is not a BMFont file.\n", _filepath.string().c_str());
                    return false;
                }
                CharDef d{};
                d.channel_id = chars.channel;
                d.dimensions = IntVector2(chars.width, chars.height);
                d.id = chars.id;
                d.offsets = IntVector2(chars.x_offset, chars.y_offset);
                d.page_id = chars.page;
                d.position = IntVector2(chars.x, chars.y);
                d.xadvance = chars.x_advance;
                _charmap.insert_or_assign(d.id, d);
            }
            break;
        }
        case BLOCK_ID_KERNINGS: {
            ++successful_block_reads;
            bss.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            uint32_t kerns_size = sizeof(kerning);
            uint32_t kerns_count = block_size / kerns_size;
            for(uint32_t i = 0; i < kerns_count; ++i) {
                if(bss.read(reinterpret_cast<char*>(&kerning), kerns_size)) {
                    KerningDef d{};
                    d.first = kerning.first;
                    d.second = kerning.second;
                    d.amount = kerning.amount;
                    _kernmap.insert_or_assign(std::make_pair(d.first, d.second), d.amount);
                }
            }
            break;
        }
        default: {
            DebuggerPrintf("BMF LAYOUT HAS CHANGED. See https://www.angelcode.com/products/bmfont/doc/file_format.html#bin for updates!\n");
            return false;
        }
        }
    }
    if(successful_block_reads < BLOCK_ID_KERNINGS) {
        DebuggerPrintf("No kerning pairs found in font \"%s\"\n", _name.c_str());
    }
    return true;
}
