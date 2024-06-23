#pragma once

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/Matrix4.hpp"

#include <filesystem>
#include <string>

class Texture;
class Shader;

struct screenshot_job_t {
public:
    screenshot_job_t();
    screenshot_job_t(std::filesystem::path location);
    screenshot_job_t(std::string location);
    operator bool() const noexcept;
    operator std::string() const noexcept;
    operator std::filesystem::path() const noexcept;
    void clear() noexcept;

private:
    std::filesystem::path m_saveLocation{};
};

struct matrix_buffer_t {
    Matrix4 model{};
    Matrix4 view{};
    Matrix4 projection{};
};

struct time_buffer_t {
    float game_time = 0.0f;
    float system_time = 0.0f;
    float game_frame_time = 0.0f;
    float system_frame_time = 0.0f;
};

struct PointLightDesc {
    Vector3 position = Vector3::Zero;
    Vector3 attenuation = Vector3::Z_Axis;
    float intensity = 1.0f;
    Rgba color = Rgba::White;
};

struct DirectionalLightDesc {
    Vector3 direction = Vector3::X_Axis;
    Vector3 attenuation = Vector3::X_Axis;
    float intensity = 1.0f;
    Rgba color = Rgba::White;
};

struct SpotLightDesc {
    Vector3 position = Vector3::Zero;
    Vector3 direction = Vector3::X_Axis;
    Vector3 attenuation = Vector3::Z_Axis;
    Vector2 inner_outer_anglesDegrees = Vector2{30.0f, 60.0f};
    float intensity = 1.0f;
    Rgba color = Rgba::White;
};

struct light_t {
    Vector4 position = Vector4::Zero;
    Vector4 color = Vector4::One_XYZ_Zero_W;
    Vector4 attenuation = Vector4::Z_Axis;
    Vector4 specAttenuation = Vector4::X_Axis;
    Vector4 innerOuterDotThresholds = Vector4(-2.0f, -3.0f, 0.0f, 0.0f);
    Vector4 direction = -Vector4::Z_Axis;
};

constexpr const unsigned int max_light_count = 16;

struct lighting_buffer_t {
    light_t lights[max_light_count] = {light_t{}};
    Vector4 ambient = Vector4::Zero;
    Vector4 specular_glossy_emissive_factors = Vector4(1.0f, 8.0f, 0.0f, 1.0f);
    Vector4 eye_position = Vector4::Zero;
    int useVertexNormals = 0;
    float padding[3] = {0.0f, 0.0f, 0.0f};
};

struct ComputeJob {
    std::size_t uavCount = 0;
    std::vector<Texture*> uavTextures{};
    Shader* computeShader = nullptr;
    unsigned int threadGroupCountX = 1;
    unsigned int threadGroupCountY = 1;
    unsigned int threadGroupCountZ = 1;
    ComputeJob() = default;
    ComputeJob(std::size_t uavCount,
               const std::vector<Texture*>& uavTextures,
               Shader* computeShader,
               unsigned int threadGroupCountX,
               unsigned int threadGroupCountY,
               unsigned int threadGroupCountZ) noexcept;
    ~ComputeJob() noexcept;
};
