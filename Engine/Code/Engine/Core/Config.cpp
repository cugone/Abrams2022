#include "Engine/Core/Config.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <algorithm>
#include <locale>
#include <sstream>

Config::Config(KeyValueParser&& kvp) noexcept
: m_config(std::move(kvp.Release())) {
    /* DO NOTHING */
}

Config::Config(Config&& other) noexcept
: m_config(std::move(other.m_config)) {
    other.m_config = {};
}

Config& Config::operator=(Config&& rhs) noexcept {
    m_config = rhs.m_config;
    rhs.m_config = {};
    return *this;
}

bool Config::LoadFromFile(const std::filesystem::path& filepath) noexcept {
    if(std::filesystem::exists(filepath)) {
        KeyValueParser kvp(filepath);
        m_config = std::move(kvp.Release());
        return true;
    }
    return false;
}

bool Config::AppendFromFile(const std::filesystem::path& filepath) noexcept {
    if(std::filesystem::exists(filepath)) {
        KeyValueParser kvp(filepath);
        const auto&& new_entries = std::move(kvp.Release());
        for(const auto& [key, value] : new_entries) {
            if(HasKey(key)) {
                continue;
            }
            SetValue(key, value);
        }
        return true;
    }
    return false;
}

bool Config::AppendToFile(const std::filesystem::path& filepath) noexcept {
    if(std::filesystem::exists(filepath)) {
        std::ofstream ofs;
        ofs.open(filepath, std::ios_base::app);
        PrintConfigs(ofs);
        ofs.flush();
        ofs.close();
        return true;
    }
    return SaveToFile(filepath);
}

bool Config::SaveToFile(const std::filesystem::path& filepath) noexcept {
    std::ofstream ofs;
    ofs.open(filepath);
    PrintConfigs(ofs);
    ofs.flush();
    ofs.close();
    return true;
}

bool Config::HasKey(const std::string& key) const noexcept {
    return m_config.find(key) != m_config.end();
}

void Config::GetValue(const std::string& key, bool& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        try {
            int keyAsInt = std::stoi(found->second);
            value = keyAsInt != 0;
        } catch(...) {
            std::string keyAsString = StringUtils::ToLowerCase(found->second);
            if(keyAsString == "true") {
                value = true;
            } else if(keyAsString == "false") {
                value = false;
            } else {
                value = false;
            }
        }
    }
}

void Config::GetValue(const std::string& key, char& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = *(found->second.begin());
    }
}

void Config::GetValue(const std::string& key, unsigned char& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<unsigned char>(std::stoul(found->second));
    }
}

void Config::GetValue(const std::string& key, signed char& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<signed char>(std::stoi(found->second));
    }
}

void Config::GetValue(const std::string& key, unsigned int& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<unsigned int>(std::stoul(found->second));
    }
}

void Config::GetValue(const std::string& key, int& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoi(found->second);
    }
}

void Config::GetValue(const std::string& key, long& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stol(found->second);
    }
}

void Config::GetValue(const std::string& key, unsigned long& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoul(found->second);
    }
}

void Config::GetValue(const std::string& key, long long& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoll(found->second);
    }
}

void Config::GetValue(const std::string& key, unsigned long long& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoull(found->second);
    }
}

void Config::GetValue(const std::string& key, float& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stof(found->second);
    }
}

void Config::GetValue(const std::string& key, double& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stod(found->second);
    }
}

void Config::GetValue(const std::string& key, long double& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stold(found->second);
    }
}

void Config::GetValue(const std::string& key, std::string& value) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = found->second;
    }
}

void Config::GetValueOr(const std::string& key, bool& value, bool defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        try {
            int keyAsInt = std::stoi(found->second);
            value = keyAsInt != 0;
        } catch(...) {
            std::string keyAsString = StringUtils::ToLowerCase(found->second);
            if(keyAsString == "true") {
                value = true;
            } else if(keyAsString == "false") {
                value = false;
            } else {
                value = false;
            }
        }
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, char& value, char defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = *(found->second.begin());
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, unsigned char& value, unsigned char defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<unsigned char>(std::stoul(found->second));
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, signed char& value, signed char defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<signed char>(std::stoi(found->second));
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, unsigned int& value, unsigned int defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = static_cast<unsigned int>(std::stoul(found->second));
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, int& value, int defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoi(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, long& value, long defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stol(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, unsigned long& value, unsigned long defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoul(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, long long& value, long long defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoll(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, unsigned long long& value, unsigned long long defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stoull(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, float& value, float defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stof(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, double& value, double defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stod(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, long double& value, long double defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = std::stold(found->second);
    } else {
        value = defaultValue;
    }
}

void Config::GetValueOr(const std::string& key, std::string& value, std::string defaultValue) const noexcept {
    if(auto found = m_config.find(key); found != m_config.end()) {
        value = found->second;
    } else {
        value = defaultValue;
    }
}

void Config::SetValue(const std::string& key, const char& value) noexcept {
    m_config[key] = value;
}

void Config::SetValue(const std::string& key, const unsigned char& value) noexcept {
    m_config[key] = value;
}

void Config::SetValue(const std::string& key, const signed char& value) noexcept {
    m_config[key] = value;
}

void Config::SetValue(const std::string& key, const bool& value) noexcept {
    m_config[key] = value ? "true" : "false";
}

void Config::SetValue(const std::string& key, const unsigned int& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const int& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const unsigned long& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long long& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const unsigned long long& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const float& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const double& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long double& value) noexcept {
    m_config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const std::string& value) noexcept {
    m_config[key] = value;
}

void Config::SetValue(const std::string& key, const char* value) noexcept {
    SetValue(key, value ? std::string(value) : std::string{});
}

void Config::PrintConfig(const std::string& key, std::ostream& output) const noexcept {
    if(const auto iter = m_config.find(key); iter != std::end(m_config)) {
        PrintKeyValue(output, iter->first, iter->second);
    }
}

void Config::PrintConfigs(std::ostream& output) const noexcept {
    for(const auto& [key, value] : m_config) {
        PrintKeyValue(output, key, value);
    }
}

void Config::PrintKeyValue(std::ostream& output, const std::string& key, const std::string& value) const noexcept {

    //TODO: Replace with std::format({}={}, key, value); ?

    bool value_has_space = false;
    for(const auto& c : value) {
        value_has_space |= std::isspace(c, std::locale(""));
        if(value_has_space) {
            break;
        }
    }
    output << key << '=';
    if(value_has_space) {
        output << '"';
    }
    output << value;
    if(value_has_space) {
        output << '"';
    }
    output << '\n';
}

std::ostream& operator<<(std::ostream& output, const Config& config) noexcept {
    config.PrintConfigs(output);
    return output;
}

std::istream& operator>>(std::istream& input, Config& config) noexcept {
    KeyValueParser kvp(input);
    config.m_config = std::move(kvp.Release());
    return input;
}