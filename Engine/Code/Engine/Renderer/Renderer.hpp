#pragma once

#include "Engine/Core/Config.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/RHI/RHI.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/Camera3D.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/RenderTargetStack.hpp"
#include "Engine/Renderer/StructuredBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Renderer/Vertex3DInstanced.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/VertexBufferInstanced.hpp"

#include "Engine/Services/IRendererService.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AABB2;
class BlendState;
class Config;
class ConstantBuffer;
class DepthStencilState;
struct DepthStencilDesc;
class Disc2;
class Frustum;
class FrameBuffer;
class IndexBuffer;
class IntVector3;
class KerningFont;
class Material;
class OBB2;
class Polygon2;
class RasterState;
struct RasterDesc;
class Rgba;
class Sampler;
struct SamplerDesc;
class Shader;
class ShaderProgram;
struct ShaderProgramDesc;
class SpriteSheet;
class StructuredBuffer;
class Texture;
class Texture1D;
class Texture2D;
class Texture3D;
class VertexBuffer;

struct screenshot_job_t {
public:
    screenshot_job_t()
    : saveLocation{FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineData) / std::filesystem::path{"Screenshots"}} {
        namespace FS = std::filesystem;
        FileUtils::CreateFolders(saveLocation);
        const std::filesystem::path folder = saveLocation;
        const auto screenshot_count = FileUtils::CountFilesInFolders(folder);
        const auto filepath = folder / FS::path{"Screenshot_" + std::to_string(screenshot_count + 1) + ".png"};
        saveLocation = filepath;
    }
    screenshot_job_t(std::filesystem::path location)
    : saveLocation{location} {
        /* DO NOTHING */
    }
    screenshot_job_t(std::string location)
    : saveLocation{location} {
        /* DO NOTHING */
    }
    operator bool() const noexcept {
        return !saveLocation.empty() && std::filesystem::exists(saveLocation.parent_path());
    }
    operator std::string() const noexcept {
        return saveLocation.string();
    }
    operator std::filesystem::path() const noexcept {
        return saveLocation;
    }
    void clear() noexcept {
        saveLocation.clear();
    }

private:
    std::filesystem::path saveLocation{};
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
    Renderer& renderer;
    std::size_t uavCount = 0;
    std::vector<Texture*> uavTextures{};
    Shader* computeShader = nullptr;
    unsigned int threadGroupCountX = 1;
    unsigned int threadGroupCountY = 1;
    unsigned int threadGroupCountZ = 1;
    ComputeJob() = default;
    ComputeJob(Renderer& renderer,
               std::size_t uavCount,
               const std::vector<Texture*>& uavTextures,
               Shader* computeShader,
               unsigned int threadGroupCountX,
               unsigned int threadGroupCountY,
               unsigned int threadGroupCountZ) noexcept;
    ~ComputeJob() noexcept;
};

class Renderer : public EngineSubsystem, public IRendererService {
public:
    Renderer() noexcept;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer() noexcept;

    /************************************/
    /* BEGIN ENGINE SUBSYSTEM INTERFACE */
    /************************************/

    [[nodiscard]] bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;
    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    /**********************************/
    /* END ENGINE SUBSYSTEM INTERFACE */
    /**********************************/
    
    /***************************/
    /* BEGIN SERVICE INTERFACE */
    /***************************/

    void BeginRender(Texture* color_target = nullptr, const Rgba& clear_color = Rgba::Black, Texture* depthstencil_target = nullptr) noexcept override;
    void BeginRenderToBackbuffer(const Rgba& clear_color = Rgba::Black) noexcept override;
    void BeginHUDRender(Camera2D& ui_camera, const Vector2& camera_position, float window_height) noexcept override;

    [[nodiscard]] TimeUtils::FPSeconds GetGameFrameTime() const noexcept override;
    [[nodiscard]] TimeUtils::FPSeconds GetSystemFrameTime() const noexcept override;
    [[nodiscard]] TimeUtils::FPSeconds GetGameTime() const noexcept override;
    [[nodiscard]] TimeUtils::FPSeconds GetSystemTime() const noexcept override;

    void SetFullscreen(bool isFullscreen) noexcept override;
    void SetFullscreenMode() noexcept override;
    void SetWindowedMode() noexcept override;
    void SetWindowTitle(const std::string& newTitle) noexcept override;
    [[nodiscard]] std::string GetWindowTitle() const noexcept override;

    void SetWindowIcon(void* iconResource) noexcept;

    [[nodiscard]] std::unique_ptr<VertexBuffer> CreateVertexBuffer(const VertexBuffer::buffer_t& vbo) const noexcept override;
    [[nodiscard]] std::unique_ptr<VertexBufferInstanced> CreateVertexBufferInstanced(const VertexBufferInstanced::buffer_t& vbio) const noexcept override;
    [[nodiscard]] std::unique_ptr<IndexBuffer> CreateIndexBuffer(const IndexBuffer::buffer_t& ibo) const noexcept override;
    [[nodiscard]] std::unique_ptr<ConstantBuffer> CreateConstantBuffer(void* const& buffer, const std::size_t& buffer_size) const noexcept override;
    [[nodiscard]] std::unique_ptr<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::buffer_t& sbo, std::size_t element_size, std::size_t element_count) const noexcept override;

    [[nodiscard]] Texture* CreateOrGetTexture(const std::filesystem::path& filepath, const IntVector3& dimensions) noexcept override;
    void RegisterTexturesFromFolder(std::filesystem::path folderpath, bool recursive = false) noexcept override;
    [[nodiscard]] bool RegisterTexture(const std::string& name, std::unique_ptr<Texture> texture) noexcept override;
    void SetTexture(Texture* texture, unsigned int registerIndex = 0) noexcept override;

    [[nodiscard]] Texture* GetTexture(const std::string& nameOrFile) noexcept override;

    //TODO: [[nodiscard]] std::unique_ptr<Texture> CreateDepthStencil(const RHIDevice& owner, uint32_t width, uint32_t height) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> CreateDepthStencil(const RHIDevice& owner, const IntVector2& dimensions) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> CreateRenderableDepthStencil(const RHIDevice& owner, const IntVector2& dimensions) noexcept override;

    [[nodiscard]] Texture* GetDefaultDepthStencil() const noexcept override;
    void SetDepthStencilState(DepthStencilState* depthstencil) noexcept override;
    [[nodiscard]] DepthStencilState* GetDepthStencilState(const std::string& name) noexcept override;
    void CreateAndRegisterDepthStencilStateFromDepthStencilDescription(const std::string& name, const DepthStencilDesc& desc) noexcept override;
    void EnableDepth(bool isDepthEnabled) noexcept override;
    void EnableDepth() noexcept override;
    void DisableDepth() noexcept override;
    void EnableDepthWrite(bool isDepthWriteEnabled) noexcept override;
    void EnableDepthWrite() noexcept override;
    void DisableDepthWrite() noexcept override;

    void SetDepthComparison(ComparisonFunction cf) noexcept override;
    [[nodiscard]] ComparisonFunction GetDepthComparison() const noexcept override;
    void SetStencilFrontComparison(ComparisonFunction cf) noexcept override;
    void SetStencilBackComparison(ComparisonFunction cf) noexcept override;
    void EnableStencilWrite() noexcept override;
    void DisableStencilWrite() noexcept override;

    [[nodiscard]] Texture* Create1DTexture(std::filesystem::path filepath, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create1DTextureFromMemory(const unsigned char* data, unsigned int width = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create1DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;
    [[nodiscard]] Texture* Create2DTexture(std::filesystem::path filepath, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create2DTextureFromMemory(const unsigned char* data, unsigned int width = 1, unsigned int height = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) const noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create2DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width = 1, unsigned int height = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) const noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create2DTextureFromMemory(const void* data, std::size_t elementSize, unsigned int width = 1, unsigned int height = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) const noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create2DTextureArrayFromMemory(const unsigned char* data, unsigned int width = 1, unsigned int height = 1, unsigned int depth = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;
    [[nodiscard]] Texture* Create3DTexture(std::filesystem::path filepath, const IntVector3& dimensions, const BufferUsage& bufferUsage, const BufferBindUsage& bindUsage, const ImageFormat& imageFormat) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create3DTextureFromMemory(const unsigned char* data, unsigned int width = 1, unsigned int height = 1, unsigned int depth = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;
    [[nodiscard]] std::unique_ptr<Texture> Create3DTextureFromMemory(const std::vector<Rgba>& data, unsigned int width = 1, unsigned int height = 1, unsigned int depth = 1, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;
    [[nodiscard]] Texture* CreateTexture(std::filesystem::path filepath, const IntVector3& dimensions, const BufferUsage& bufferUsage = BufferUsage::Static, const BufferBindUsage& bindUsage = BufferBindUsage::Shader_Resource, const ImageFormat& imageFormat = ImageFormat::R8G8B8A8_UNorm) noexcept override;

    [[nodiscard]] std::shared_ptr<SpriteSheet> CreateSpriteSheet(const std::filesystem::path& filepath, unsigned int width = 1, unsigned int height = 1) noexcept override;
    [[nodiscard]] std::shared_ptr<SpriteSheet> CreateSpriteSheet(const XMLElement& elem) noexcept override;
    [[nodiscard]] std::unique_ptr<AnimatedSprite> CreateAnimatedSprite(std::filesystem::path filepath) noexcept override;
    [[nodiscard]] std::unique_ptr<AnimatedSprite> CreateAnimatedSprite(const AnimatedSpriteDesc& desc) noexcept override;
    [[nodiscard]] std::unique_ptr<AnimatedSprite> CreateAnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const IntVector2& startSpriteCoords = IntVector2::Zero) noexcept override;
    [[nodiscard]] std::unique_ptr<AnimatedSprite> CreateAnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const XMLElement& elem) noexcept override;
    [[nodiscard]] std::unique_ptr<AnimatedSprite> CreateAnimatedSprite(const XMLElement& elem) noexcept override;

    void ClearRenderTargets(const RenderTargetType& rtt) noexcept override;
    void SetRenderTarget(FrameBuffer& frameBuffer) noexcept override;
    void SetRenderTarget(Texture* color_target = nullptr, Texture* depthstencil_target = nullptr) noexcept override;
    void SetRenderTargetsToBackBuffer() noexcept override;
    [[nodiscard]] ViewportDesc GetCurrentViewport() const override;
    [[nodiscard]] float GetCurrentViewportAspectRatio() const override;
    [[nodiscard]] ViewportDesc GetViewport(std::size_t index) const noexcept override;
    [[nodiscard]] unsigned int GetViewportCount() const noexcept override;
    [[nodiscard]] std::vector<ViewportDesc> GetAllViewports() const noexcept override;
    void SetViewport(const ViewportDesc& desc) noexcept override;
    void SetViewport(float x, float y, float width, float height) noexcept override;
    void SetViewport(const AABB2& viewport) noexcept override;
    void SetViewportAndScissor(float x, float y, float width, float height) noexcept override;
    void SetViewportAndScissor(const AABB2& viewport_and_scissor) noexcept override;
    void SetViewports(const std::vector<AABB3>& viewports) noexcept override;
    void SetViewportAsPercent(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 1.0f) noexcept override;
    void SetViewportAndScissorAsPercent(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 1.0f) noexcept override;

    void EnableScissorTest() override;
    void DisableScissorTest() override;

    void SetScissor(unsigned int x, unsigned int y, unsigned int width, unsigned int height) noexcept override;
    void SetScissor(const AABB2& scissor) noexcept override;
    void SetScissorAsPercent(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 1.0f) noexcept override;
    void SetScissorAndViewport(float x, float y, float width, float height) noexcept override;
    void SetScissorAndViewport(const AABB2& scissor_and_viewport) noexcept override;
    void SetScissorAndViewportAsPercent(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 1.0f) noexcept override;
    void SetScissors(const std::vector<AABB2>& scissors) noexcept override;

    void ClearColor(const Rgba& color) noexcept override;
    void ClearTargetColor(Texture* target, const Rgba& color) noexcept override;
    void ClearTargetDepthStencilBuffer(Texture* target, bool depth = true, bool stencil = true, float depthValue = 1.0f, unsigned char stencilValue = 0) noexcept override;
    void ClearDepthStencilBuffer() noexcept override;
    void Present() noexcept override;

    void DrawPoint(const Vertex3D& point) noexcept override;
    void DrawPoint(const Vector3& point, const Rgba& color = Rgba::White, const Vector2& tex_coords = Vector2::Zero) noexcept override;
    void DrawFrustum(const Frustum& frustum, const Rgba& color = Rgba::Yellow, const Vector2& tex_coords = Vector2::Zero) noexcept override;
    void DrawWorldGridXZ(float radius = 500.0f, float major_gridsize = 20.0f, float minor_gridsize = 5.0f, const Rgba& major_color = Rgba::White, const Rgba& minor_color = Rgba::DarkGray) noexcept override;
    void DrawWorldGridXY(float radius = 500.0f, float major_gridsize = 20.0f, float minor_gridsize = 5.0f, const Rgba& major_color = Rgba::White, const Rgba& minor_color = Rgba::DarkGray) noexcept override;
    void DrawWorldGrid2D(const IntVector2& dimensions, const Rgba& color = Rgba::White) noexcept override;
    void DrawWorldGrid2D(int width, int height, const Rgba& color = Rgba::White) noexcept override;

    void DrawAxes(float maxlength = 1000.0f, bool disable_unit_depth = true) noexcept override;
    void DrawDebugSphere(const Rgba& color) noexcept override;

    void Draw(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo) noexcept override;
    void Draw(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, std::size_t vertex_count) noexcept override;
    void DrawIndexed(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<unsigned int>& ibo) noexcept override;
    void DrawIndexed(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<unsigned int>& ibo, std::size_t index_count, std::size_t startVertex = 0, std::size_t baseVertexLocation = 0) noexcept override;
    void DrawInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount) noexcept override;
    void DrawInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, std::size_t instanceCount, std::size_t vertexCount) noexcept override;
    void DrawIndexedInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, const std::vector<unsigned int>& ibo, std::size_t instanceCount) noexcept override;
    void DrawIndexedInstanced(const PrimitiveType& topology, const std::vector<Vertex3D>& vbo, const std::vector<Vertex3DInstanced>& vbio, const std::vector<unsigned int>& ibo, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept override;

    void SetLightingEyePosition(const Vector3& position) noexcept override;
    void SetAmbientLight(const Rgba& ambient) noexcept override;
    void SetAmbientLight(const Rgba& color, float intensity) noexcept override;
    void SetSpecGlossEmitFactors(Material* mat) noexcept override;
    void SetUseVertexNormalsForLighting(bool value) noexcept override;

    [[nodiscard]] const light_t& GetLight(unsigned int index) const noexcept override;
    void SetPointLight(unsigned int index, const PointLightDesc& desc) noexcept override;
    void SetDirectionalLight(unsigned int index, const DirectionalLightDesc& desc) noexcept override;
    void SetSpotlight(unsigned int index, const SpotLightDesc& desc) noexcept override;

    [[nodiscard]] RHIDeviceContext* GetDeviceContext() const noexcept override;
    [[nodiscard]] RHIDevice* GetDevice() const noexcept override;
    [[nodiscard]] RHIOutput* GetOutput() const noexcept override;
    [[nodiscard]] RHIInstance* GetInstance() const noexcept override;

    [[nodiscard]] ShaderProgram* GetShaderProgram(const std::string& nameOrFile) noexcept override;

    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateShaderProgramFromCsoFile(std::filesystem::path filepath, const PipelineStage& target) const noexcept override;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateShaderProgramFromDesc(ShaderProgramDesc&& desc) const noexcept override;

    void CreateAndRegisterShaderProgramFromCsoFile(std::filesystem::path filepath, const PipelineStage& target) noexcept override;
    void CreateAndRegisterRasterStateFromRasterDescription(const std::string& name, const RasterDesc& desc) noexcept override;
    void SetRasterState(RasterState* raster) noexcept override;
    void SetRasterState(FillMode fillmode, CullMode cullmode) noexcept override;
    [[nodiscard]] RasterState* GetRasterState(const std::string& name) noexcept override;

    void SetWireframeRaster(CullMode cullmode = CullMode::Back) noexcept override;
    void SetSolidRaster(CullMode cullmode = CullMode::Back) noexcept override;

    void CreateAndRegisterSamplerFromSamplerDescription(const std::string& name, const SamplerDesc& desc) noexcept override;
    [[nodiscard]] Sampler* GetSampler(const std::string& name) noexcept override;
    void SetSampler(Sampler* sampler) noexcept override;

    void SetVSync(bool value) noexcept override;

    [[nodiscard]] std::unique_ptr<Material> CreateMaterialFromFont(KerningFont* font) noexcept override;
    [[nodiscard]] bool RegisterMaterial(std::filesystem::path filepath) noexcept override;
    void RegisterMaterial(std::unique_ptr<Material> mat) noexcept override;
    void RegisterMaterialsFromFolder(std::filesystem::path folderpath, bool recursive = false) noexcept override;
    void ReloadMaterials() noexcept override;

    [[nodiscard]] Material* GetMaterial(const std::string& nameOrFile) noexcept override;
    void SetMaterial(Material* material) noexcept override;
    void SetMaterial(const std::string& nameOrFile) noexcept override;
    void ResetMaterial() noexcept override;

    [[nodiscard]] bool IsTextureLoaded(const std::string& nameOrFile) const noexcept override;
    [[nodiscard]] bool IsTextureNotLoaded(const std::string& nameOrFile) const noexcept override;

    [[nodiscard]] bool RegisterShader(std::filesystem::path filepath) noexcept override;
    void RegisterShader(std::unique_ptr<Shader> shader) noexcept override;

    [[nodiscard]] Shader* GetShader(const std::string& nameOrFile) noexcept override;
    [[nodiscard]] std::string GetShaderName(const std::filesystem::path filepath) noexcept override;

    void SetComputeShader(Shader* shader) noexcept override;
    void DispatchComputeJob(const ComputeJob& job) noexcept override;

    [[nodiscard]] KerningFont* GetFont(const std::string& nameOrFile) noexcept override;

    void RegisterFont(std::unique_ptr<KerningFont> font) noexcept override;
    [[nodiscard]] bool RegisterFont(std::filesystem::path filepath) noexcept override;
    void RegisterFontsFromFolder(std::filesystem::path folderpath, bool recursive = false) noexcept override;

    void UpdateGameTime(TimeUtils::FPSeconds deltaSeconds) noexcept override;

    void ResetModelViewProjection() noexcept override;
    void AppendModelMatrix(const Matrix4& modelMatrix) noexcept override;
    void SetModelMatrix(const Matrix4& mat = Matrix4::I) noexcept override;
    void SetViewMatrix(const Matrix4& mat = Matrix4::I) noexcept override;
    void SetProjectionMatrix(const Matrix4& mat = Matrix4::I) noexcept override;
    void SetOrthoProjection(const Vector2& leftBottom, const Vector2& rightTop, const Vector2& near_far) noexcept override;
    void SetOrthoProjection(const Vector2& dimensions, const Vector2& origin, float nearz, float farz) noexcept override;
    void SetOrthoProjectionFromViewHeight(float viewHeight, float aspectRatio, float nearz, float farz) noexcept override;
    void SetOrthoProjectionFromViewWidth(float viewWidth, float aspectRatio, float nearz, float farz) noexcept override;
    void SetOrthoProjectionFromCamera(const Camera3D& camera) noexcept override;
    void SetPerspectiveProjection(const Vector2& vfovDegrees_aspect, const Vector2& nz_fz) noexcept override;
    void SetPerspectiveProjectionFromCamera(const Camera3D& camera) noexcept override;
    void SetCamera(const Camera3D& camera) noexcept override;
    void SetCamera(const Camera2D& camera) noexcept override;
    [[nodiscard]] Camera3D GetCamera() const noexcept override;

    [[nodiscard]] Vector2 ConvertWorldToScreenCoords(const Vector3& worldCoords) const noexcept override;
    [[nodiscard]] Vector2 ConvertWorldToScreenCoords(const Vector2& worldCoords) const noexcept override;
    [[nodiscard]] Vector2 ConvertWorldToScreenCoords(const Camera3D& camera, const Vector3& worldCoords) const noexcept override;
    [[nodiscard]] Vector2 ConvertWorldToScreenCoords(const Camera2D& camera, const Vector2& worldCoords) const noexcept override;
    [[nodiscard]] Vector3 ConvertScreenToWorldCoords(const Vector2& mouseCoords) const noexcept override;
    [[nodiscard]] Vector3 ConvertScreenToWorldCoords(const Camera3D& camera, const Vector2& mouseCoords) const noexcept override;
    [[nodiscard]] Vector2 ConvertScreenToWorldCoords(const Camera2D& camera, const Vector2& mouseCoords) const noexcept override;

    [[nodiscard]] Vector3 ConvertScreenToNdcCoords(const Camera3D& camera, const Vector2& mouseCoords) const noexcept override;
    [[nodiscard]] Vector2 ConvertScreenToNdcCoords(const Camera2D& camera, const Vector2& mouseCoords) const noexcept override;
    [[nodiscard]] Vector3 ConvertScreenToNdcCoords(const Vector2& mouseCoords) const noexcept override;

    void SetConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept override;
    void SetStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept override;
    void SetComputeConstantBuffer(unsigned int index, ConstantBuffer* buffer) noexcept override;
    void SetComputeStructuredBuffer(unsigned int index, StructuredBuffer* buffer) noexcept override;

    void DrawCube(const Vector3& position = Vector3::Zero, const Vector3& halfExtents = Vector3::One * 0.5f, const Rgba& color = Rgba::White) override;
    void DrawQuad(const Vector3& position = Vector3::Zero, const Vector3& halfExtents = Vector3::XY_Axis * 0.5f, const Rgba& color = Rgba::White, const Vector4& texCoords = Vector4::ZW_Axis, const Vector3& normalFront = Vector3::Z_Axis, const Vector3& worldUp = Vector3::Y_Axis) noexcept override;
    void DrawQuad(const Rgba& frontColor, const Rgba& backColor, const Vector3& position = Vector3::Zero, const Vector3& halfExtents = Vector3::XY_Axis * 0.5f, const Vector4& texCoords = Vector4::ZW_Axis, const Vector3& normalFront = Vector3::Z_Axis, const Vector3& worldUp = Vector3::Y_Axis) noexcept override;
    void DrawPoint2D(float pointX, float pointY, const Rgba& color = Rgba::White) noexcept override;
    void DrawPoint2D(const Vector2& point, const Rgba& color = Rgba::White) noexcept override;
    void DrawLine2D(float startX, float startY, float endX, float endY, const Rgba& color = Rgba::White, float thickness = 0.0f) noexcept override;
    void DrawLine2D(const Vector2& start, const Vector2& end, const Rgba& color = Rgba::White, float thickness = 0.0f) noexcept override;
    void DrawQuad2D(float left, float bottom, float right, float top, const Rgba& color = Rgba::White, const Vector4& texCoords = Vector4::ZW_Axis) noexcept override;
    void DrawQuad2D(const Vector2& position = Vector2::Zero, const Vector2& halfExtents = Vector2(0.5f, 0.5f), const Rgba& color = Rgba::White, const Vector4& texCoords = Vector4::ZW_Axis) noexcept override;
    void DrawQuad2D(const Matrix4& transform, const Rgba& color = Rgba::White, const Vector4& texCoords = Vector4::ZW_Axis) noexcept override;
    void DrawQuad2D(const Rgba& color) noexcept override;
    void DrawQuad2D(const Vector4& texCoords) noexcept override;
    void DrawQuad2D(const Rgba& color, const Vector4& texCoords) noexcept override;
    void DrawCircle2D(float centerX, float centerY, float radius, const Rgba& color = Rgba::White) noexcept override;
    void DrawCircle2D(const Matrix4& transform, float thickness, const Rgba& color = Rgba::White) noexcept override;
    void DrawCircle2D(const Vector2& center, float radius, const Rgba& color = Rgba::White) noexcept override;
    void DrawCircle2D(const Disc2& circle, const Rgba& color = Rgba::White) noexcept override;
    void DrawFilledCircle2D(const Disc2& circle, const Rgba& color = Rgba::White) noexcept override;
    void DrawFilledCircle2D(const Vector2& center, float radius, const Rgba& color = Rgba::White) noexcept override;
    void DrawAABB2(const AABB2& bounds, const Rgba& edgeColor, const Rgba& fillColor, const Vector2& edgeHalfExtents = Vector2::Zero) noexcept override;
    void DrawAABB2(const Rgba& edgeColor, const Rgba& fillColor) noexcept override;
    void DrawOBB2(float orientationDegrees, const Rgba& edgeColor, const Rgba& fillColor = Rgba::NoAlpha) noexcept override;
    void DrawOBB2(const OBB2& obb, const Rgba& edgeColor, const Rgba& fillColor = Rgba::NoAlpha, const Vector2& edgeHalfExtents = Vector2::Zero) noexcept override;
    void DrawPolygon2D(float centerX, float centerY, float radius, std::size_t numSides = 3, const Rgba& color = Rgba::White) noexcept override;
    void DrawPolygon2D(const Vector2& center, float radius, std::size_t numSides = 3, const Rgba& color = Rgba::White) noexcept override;
    void DrawPolygon2D(const Polygon2& polygon, const Rgba& color = Rgba::White) override;
    void DrawX2D(const Vector2& position = Vector2::Zero, const Vector2& half_extents = Vector2(0.5f, 0.5f), const Rgba& color = Rgba::White) noexcept override;
    void DrawX2D(const Rgba& color) noexcept override;
    void DrawTextLine(const KerningFont* font, const std::string& text, const Rgba& color = Rgba::White) noexcept override;
    void DrawTextLine(const Matrix4& transform, const KerningFont* font, const std::string& text, const Rgba& color = Rgba::White) noexcept override;
    void DrawMultilineText(KerningFont* font, const std::string& text, const Rgba& color = Rgba::White) noexcept override;
    void AppendMultiLineTextBuffer(KerningFont* font, const std::string& text, const Vector2& start_position, const Rgba& color, std::vector<Vertex3D>& vbo, std::vector<unsigned int>& ibo) noexcept override;

    void CopyTexture(const Texture* src, Texture* dst) const noexcept override;
    void ResizeBuffers() noexcept override;
    void ClearState() noexcept override;

    void RequestScreenShot() override;
    void RequestScreenShot(std::filesystem::path saveLocation) override;

    [[nodiscard]] constexpr unsigned int GetMatrixBufferIndex() const noexcept override;
    [[nodiscard]] constexpr unsigned int GetTimeBufferIndex() const noexcept override;
    [[nodiscard]] constexpr unsigned int GetLightingBufferIndex() const noexcept override;
    [[nodiscard]] constexpr unsigned int GetConstantBufferStartIndex() const noexcept override;
    [[nodiscard]] constexpr unsigned int GetStructuredBufferStartIndex() const noexcept override;
    [[nodiscard]] constexpr unsigned int GetMaxLightCount() const noexcept override;

    /*************************/
    /* END SERVICE INTERFACE */
    /*************************/

protected:
private:

    [[nodiscard]] Image GetBackbufferAsImage() const noexcept;

    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* _shader_program) noexcept;
    [[nodiscard]] static std::vector<std::unique_ptr<ConstantBuffer>> CreateComputeConstantBuffersFromShaderProgram(RHIDevice& device, const ShaderProgram* shaderProgram) noexcept;

    void SetWinProc(const std::function<bool(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>& windowProcedure) noexcept;

    void UpdateSystemTime(TimeUtils::FPSeconds deltaSeconds) noexcept;
    [[nodiscard]] bool RegisterTexture(const std::filesystem::path& filepath) noexcept;
    void RegisterShaderProgram(const std::string& name, std::unique_ptr<ShaderProgram> sp) noexcept override;
    void RegisterShader(const std::string& name, std::unique_ptr<Shader> shader) noexcept override;
    void RegisterMaterial(const std::string& name, std::unique_ptr<Material> mat) noexcept override;
    void RegisterRasterState(const std::string& name, std::unique_ptr<RasterState> raster) noexcept override;
    void RegisterDepthStencilState(const std::string& name, std::unique_ptr<DepthStencilState> depthstencil) noexcept override;
    void RegisterSampler(const std::string& name, std::unique_ptr<Sampler> sampler) noexcept override;
    void RegisterFont(const std::string& name, std::unique_ptr<KerningFont> font) noexcept override;

    void CreateDefaultConstantBuffers() noexcept;
    void CreateWorkingVboAndIbo() noexcept;
    void UpdateVbo(const VertexBuffer::buffer_t& vbo) noexcept;
    void UpdateVbio(const VertexBufferInstanced::buffer_t& vbio) noexcept;
    void UpdateIbo(const IndexBuffer::buffer_t& ibo) noexcept;

    void Draw(const PrimitiveType& topology, VertexBuffer* vbo, std::size_t vertex_count) noexcept;
    void DrawInstanced(const PrimitiveType& topology, VertexBuffer* vbo, VertexBufferInstanced* vbio, std::size_t vertexPerInstanceCount, std::size_t instanceCount, std::size_t startVertexLocation, std::size_t startInstanceLocation) noexcept;
    void DrawIndexed(const PrimitiveType& topology, VertexBuffer* vbo, IndexBuffer* ibo, std::size_t index_count, std::size_t startVertex = 0, std::size_t baseVertexLocation = 0) noexcept;
    void DrawIndexedInstanced(const PrimitiveType& topology, VertexBuffer* vbo, VertexBufferInstanced* vbio, IndexBuffer* ibo, std::size_t indexPerInstanceCount, std::size_t instanceCount, std::size_t startIndexLocation, std::size_t baseVertexLocation, std::size_t startInstanceLocation) noexcept;

    [[nodiscard]] std::shared_ptr<SpriteSheet> CreateSpriteSheet(Texture* texture, int tilesWide, int tilesHigh) noexcept;

    void SetLightAtIndex(unsigned int index, const light_t& light) noexcept;
    void SetPointLight(unsigned int index, const light_t& light) noexcept;
    void SetDirectionalLight(unsigned int index, const light_t& light) noexcept;
    void SetSpotlight(unsigned int index, const light_t& light) noexcept;

    void CreateAndRegisterDefaultTextures() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateInvalidTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultDiffuseTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultNormalTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultDisplacementTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultSpecularTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultOcclusionTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultEmissiveTexture() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultFullscreenTexture() noexcept;

    void CreateDefaultColorTextures() noexcept;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultColorTexture(const Rgba& color) noexcept;

    void CreateAndRegisterDefaultShaderPrograms() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultShaderProgram() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultUnlitShaderProgram() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultNormalShaderProgram() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultNormalMapShaderProgram() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultFontShaderProgram() noexcept;
    [[nodiscard]] std::unique_ptr<ShaderProgram> CreateDefaultCircle2DShaderProgram() noexcept;

    [[nodiscard]] void CreateAndRegisterDefaultShaders() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultUnlitShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefault2DShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultCircle2DShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultNormalShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultNormalMapShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultInvalidShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateDefaultFontShader() noexcept;
    [[nodiscard]] std::unique_ptr<Shader> CreateShaderFromFile(std::filesystem::path filepath) noexcept;

    void CreateAndRegisterDefaultMaterials() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultUnlitMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefault2DMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultNormalMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultNormalMapMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultInvalidMaterial() noexcept;
    [[nodiscard]] std::unique_ptr<Material> CreateDefaultCircle2DMaterial() noexcept;

    void CreateAndRegisterDefaultEngineFonts() noexcept;

    void CreateAndRegisterDefaultSamplers() noexcept;
    [[nodiscard]] std::unique_ptr<Sampler> CreateDefaultSampler() noexcept;
    [[nodiscard]] std::unique_ptr<Sampler> CreateLinearSampler() noexcept;
    [[nodiscard]] std::unique_ptr<Sampler> CreatePointSampler() noexcept;
    [[nodiscard]] std::unique_ptr<Sampler> CreateInvalidSampler() noexcept;

    void CreateAndRegisterDefaultRasterStates() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateDefaultRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateScissorEnableRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateScissorDisableRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateWireframeRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateSolidRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateWireframeNoCullingRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateSolidNoCullingRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateWireframeFrontCullingRaster() noexcept;
    [[nodiscard]] std::unique_ptr<RasterState> CreateSolidFrontCullingRaster() noexcept;

    void CreateAndRegisterDefaultDepthStencilStates() noexcept;
    [[nodiscard]] std::unique_ptr<DepthStencilState> CreateDefaultDepthStencilState() noexcept;
    [[nodiscard]] std::unique_ptr<DepthStencilState> CreateDisabledDepth() noexcept;
    [[nodiscard]] std::unique_ptr<DepthStencilState> CreateEnabledDepth() noexcept;
    [[nodiscard]] std::unique_ptr<DepthStencilState> CreateDisabledStencil() noexcept;
    [[nodiscard]] std::unique_ptr<DepthStencilState> CreateEnabledStencil() noexcept;

    void CreateAndRegisterDefaultFonts() noexcept;
    [[nodiscard]] std::unique_ptr<KerningFont> CreateDefaultSystem32Font() noexcept;

    void UnbindAllResourcesAndBuffers() noexcept;
    void UnbindAllResources() noexcept;
    void UnbindAllBuffers() noexcept;
    void UnbindAllShaderResources() noexcept;
    void UnbindAllConstantBuffers() noexcept;
    void UnbindWorkingVboAndIbo() noexcept;
    void UnbindComputeShaderResources() noexcept;
    void UnbindComputeConstantBuffers() noexcept;

    void LogAvailableDisplays() noexcept;

    [[nodiscard]] Vector2 GetScreenCenter() const noexcept;
    [[nodiscard]] Vector2 GetWindowCenter() const noexcept;
    [[nodiscard]] Vector2 GetWindowCenter(const Window& window) const noexcept;

    void FulfillScreenshotRequest() noexcept;

    Camera3D _camera{};
    matrix_buffer_t _matrix_data{};
    time_buffer_t _time_data{};
    lighting_buffer_t _lighting_data{};
    std::size_t _current_vbo_size = 0;
    std::size_t _current_vbio_size = 0;
    std::size_t _current_ibo_size = 0;
    RHIInstance* _rhi_instance = nullptr;
    std::unique_ptr<RHIDevice> _rhi_device = nullptr;
    std::unique_ptr<RHIDeviceContext> _rhi_context = nullptr;
    std::unique_ptr<RHIOutput> _rhi_output = nullptr;
    Texture* _current_target = nullptr;
    Texture* _current_depthstencil = nullptr;
    Texture* _default_depthstencil = nullptr;
    DepthStencilState* _current_depthstencil_state = nullptr;
    RasterState* _current_raster_state = nullptr;
    Sampler* _current_sampler = nullptr;
    Material* _current_material = nullptr;
    IntVector2 _window_dimensions = IntVector2::Zero;
    RHIOutputMode _current_outputMode = RHIOutputMode::Windowed;
    std::unique_ptr<FrameBuffer> _backbuffer{};
    std::unique_ptr<VertexBuffer> _temp_vbo = nullptr;
    std::unique_ptr<VertexBufferInstanced> _temp_vbio = nullptr;
    std::unique_ptr<IndexBuffer> _temp_ibo = nullptr;
    std::unique_ptr<ConstantBuffer> _matrix_cb = nullptr;
    std::unique_ptr<ConstantBuffer> _time_cb = nullptr;
    std::unique_ptr<ConstantBuffer> _lighting_cb = nullptr;
    std::vector<std::pair<std::string, std::unique_ptr<Texture>>> _textures{};
    std::vector<std::pair<std::string, std::unique_ptr<ShaderProgram>>> _shader_programs;
    std::vector<std::pair<std::string, std::unique_ptr<Shader>>> _shaders;
    std::vector<std::pair<std::string, std::unique_ptr<Material>>> _materials;
    std::vector<std::pair<std::string, std::unique_ptr<Sampler>>> _samplers;
    std::vector<std::pair<std::string, std::unique_ptr<RasterState>>> _rasters;
    std::vector<std::pair<std::string, std::unique_ptr<DepthStencilState>>> _depthstencils;
    std::vector<std::pair<std::string, std::unique_ptr<KerningFont>>> _fonts;
    mutable std::mutex _cs{};
    screenshot_job_t _screenshot{};
    std::filesystem::path _last_screenshot_location{};
    bool _vsync = false;
    bool _materials_need_updating = true;
    bool _sizemove_in_progress = false;
    bool _is_minimized = false;

    friend class Shader;
    friend class AnimatedSprite;
    friend class SpriteSheet;
};
