#include "Engine/Core/Config.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <algorithm>
#include <locale>
#include <sstream>

Config::Config(KeyValueParser&& kvp) noexcept
: _config(std::move(kvp.Release())) {
    /* DO NOTHING */
}

Config::Config(Config&& other) noexcept
: _config(std::move(other._config)) {
    other._config = {};
}

Config& Config::operator=(Config&& rhs) noexcept {
    _config = rhs._config;
    rhs._config = {};
    return *this;
}

bool Config::LoadFromFile(const std::filesystem::path& filepath) noexcept {
    if(std::filesystem::exists(filepath)) {
        KeyValueParser kvp(filepath);
        _config = std::move(kvp.Release());
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
    return _config.find(key) != _config.end();
}

void Config::GetValue(const std::string& key, bool& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
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
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = *(found->second.begin());
    }
}

void Config::GetValue(const std::string& key, unsigned char& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = static_cast<unsigned char>(std::stoul(found->second));
    }
}

void Config::GetValue(const std::string& key, signed char& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = static_cast<signed char>(std::stoi(found->second));
    }
}

void Config::GetValue(const std::string& key, unsigned int& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = static_cast<unsigned int>(std::stoul(found->second));
    }
}

void Config::GetValue(const std::string& key, int& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stoi(found->second);
    }
}

void Config::GetValue(const std::string& key, long& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stol(found->second);
    }
}

void Config::GetValue(const std::string& key, unsigned long& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stoul(found->second);
    }
}

void Config::GetValue(const std::string& key, long long& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stoll(found->second);
    }
}

void Config::GetValue(const std::string& key, unsigned long long& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stoull(found->second);
    }
}

void Config::GetValue(const std::string& key, float& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stof(found->second);
    }
}

void Config::GetValue(const std::string& key, double& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stod(found->second);
    }
}

void Config::GetValue(const std::string& key, long double& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = std::stold(found->second);
    }
}

void Config::GetValue(const std::string& key, std::string& value) const noexcept {
    auto found = _config.find(key);
    if(found != _config.end()) {
        value = found->second;
    }
}

void Config::SetValue(const std::string& key, const char& value) noexcept {
    _config[key] = value;
}

void Config::SetValue(const std::string& key, const unsigned char& value) noexcept {
    _config[key] = value;
}

void Config::SetValue(const std::string& key, const signed char& value) noexcept {
    _config[key] = value;
}

void Config::SetValue(const std::string& key, const bool& value) noexcept {
    _config[key] = value ? "true" : "false";
}

void Config::SetValue(const std::string& key, const unsigned int& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const int& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const unsigned long& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long long& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const unsigned long long& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const float& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const double& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const long double& value) noexcept {
    _config[key] = std::to_string(value);
}

void Config::SetValue(const std::string& key, const std::string& value) noexcept {
    _config[key] = value;
}

void Config::SetValue(const std::string& key, const char* value) noexcept {
    SetValue(key, value ? std::string(value) : std::string{});
}

void Config::PrintConfig(const std::string& key, std::ostream& output) const noexcept {
    if(const auto iter = _config.find(key); iter != std::end(_config)) {
        PrintKeyValue(output, iter->first, iter->second);
    }
}

void Config::PrintConfigs(std::ostream& output) const noexcept {
    for(const auto& [key, value] : _config) {
        PrintKeyValue(output, key, value);
    }
}

void Config::PrintKeyValue(std::ostream& output, const std::string& key, const std::string& value) const noexcept {
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
    config._config = std::move(kvp.Release());
    return input;
}