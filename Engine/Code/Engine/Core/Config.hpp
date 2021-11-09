#pragma once

#include "Engine/Services/IConfigService.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

class KeyValueParser;

class Config : public IConfigService {
public:
    Config() = default;
    Config(const Config& other) = delete;
    Config& operator=(const Config& rhs) = delete;
    Config(Config&& other) noexcept;
    Config& operator=(Config&& rhs) noexcept;
    explicit Config(KeyValueParser&& kvp) noexcept;
    virtual ~Config() noexcept = default;

    [[nodiscard]] bool LoadFromFile(const std::filesystem::path& filepath) noexcept override;
    [[nodiscard]] bool AppendFromFile(const std::filesystem::path& filepath) noexcept override;
    [[nodiscard]] bool AppendToFile(const std::filesystem::path& filepath) noexcept override;
    [[nodiscard]] bool SaveToFile(const std::filesystem::path& filepath) noexcept override;

    [[nodiscard]] bool HasKey(const std::string& key) const noexcept override;

    void GetValue(const std::string& key, bool& value) const noexcept override;
    void GetValue(const std::string& key, char& value) const noexcept override;
    void GetValue(const std::string& key, unsigned char& value) const noexcept override;
    void GetValue(const std::string& key, signed char& value) const noexcept override;
    void GetValue(const std::string& key, unsigned int& value) const noexcept override;
    void GetValue(const std::string& key, int& value) const noexcept override;
    void GetValue(const std::string& key, long& value) const noexcept override;
    void GetValue(const std::string& key, unsigned long& value) const noexcept override;
    void GetValue(const std::string& key, long long& value) const noexcept override;
    void GetValue(const std::string& key, unsigned long long& value) const noexcept override;
    void GetValue(const std::string& key, float& value) const noexcept override;
    void GetValue(const std::string& key, double& value) const noexcept override;
    void GetValue(const std::string& key, long double& value) const noexcept override;
    void GetValue(const std::string& key, std::string& value) const noexcept override;

    void SetValue(const std::string& key, const bool& value) noexcept override;
    void SetValue(const std::string& key, const char& value) noexcept override;
    void SetValue(const std::string& key, const unsigned char& value) noexcept override;
    void SetValue(const std::string& key, const signed char& value) noexcept override;
    void SetValue(const std::string& key, const unsigned int& value) noexcept override;
    void SetValue(const std::string& key, const int& value) noexcept override;
    void SetValue(const std::string& key, const long& value) noexcept override;
    void SetValue(const std::string& key, const unsigned long& value) noexcept override;
    void SetValue(const std::string& key, const long long& value) noexcept override;
    void SetValue(const std::string& key, const unsigned long long& value) noexcept override;
    void SetValue(const std::string& key, const float& value) noexcept override;
    void SetValue(const std::string& key, const double& value) noexcept override;
    void SetValue(const std::string& key, const long double& value) noexcept override;
    void SetValue(const std::string& key, const std::string& value) noexcept override;
    void SetValue(const std::string& key, const char* value) noexcept override;

    void PrintConfig(const std::string& key, std::ostream& output) const noexcept override;
    void PrintConfigs(std::ostream& output) const noexcept override;
    void PrintKeyValue(std::ostream& output, const std::string& key, const std::string& value) const noexcept override;

    friend std::ostream& operator<<(std::ostream& output, const Config& config) noexcept;
    friend std::istream& operator>>(std::istream& input, Config& config) noexcept;

protected:
private:
    std::map<std::string, std::string> _config{};
};
