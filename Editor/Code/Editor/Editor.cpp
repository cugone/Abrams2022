#include "Editor/Editor.hpp"

#include "Engine/Core/Image.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Platform/DirectX/DirectX11FrameBuffer.hpp"
#include "Engine/Platform/PlatformUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"


#include "Engine/Scene/Scene.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IInputService.hpp"

#include "Engine/Core/ThreadUtils.hpp"
#include "Engine/Math/Ray3.hpp"

#include "Engine/Input/InputSystem.hpp"
#include "Engine/UI/UISystem.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <numeric>
#include <sstream>
#include <thread>

struct hit_record {
    float t{};
    Vector3 p{};
    Vector3 normal{};
};

class hitable {
public:
    virtual bool hit(const Ray3& r, float t_min, float t_max, hit_record& record) const = 0;
    virtual ~hitable() noexcept = default;
};

class sphere : public hitable {
public:
    sphere() = default;
    virtual ~sphere() noexcept = default;
    sphere(Vector3 center, float r)
    : m_center(center)
    , m_radius(r){};
    virtual bool hit(const Ray3& r, float tmin, float tmax, hit_record& record) const override;
    Vector3 m_center{};
    float m_radius{0.0f};
};

bool sphere::hit(const Ray3& r, float t_min, float t_max, hit_record& record) const {
    Vector3 oc = r.position - m_center;
    float a = MathUtils::DotProduct(r.direction, r.direction);
    float b = MathUtils::DotProduct(oc, r.direction);
    float c = MathUtils::DotProduct(oc, oc) - m_radius * m_radius;
    float discriminant = b * b - a * c;
    if(discriminant > 0) {
        float temp = (-b - std::sqrt(b * b - a * c)) / a;
        if(temp < t_max && temp > t_min) {
            record.t = temp;
            record.p = r.Interpolate(record.t);
            record.normal = (record.p - m_center) / m_radius;
            return true;
        }
        temp = (-b + std::sqrt(b * b - a * c)) / a;
        if(temp < t_max && temp > t_min) {
            record.t = temp;
            record.p = r.Interpolate(record.t);
            record.normal = (record.p - m_center) / m_radius;
            return true;
        }
    }
    return false;
}

class hitable_list : public hitable {
public:
    hitable_list() = default;
    hitable_list(hitable** l, int n) {
        list = l;
        list_size = n;
    }
    virtual ~hitable_list() noexcept = default;
    virtual bool hit(const Ray3& r, float tmin, float tmax, hit_record& record) const override;
    hitable** list;
    int list_size;
};

bool hitable_list::hit(const Ray3& r, float t_min, float t_max, hit_record& record) const {
    hit_record temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;
    for(int i = 0; i < list_size; ++i) {
        if(list[i]->hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            record = temp_rec;
        }
    }
    return hit_anything;
}

void Editor::ShowSelectedEntityComponents([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    /* DO NOTHING */
}

void Editor::HandleInput(TimeUtils::FPSeconds deltaSeconds) noexcept {
    HandleMenuKeyboardInput(deltaSeconds);
    HandleCameraInput(deltaSeconds);
}

bool Editor::IsSceneLoaded() const noexcept {
    return m_ActiveScene.get() != nullptr;
}

Editor::~Editor() noexcept {
    m_requestquit = true;
    if(m_rayTraceWorker.joinable()) {
        m_rayTraceWorker.join();
    }
    buffer.reset();
}

void Editor::Initialize() noexcept {
    MathUtils::SetRandomEngineSeed(0u);
    auto* renderer = ServiceLocator::get<IRendererService>();
    m_ContentBrowser.currentDirectory = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData);
    renderer->RegisterTexturesFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / std::filesystem::path{"Images"}, true);
    renderer->RegisterTexturesFromFolder(m_ContentBrowser.currentDirectory / std::filesystem::path{"Resources/Icons"}, true);
    m_ContentBrowser.currentDirectory = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EditorContent);
    m_ContentBrowser.UpdateContentBrowserPaths();
    buffer = FrameBuffer::Create(FrameBufferDesc{});
}

void Editor::BeginFrame() noexcept {
    ImGui::DockSpaceOverViewport();
}

void Editor::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->UpdateGameTime(deltaSeconds);
    ShowUI(deltaSeconds);
    HandleInput(deltaSeconds);
    //GenerateRayTrace();
}

void Editor::GenerateRayTrace() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    {
        const auto start = TimeUtils::Now();
        std::vector<Rgba> data(static_cast<std::size_t>(m_ViewportWidth) * m_ViewportHeight, Rgba::Magenta);
        std::generate(std::begin(data), std::end(data), Rgba::Random);
        m_raytraceTexture = std::move(renderer->Create2DTextureFromMemory(data, m_ViewportWidth, m_ViewportHeight));
        const auto end = TimeUtils::Now();
        const auto duration = end - start;
        m_lastRenderTime = duration;
    }
}

void Editor::FillRayTrace() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    {
        const auto start = TimeUtils::Now();
        std::vector<Rgba> data(static_cast<std::size_t>(m_ViewportWidth) * m_ViewportHeight, Rgba::NoAlpha);
        std::fill(std::begin(data), std::end(data), Rgba::Random());
        //for(auto& p : data) {
        //    p = Rgba::Random();
        //}
        m_raytraceTexture = std::move(renderer->Create2DTextureFromMemory(data, m_ViewportWidth, m_ViewportHeight));
        const auto end = TimeUtils::Now();
        const auto duration = end - start;
        m_lastRenderTime = duration;
    }
}

void Editor::FastFillRayTrace() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    {
        const auto clr = Rgba::Random();
        const auto start = TimeUtils::Now();
        std::vector<Rgba> data(static_cast<std::size_t>(m_ViewportWidth) * m_ViewportHeight, Rgba::NoAlpha);
        std::fill(std::begin(data), std::end(data), clr);
        m_raytraceTexture = std::move(renderer->Create2DTextureFromMemory(data.data(), m_ViewportWidth, m_ViewportHeight));
        const auto end = TimeUtils::Now();
        const auto duration = end - start;
        m_lastRenderTime = duration;
    }
}

void Editor::PSRayTrace() noexcept {
    //auto* renderer = ServiceLocator::get<IRendererService>();
    //{
    //    auto* dc = renderer->GetDeviceContext();
    //    auto* dx_dc = dc->GetDxContext();
    //    
    //}
}

float hit_sphere(const Vector3& center, float radius, const Ray3& r) {
    Vector3 oc = r.position - center;
    float a = MathUtils::DotProduct(r.direction, r.direction);
    float b = 2.0f * MathUtils::DotProduct(oc, r.direction);
    float c = MathUtils::DotProduct(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    if(discriminant < 0) {
        return -1.0f;
    }
    return (-b - std::sqrt(discriminant)) / (2.0f * a);
}

Vector3 color(const Ray3& r, hitable* world) {
    hit_record rec;
    if(world->hit(r, 0.0f, (std::numeric_limits<float>::max)(), rec)) {
        return 0.5f * Vector3(rec.normal.x + 1, rec.normal.y + 1, rec.normal.z + 1);
    } else {
        Vector3 unit_direction = r.direction.GetNormalize();
        float t = 0.5f * (unit_direction.y + 1.0f);
        return (1.0f - t) * Vector3::One + t * Vector3(0.5f, 0.7f, 1.0f);
    }
}

void Editor::raytrace_worker() noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    Vector3 lower_left_corner{-2.0f, -1.0f, -1.0f};
    Vector3 horizontal = Vector3::X_Axis * 4.0f;
    Vector3 vertical = Vector3::Y_Axis * 2.0f;
    Vector3 origin = Vector3::Zero;
    //static Vector3 prev_origin{};
    hitable* list[2];
    list[0] = new sphere(Vector3(0.0f, 0.0f, -1.0f), 0.5f);
    list[1] = new sphere(Vector3(0.0f, 100.5f, -1.0f), 100.0f);
    hitable* world = new hitable_list(list, 2);
    while(true) {
        if(m_requestquit) {
            break;
        }
        const auto start = TimeUtils::Now();
        //31 ms
        std::vector<Rgba> data(static_cast<std::size_t>(m_ViewportWidth) * m_ViewportHeight, Rgba::Green);
        for(long long y = 0LL; y < static_cast<long long>(m_ViewportHeight); ++y) {
            for(long long x = 0LL; x < static_cast<long long>(m_ViewportWidth); ++x) {
                const auto u = static_cast<float>(x) / m_ViewportWidth;
                const auto v = static_cast<float>(y) / m_ViewportHeight;
                Ray3 ray{origin, lower_left_corner + u * horizontal + v * vertical};
                Vector3 p = ray.Interpolate(2.0f);
                Vector3 col = color(ray, world);
                const auto r = static_cast<unsigned char>(static_cast<int>(255.99 * col.x));
                const auto g = static_cast<unsigned char>(static_cast<int>(255.99 * col.y));
                const auto b = static_cast<unsigned char>(static_cast<int>(255.99 * col.z));
                const auto idx = y * m_ViewportWidth + x;
                data[idx] = Rgba(r, g, b);
            }
        }
        //if(origin != prev_origin) {
        m_raytraceTexture = std::move(renderer->Create2DTextureFromMemory(data, m_ViewportWidth, m_ViewportHeight));
            //prev_origin = origin;
        //}
        const auto end = TimeUtils::Now();
        const auto duration = end - start;
        {
            m_lastRenderTime = duration;
        }
    }
    for(int i = 0; i < 2; ++i) {
        delete list[i];
        list[i] = nullptr;
    }
    delete world;
    world = nullptr;
}

void Editor::JobFillRayTrace() noexcept {
    m_requestquit = true;
    if(m_rayTraceWorker.joinable()) {
        m_rayTraceWorker.join();
    }
    m_requestquit = false;
    m_rayTraceWorker = std::thread(&Editor::raytrace_worker, this);
    ThreadUtils::SetThreadDescription(m_rayTraceWorker, L"Ray Trace Worker");
}

void Editor::RenderRayTrace() const noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetModelMatrix();
    renderer->SetMaterial("__2D");
    renderer->SetTexture(m_raytraceTexture.get());
    const auto S = Matrix4::CreateScaleMatrix(Vector2{static_cast<float>(m_ViewportWidth), static_cast<float>(m_ViewportHeight)} * 0.5f);
    const auto R = Matrix4::I;
    const auto T = Matrix4::I;
    const auto M = Matrix4::MakeSRT(S, R, T);
    renderer->DrawQuad2D(M, Rgba::White, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
}

void Editor::Render() const noexcept {

    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->BeginRender(buffer->GetTexture(), Rgba::Black, buffer->GetDepthStencil());

    renderer->SetOrthoProjectionFromViewWidth(static_cast<float>(m_ViewportWidth), m_editorCamera.GetAspectRatio(), 0.01f, 1.0f);
    renderer->SetCamera(m_editorCamera.GetCamera());

    RenderRayTrace();

    renderer->BeginRenderToBackbuffer();

}

void Editor::EndFrame() noexcept {
    /* DO NOTHING */
}

void Editor::HandleWindowResize(unsigned int newWidth, unsigned int newHeight) noexcept {
    buffer->Resize(newWidth, newHeight);
}

const GameSettings* Editor::GetSettings() const noexcept {
    return GameBase::GetSettings();
}

GameSettings* Editor::GetSettings() noexcept {
    return GameBase::GetSettings();
}

void Editor::DoFileNew() noexcept {
    /* DO NOTHING */
}

void Editor::DoFileOpen() noexcept {
    if(auto path = FileDialogs::OpenFile("Abrams Scene (*.ascene)\0*.ascene\0All Files (*.*)\0*.*\0\0"); !path.empty()) {
        
    }
}

void Editor::DoFileSaveAs() noexcept {
    if(auto path = FileDialogs::SaveFile("Abrams Scene (*.ascene)\0*.ascene\0All Files (*.*)\0*.*\0\0"); !path.empty()) {
    }
}

void Editor::DoFileSave() noexcept {
    /* DO NOTHING */
}

void Editor::ShowUI(TimeUtils::FPSeconds deltaSeconds) noexcept {
    ShowMainMenu(deltaSeconds);
    ShowWorldInspectorWindow(deltaSeconds);
    ShowSettingsWindow(deltaSeconds);
    ShowPropertiesWindow(deltaSeconds);
    ShowContentBrowserWindow(deltaSeconds);
    ShowMainViewport(deltaSeconds);
}

void Editor::ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    ImGui::BeginMainMenuBar();
    {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("New", "Ctrl+N")) {
                DoFileNew();
            }
            if(ImGui::MenuItem("Open...", "Ctrl+O")) {
                DoFileOpen();
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Minimize")) {
                auto* app = ServiceLocator::get<IAppService>();
                app->Minimize();
            }
            if(ImGui::MenuItem("Maximize")) {
                auto* app = ServiceLocator::get<IAppService>();
                app->Maximize();
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Save", "Ctrl+S", nullptr, m_ActiveScene.get())) {
                DoFileSave();
            }
            if(ImGui::MenuItem("Save As...", "", nullptr, m_ActiveScene.get())) {
                DoFileSaveAs();
            }
            if(ImGui::MenuItem("Exit")) {
                auto* app = ServiceLocator::get<IAppService>();
                app->SetIsQuitting(true);
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Debug")) {
            if(ImGui::MenuItem("Dear ImGui Demo Window", nullptr, g_theUISystem->IsImguiDemoWindowVisible(), IsDebuggerAvailable())) {
                g_theUISystem->ToggleImguiDemoWindow();
            }
            if(ImGui::MenuItem("Dear ImGui Metrics Window", nullptr, g_theUISystem->IsImguiMetricsWindowVisible(), IsDebuggerAvailable())) {
                g_theUISystem->ToggleImguiMetricsWindow();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Camera")) {
            if(ImGui::BeginMenu("Camera Speed")) {
                static int cameraTranslationMultiplier = 1;
                static int cameraSpeedMultiplier = 1;
                if(ImGui::SliderInt("Speed##CameraSpeedSlider", &cameraTranslationMultiplier, 1, 10, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput)) {
                    m_editorCamera.SetTranslationMultiplier(cameraTranslationMultiplier);
                }
                if(ImGui::InputInt("Multiplier##CameraSpeedMultipler", &cameraSpeedMultiplier, 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_NoUndoRedo)) {
                    cameraSpeedMultiplier = (std::max)(1, cameraSpeedMultiplier);
                    if(!ImGui::IsItemDeactivatedAfterEdit()) {
                        cameraSpeedMultiplier = (std::max)(1, cameraSpeedMultiplier);
                    }
                    m_editorCamera.SetSpeedMultiplier(cameraSpeedMultiplier);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if(ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted("Multiply speed by this value.");
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        //TODO: Implement custom minimize, maximize, and close buttons
        //ShowMinMaxCloseButtons();
        ImGui::EndMainMenuBar();
    }
}

void Editor::ShowMinMaxCloseButtons() noexcept {
    static float nc_button_offset = 40.0f;
    static float close_button_offset = ImGui::GetWindowWidth() - nc_button_offset;
    static float maximize_button_offset = close_button_offset - nc_button_offset;
    static float minimize_button_offset = maximize_button_offset - nc_button_offset;
    static float nc_button_sizes = 32.0f;
    ImGui::SameLine(close_button_offset);
    ImGui::PushStyleColor(ImGuiCol_Button, Rgba::NoAlpha.GetAsRawValue());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Rgba::NoAlpha.GetAsRawValue());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Rgba::Red.GetAsRawValue());

    if(ImGui::ImageButton(GetAssetTextureFromPath(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/CloseButtonAsset.png"), Vector2{nc_button_sizes, nc_button_sizes}, Vector2::Zero, Vector2::One, 0, Rgba::NoAlpha, Rgba::NoAlpha)) {
        auto* app = ServiceLocator::get<IAppService>();
        app->SetIsQuitting(true);
    }
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Rgba::LightGray.GetAsRawValue());
    ImGui::SameLine(maximize_button_offset);
    const std::filesystem::path max_or_restore_down_button_path = []() {
        auto* renderer = ServiceLocator::get<IRendererService>();
        if(const auto is_fullscreen = renderer->GetOutput()->GetWindow()->IsFullscreen(); !is_fullscreen) {
            return FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/MaximizeButtonAsset.png";
        }
        return FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/RestoreDownButtonAsset.png";
    }(); //IIIL
    if(ImGui::ImageButton(GetAssetTextureFromPath(max_or_restore_down_button_path), Vector2{nc_button_sizes, nc_button_sizes}, Vector2::Zero, Vector2::One, 0, Rgba::NoAlpha, Rgba::NoAlpha)) {
        auto* renderer = ServiceLocator::get<IRendererService>();
        auto* app = ServiceLocator::get<IAppService>();
        !renderer->GetOutput()->GetWindow()->IsFullscreen() ? app->Maximize() : app->Restore();
    }
    ImGui::SameLine(minimize_button_offset);
    if(ImGui::ImageButton(GetAssetTextureFromPath(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/MinimizeButtonAsset.png"), Vector2{nc_button_sizes, nc_button_sizes}, Vector2::Zero, Vector2::One, 0, Rgba::NoAlpha, Rgba::NoAlpha)) {
        auto* app = ServiceLocator::get<IAppService>();
        app->SetIsQuitting(true);
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

void Editor::ShowWorldInspectorWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    ImGui::Begin("World Inspector");
    {
        
    }
    ImGui::End();
}

void Editor::ShowSettingsWindow(TimeUtils::FPSeconds deltaSeconds) noexcept {
    ImGui::Begin("Settings");
    {
        const auto str = std::format("Render Time: {}", m_lastRenderTime);
        ImGui::Text(str.c_str());
        if(ImGui::Button("Generate")) {
            GenerateRayTrace();
        }
        ImGui::SameLine();
        if(ImGui::Button("Fill")) {
            FillRayTrace();
        }
        ImGui::SameLine();
        if(ImGui::Button("Fast Fill")) {
            FastFillRayTrace();
        }
        if(ImGui::Button("Job Fill")) {
            JobFillRayTrace();
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop Job")) {
            m_requestquit = true;
            if(m_rayTraceWorker.joinable()) {
                m_rayTraceWorker.join();
            }
        }
        ShowSelectedEntityComponents(deltaSeconds);
    }
    ImGui::End();
}

void Editor::ShowPropertiesWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    ImGui::Begin("Properties");
    {

    }
    ImGui::End();
}

void Editor::ShowMainViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    {
        std::ostringstream ss{};
        if(IsSceneLoaded()) {
            //TODO: Scenes should store their names.
            ss << m_ActiveScene;
        } else {
            ss << "Viewport";
        }
        ImGui::Begin(ss.str().c_str(), nullptr);
    }
    const auto viewportSize = ImGui::GetContentRegionAvail();
    if(viewportSize.x != m_ViewportWidth || viewportSize.y != m_ViewportHeight) {
        m_ViewportWidth = static_cast<uint32_t>(std::floor(viewportSize.x));
        m_ViewportHeight = static_cast<uint32_t>(std::floor(viewportSize.y));
        buffer->Resize(m_ViewportWidth, m_ViewportHeight);
    }
    ImGui::Image(buffer->GetTexture(), viewportSize, Vector2::Zero, Vector2::One, Rgba::White, Rgba::NoAlpha);
    m_IsViewportWindowActive = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
    ImGui::End();
}

void Editor::ShowContentBrowserWindow(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_ContentBrowser.Update(deltaSeconds);
}

void Editor::HandleMenuKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* input = ServiceLocator::get<IInputService>();
    if(input->IsKeyDown(KeyCode::Ctrl)) {
        if(input->WasKeyJustPressed(KeyCode::N)) {
            DoFileNew();
        } else if(input->WasKeyJustPressed(KeyCode::O)) {
            DoFileOpen();
        } else if(input->WasKeyJustPressed(KeyCode::S)) {
            DoFileSaveAs();
        }
    }
}

void Editor::HandleCameraInput(TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(m_IsViewportWindowActive) {
        m_editorCamera.Update(deltaSeconds);
    }
}

bool Editor::HasAssetExtension(const std::filesystem::path& path) const noexcept {
    return std::filesystem::is_directory(path) || path.has_extension() && IsAssetExtension(path.extension());
}

Texture* Editor::GetAssetTextureFromPath(const std::filesystem::path& path) const noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    auto* defaultTexture = renderer->GetTexture("__white");
    if(HasAssetExtension(path)) {
        const auto e = path.extension();
        std::filesystem::path p{};
        const auto BuildPath = [&](const char* pathSuffix) -> std::filesystem::path {
            auto p = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / std::filesystem::path{pathSuffix};
            p = std::filesystem::canonical(p); //Intentionally throw uncaught exception if path does not exist.
            p = p.make_preferred();
            return p;
        };
        if(std::filesystem::is_directory(path)) {
            p = BuildPath("Resources/Icons/FolderAsset.png");
        } else if(e == ".txt") {
            p = BuildPath("Resources/Icons/TextAsset.png");
        } else if(e == ".ascene") {
            p = BuildPath("Resources/Icons/SceneAsset.png");
        } else if(e == ".log") {
            p = BuildPath("Resources/Icons/LogAsset.png");
        } else if(IsImageAssetExtension(e)) {
            return renderer->CreateOrGetTexture(path, IntVector3::XY_Axis);
        }
        return p.empty() ? defaultTexture : renderer->GetTexture(p.string());
    }
    return defaultTexture;
}

Editor::AssetType Editor::GetAssetType(const std::filesystem::path& path) const noexcept {
    const auto e = path.extension();
    if(IsAssetExtension(e)) {
        if(e == ".txt") {
            return Editor::AssetType::Text;
        }
        if(e == ".log") {
            return Editor::AssetType::Log;
        }
        if(e == ".ascene") {
            return Editor::AssetType::Scene;
        }
        if(IsImageAssetExtension(e)) {
            return Editor::AssetType::Texture;
        }
    }
    if(std::filesystem::is_directory(path)) {
        return Editor::AssetType::Folder;
    }
    return Editor::AssetType::None;
}

bool Editor::IsAssetExtension(const std::filesystem::path& ext) const noexcept {
    if(ext == ".txt") {
        return true;
    } else if(ext == ".ascene") {
        return true;
    } else if(ext == ".log") {
        return true;
    } else {
        return IsImageAssetExtension(ext);
    }
}

bool Editor::IsImageAssetExtension(const std::filesystem::path& ext) const noexcept {
    return Image::IsSupportedExtension(ext);
}
