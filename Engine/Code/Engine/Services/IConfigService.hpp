#pragma once

#include "Engine/Services/IService.hpp"

#include <filesystem>
#include <string>

class IConfigService : public IService {
public:
    virtual ~IConfigService() noexcept {};

    [[nodiscard]] virtual bool LoadFromFile(const std::filesystem::path& filepath) noexcept = 0;
    [[nodiscard]] virtual bool AppendFromFile(const std::filesystem::path& filepath) noexcept = 0;
    [[nodiscard]] virtual bool AppendToFile(const std::filesystem::path& filepath) noexcept = 0;
    [[nodiscard]] virtual bool SaveToFile(const std::filesystem::path& filepath) noexcept = 0;

    [[nodiscard]] virtual bool HasKey(const std::string& key) const noexcept = 0;

    virtual void GetValue(const std::string& key, bool& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, char& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, unsigned char& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, signed char& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, unsigned int& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, int& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, long& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, unsigned long& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, long long& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, unsigned long long& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, float& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, double& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, long double& value) const noexcept = 0;
    virtual void GetValue(const std::string& key, std::string& value) const noexcept = 0;

    virtual void GetValueOr(const std::string& key, bool& value, bool defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, char& value, char defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, unsigned char& value, unsigned char defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, signed char& value, signed char defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, unsigned int& value, unsigned int defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, int& value, int defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, long& value, long defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, unsigned long& value, unsigned long defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, long long& value, long long defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, unsigned long long& value, unsigned long long defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, float& value, float defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, double& value, double defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, long double& value, long double defaultValue) const noexcept = 0;
    virtual void GetValueOr(const std::string& key, std::string& value, std::string defaultValue) const noexcept = 0;

    virtual void SetValue(const std::string& key, const bool& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const char& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const unsigned char& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const signed char& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const unsigned int& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const int& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const long& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const unsigned long& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const long long& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const unsigned long long& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const float& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const double& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const long double& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const std::string& value) noexcept = 0;
    virtual void SetValue(const std::string& key, const char* value) noexcept = 0;

    virtual void PrintConfig(const std::string& key, std::ostream& output) const noexcept = 0;
    virtual void PrintConfigs(std::ostream& output) const noexcept = 0;
    virtual void PrintKeyValue(std::ostream& output, const std::string& key, const std::string& value) const noexcept = 0;

protected:
private:
    
};

class NullConfigService : public IConfigService {
public:
    virtual ~NullConfigService() noexcept {};
    bool LoadFromFile([[maybe_unused]] const std::filesystem::path& filepath) noexcept override {}
    bool AppendFromFile([[maybe_unused]] const std::filesystem::path& filepath) noexcept override {}
    bool AppendToFile([[maybe_unused]] const std::filesystem::path& filepath) noexcept override {}
    bool SaveToFile([[maybe_unused]] const std::filesystem::path& filepath) noexcept override {}
    bool HasKey([[maybe_unused]] const std::string& key) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] bool& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] char& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned char& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] signed char& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned int& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] int& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] long& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned long& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] long long& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned long long& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] float& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] double& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] long double& value) const noexcept override {}
    void GetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] std::string& value) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] bool& value, [[maybe_unused]] bool defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] char& value, [[maybe_unused]] char defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned char& value, [[maybe_unused]] unsigned char defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] signed char& value, [[maybe_unused]] signed char defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned int& value, [[maybe_unused]] unsigned int defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] int& value, [[maybe_unused]] int defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] long& value, [[maybe_unused]] long defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned long& value, [[maybe_unused]] unsigned long defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] long long& value, [[maybe_unused]] long long defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] unsigned long long& value, [[maybe_unused]] unsigned long long defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] float& value, [[maybe_unused]] float defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] double& value, [[maybe_unused]] double defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] long double& value, [[maybe_unused]] long double defaultValue) const noexcept override {}
    void GetValueOr([[maybe_unused]] const std::string& key, [[maybe_unused]] std::string& value, [[maybe_unused]] std::string defaultValue) const noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const bool& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const char& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const unsigned char& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const signed char& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const unsigned int& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const int& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const long& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const unsigned long& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const long long& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const unsigned long long& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const float& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const double& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const long double& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const std::string& value) noexcept override {}
    void SetValue([[maybe_unused]] const std::string& key, [[maybe_unused]] const char* value) noexcept override {}
    void PrintConfig([[maybe_unused]] const std::string& key, [[maybe_unused]] std::ostream& output) const noexcept override {}
    void PrintConfigs([[maybe_unused]] std::ostream& output) const noexcept override {}
    void PrintKeyValue([[maybe_unused]] std::ostream& output, [[maybe_unused]] const std::string& key, [[maybe_unused]] const std::string& value) const noexcept override {}

protected:
private:
    
};
