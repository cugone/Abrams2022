#include "Engine/UI/UISystem.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Input/KeyCode.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include "Engine/UI/UIWidget.hpp"

#include <Thirdparty/Imgui/imgui_internal.h>
#include <Thirdparty/clay/clay.h>

#include <algorithm>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static inline Clay_Dimensions MeasureText(Clay_StringSlice text, [[maybe_unused]] Clay_TextElementConfig* config, void* userData) noexcept;


namespace Clay {
Clay_Color RgbaToClayColor(Rgba color) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    return {r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f};
}
Clay_String StrToClayString(std::string str) noexcept {
    return Clay_String{static_cast<int32_t>(str.size()), str.c_str()};
}

Clay_Dimensions Vector2ToClayDimensions(Vector2 v) noexcept {
    return Clay_Dimensions{v.x, v.y};
}

Clay_Vector2 Vector2ToClayVector2(Vector2 v) noexcept {
    return Clay_Vector2{v.x, v.y};
}

} // namespace Clay


namespace ImGui {
void Image(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = border_col.GetAsFloats();
        ImGui::Image(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, Vector4{tr, tg, tb, ta}, Vector4{br, bg, bb, ba});
    }
}
void Image(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, const Rgba& tint_col, const Rgba& border_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = border_col.GetAsFloats();
        ImGui::Image(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, Vector4{tr, tg, tb, ta}, Vector4{br, bg, bb, ba});
    }
}

bool ImageButton(const Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = bg_col.GetAsFloats();
        return ImGui::ImageButton(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, frame_padding, Vector4{br, bg, bb, ba}, Vector4{tr, tg, tb, ta});
    }
    return false;
}
bool ImageButton(Texture* texture, const Vector2& size, const Vector2& uv0, const Vector2& uv1, int frame_padding, const Rgba& bg_col, const Rgba& tint_col) noexcept {
    if(texture) {
        const auto&& [tr, tg, tb, ta] = tint_col.GetAsFloats();
        const auto&& [br, bg, bb, ba] = bg_col.GetAsFloats();
        return ImGui::ImageButton(static_cast<void*>(texture->GetShaderResourceView()), size, uv0, uv1, frame_padding, Vector4{br, bg, bb, ba}, Vector4{tr, tg, tb, ta});
    }
    return false;
}


bool ColorEdit3(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, 1.0f};
    if(ImGui::ColorEdit3(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, 1.0f});
        return true;
    }
    return false;
}
bool ColorEdit4(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, a};
    if(ImGui::ColorEdit4(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, colorAsFloats.w});
        return true;
    }
    return false;
}
bool ColorPicker3(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/) noexcept {
    const auto&& [r, g, b, _] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, 1.0f};
    if(ImGui::ColorPicker3(label, colorAsFloats.GetAsFloatArray(), flags)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z});
        return true;
    }
    return false;
}
bool ColorPicker4(const char* label, Rgba& color, ImGuiColorEditFlags flags /*= 0*/, Rgba* refColor /*= nullptr*/) noexcept {
    Vector4 refColorAsFloats{};
    if(refColor) {
        const auto&& [rr, rg, rb, ra] = refColor->GetAsFloats();
        refColorAsFloats = Vector4{rr, rg, rb, ra};
    }
    const auto&& [r, g, b, a] = color.GetAsFloats();
    Vector4 colorAsFloats{r, g, b, a};
    if(ImGui::ColorPicker4(label, colorAsFloats.GetAsFloatArray(), flags, refColor ? refColorAsFloats.GetAsFloatArray() : nullptr)) {
        color.SetFromFloats({colorAsFloats.x,colorAsFloats.y,colorAsFloats.z, colorAsFloats.w});
        if(refColor) {
            refColor->SetFromFloats({refColorAsFloats.x,refColorAsFloats.y,refColorAsFloats.z, refColorAsFloats.w});
        }
        return true;
    }
    return false;
}
bool ColorButton(const char* desc_id, const Rgba& color, ImGuiColorEditFlags flags /*= 0*/, Vector2 size /*= Vector2::ZERO*/) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    return ImGui::ColorButton(desc_id, Vector4{r, g, b, a}, flags, size);
}

void TextColored(const Rgba& color, const char* fmt, ...) noexcept {
    auto&& [r, g, b, a] = color.GetAsFloats();
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(Vector4{r, g, b, a}, fmt, args);
    va_end(args);
}

} // namespace ImGui

UISystem::UISystem() noexcept
: EngineSubsystem()
, m_imguiContext(ImGui::CreateContext()) {
#ifdef UI_DEBUG
    IMGUI_CHECKVERSION();
#endif
}

UISystem::~UISystem() noexcept {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext(m_imguiContext);
    m_imguiContext = nullptr;
}

void UISystem::Initialize() noexcept {
    namespace FS = std::filesystem;

    auto* renderer = ServiceLocator::get<IRendererService>();
    auto* hwnd = renderer->GetOutput()->GetWindow()->GetWindowHandle();
    auto* dx_device = renderer->GetDevice()->GetDxDevice();
    auto* dx_context = renderer->GetDeviceContext()->GetDxContext();

    const auto dims = Vector2{renderer->GetOutput()->GetDimensions()};
    auto& io = ImGui::GetIO();
    io.DisplaySize.x = dims.x;
    io.DisplaySize.y = dims.y;

    ImGui::StyleColorsDark();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    if(std::filesystem::exists(m_ini_filepath)) {
        ImGui::LoadIniSettingsFromDisk(m_ini_filepath.string().c_str());
    } else {
        ImGui::SaveIniSettingsToDisk(m_ini_filepath.string().c_str());
    }

    m_ini_saveTimer.SetSeconds(TimeUtils::FPSeconds{io.IniSavingRate});

    io.ConfigWindowsResizeFromEdges = true;
    io.ConfigDockingWithShift = true;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dx_device, dx_context);

    ClayInit();
}

void UISystem::SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept {
    m_clayLayoutCallback = std::move(layoutCallback);
}

bool UISystem::IsClayDebugWindowVisible() const noexcept {
#if !defined(CLAY_DISABLE_DEBUG_WINDOW)
    return m_show_imgui_demo_window;
#else
    return false;
#endif
}

void UISystem::ToggleClayDebugWindow() noexcept {
#if !defined(CLAY_DISABLE_DEBUG_WINDOW)
    m_show_clay_debug_window = !m_show_clay_debug_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
    Clay_SetDebugModeEnabled(m_show_clay_debug_window);
#endif
}

void UISystem::BeginFrame() noexcept {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if(m_ini_saveTimer.CheckAndReset()) {
        ImGui::SaveIniSettingsToDisk(m_ini_filepath.string().c_str());
    }
}

void UISystem::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    const auto* const app = ServiceLocator::get<IAppService>();
    auto& io = ImGui::GetIO();
    io.AddFocusEvent(app->HasFocus());

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    if(m_show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
    }
    if(m_show_imgui_metrics_window) {
        ImGui::ShowMetricsWindow(&m_show_imgui_metrics_window);
    }
#endif

    ClayUpdate(deltaSeconds);

}

void UISystem::Render() const noexcept {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    //2D View / HUD
    auto* renderer = ServiceLocator::get<IRendererService>();
    const float ui_view_height = renderer->GetCurrentViewport().height;
    const float ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, renderer->GetCurrentViewportAspectRatio());
    renderer->SetCamera(m_ui_camera);

    ClayRender();
}

void UISystem::EndFrame() noexcept {
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
}

bool UISystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    return ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(msg.hWnd), msg.nativeMessage, msg.wparam, msg.lparam);
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text, [[maybe_unused]] Clay_TextElementConfig* config, void* userData) noexcept {
    // Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing etc
    // Note: Clay_String->chars is not guaranteed to be null terminated
    KerningFont* font = static_cast<KerningFont*>(userData);
    const auto str_text = std::string(text.chars, text.length);
    return {font->CalculateTextWidth(str_text), font->CalculateTextHeight(str_text)};
}

void UISystem::ClayInit() noexcept {
    std::size_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, (char*)(std::malloc(totalMemorySize)));

    const auto error_f = [](Clay_ErrorData errorData) {
        const auto str = std::string(errorData.errorText.chars, errorData.errorText.length);
        const auto msg = std::format("{:s}", errorData.errorText.chars);
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(msg);
        switch(errorData.errorType) {
        case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_FUNCTION_NOT_PROVIDED: {
            ERROR_AND_DIE("Clay Error: No Text Measurement function pointer provided. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED: {
            ERROR_AND_DIE("Clay Error: Arena memory exceeded. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED: {
            ERROR_AND_DIE("Clay Error: Element Capacity exceeded. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED: {
            ERROR_AND_DIE("Clay Error: Text Measurement Capacity exceeded. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_DUPLICATE_ID: {
            ERROR_AND_DIE("Clay Error: Duplicate Clay ID detected. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND: {
            ERROR_AND_DIE("Clay Error: Floating container has no parent or was not found. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_PERCENTAGE_OVER_1: {
            ERROR_AND_DIE("Clay Error: Percentage value exceedes 1.0. See log for details.");
            break;
        }
        case CLAY_ERROR_TYPE_INTERNAL_ERROR: {
            ERROR_AND_DIE("An internal Clay error occured. See log for details.");
        }
        default:
            break;
        }
    };

    auto* renderer = ServiceLocator::get<IRendererService>();
    m_clayContext = Clay_Initialize(clayMemory, Clay::Vector2ToClayDimensions(Vector2(renderer->GetOutput()->GetDimensions())), Clay_ErrorHandler{error_f});
    Clay_SetMeasureTextFunction(MeasureText, renderer->GetFont("System32"));
}

void UISystem::ClayUpdate(TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    auto* input = ServiceLocator::get<IInputService>();
    Clay_SetLayoutDimensions(Clay::Vector2ToClayDimensions(Vector2(renderer->GetOutput()->GetDimensions())));
    const auto coords = input->GetMouseCoords();
    const auto isMouseDown = input->IsKeyDown(KeyCode::LButton);
    Clay_SetPointerState(Clay::Vector2ToClayVector2(coords), isMouseDown);
    const auto scrollDelta = Vector2(IntVector2(input->GetMouseWheelHorizontalPositionNormalized(), input->GetMouseWheelPositionNormalized())) * m_clayScrollSpeed;
    Clay_UpdateScrollContainers(true, Clay_Vector2{scrollDelta.x, scrollDelta.y}, deltaSeconds.count());
}

void UISystem::ClayRender() const noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->BeginHUDRender(m_ui_camera, Vector2(renderer->GetOutput()->GetCenter()), static_cast<float>(renderer->GetOutput()->GetDimensions().y));
    Clay_BeginLayout();
    if(m_clayLayoutCallback) {
        m_clayLayoutCallback();
    }
    m_clay_commands = Clay_EndLayout();
    for(std::size_t i = 0; i < m_clay_commands.length; ++i) {
        auto* command = &m_clay_commands.internalArray[i];
        switch(command->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_NONE: // This command type should be skipped.
        {
            ERROR_AND_DIE("A CLAY RENDER COMMAND TYPE OF NONE WAS ISSUED.");
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: // The renderer should draw a solid color rectangle.
        {
            const auto& config = command->renderData.rectangle;
            const auto r = config.backgroundColor.r / 255.0f;
            const auto g = config.backgroundColor.g / 255.0f;
            const auto b = config.backgroundColor.b / 255.0f;
            const auto a = config.backgroundColor.a / 255.0f;
            const auto& bb = command->boundingBox;
            const auto width = bb.width;
            const auto height = bb.height;
            const auto top = bb.y;
            const auto left = bb.x;
            const auto bottom = top + height;
            const auto right = left + width;
            const auto top_left = Vector2(left, top);
            const auto bottom_right = Vector2(right, bottom);
            const auto bounds = AABB2(top_left, bottom_right);
            const auto fillColor = Rgba{r, g, b, a};
            const auto tl_cr = command->renderData.rectangle.cornerRadius.topLeft;
            const auto tr_cr = command->renderData.rectangle.cornerRadius.topRight;
            const auto br_cr = command->renderData.rectangle.cornerRadius.bottomRight;
            const auto bl_cr = command->renderData.rectangle.cornerRadius.bottomLeft;
            std::vector corners{tl_cr, tr_cr, br_cr, bl_cr};
            if(std::all_of(std::cbegin(corners), std::cend(corners), [](float value) { return MathUtils::IsEquivalentToZero(value); })) {
                renderer->SetMaterial(renderer->GetMaterial("__2D"));
                const auto S = Matrix4::CreateScaleMatrix(bounds.CalcDimensions());
                const auto R = Matrix4::I;
                const auto T = Matrix4::CreateTranslationMatrix(bounds.CalcCenter());
                const auto M = Matrix4::MakeSRT(S, R, T);
                renderer->DrawQuad2D(M, fillColor);
            } else {
                renderer->DrawFilledRoundedRectangle2D(bounds, fillColor, tl_cr);
            }
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER: // The renderer should draw a colored border inset into the bounding box.
        {
            const auto& config = command->renderData.border;
            const auto bounds = AABB2(command->boundingBox.x, command->boundingBox.y, command->boundingBox.x + command->boundingBox.width, command->boundingBox.y + command->boundingBox.height);
            const auto borderColor = Rgba{config.color.r / 255.0f, config.color.g / 255.0f, config.color.b / 255.0f, config.color.a / 255.0f};
            const auto borderLeft = static_cast<float>(command->renderData.border.width.left);
            const auto borderRight = static_cast<float>(command->renderData.border.width.right);
            const auto borderTop = static_cast<float>(command->renderData.border.width.top);
            const auto borderBottom = static_cast<float>(command->renderData.border.width.bottom);
            if(borderLeft > 0.0f) {
                auto new_bounds = bounds;
                new_bounds.maxs.x = new_bounds.mins.x + borderLeft;
                const auto tl_cr = command->renderData.border.cornerRadius.topLeft;
                const auto tr_cr = command->renderData.border.cornerRadius.topRight;
                const auto br_cr = command->renderData.border.cornerRadius.bottomRight;
                const auto bl_cr = command->renderData.border.cornerRadius.bottomLeft;
                std::vector corners{tl_cr, tr_cr, br_cr, bl_cr};
                if(std::all_of(std::cbegin(corners), std::cend(corners), [](float value) { return MathUtils::IsEquivalentToZero(value); })) {
                    renderer->SetMaterial(renderer->GetMaterial("__2D"));
                    const auto S = Matrix4::CreateScaleMatrix(new_bounds.CalcDimensions());
                    const auto R = Matrix4::I;
                    const auto T = Matrix4::CreateTranslationMatrix(new_bounds.CalcCenter());
                    const auto M = Matrix4::MakeSRT(S, R, T);
                    renderer->DrawQuad2D(M, borderColor);
                } else {
                    renderer->DrawFilledRoundedRectangle2D(new_bounds, borderColor, tl_cr);
                }
            }
            if(borderRight > 0.0f) {
                auto new_bounds = bounds;
                new_bounds.mins.x = new_bounds.maxs.x - borderRight;
                const auto tl_cr = command->renderData.border.cornerRadius.topLeft;
                const auto tr_cr = command->renderData.border.cornerRadius.topRight;
                const auto br_cr = command->renderData.border.cornerRadius.bottomRight;
                const auto bl_cr = command->renderData.border.cornerRadius.bottomLeft;
                std::vector corners{tl_cr, tr_cr, br_cr, bl_cr};
                if(std::all_of(std::cbegin(corners), std::cend(corners), [](float value) { return MathUtils::IsEquivalentToZero(value); })) {
                    renderer->SetMaterial(renderer->GetMaterial("__2D"));
                    const auto S = Matrix4::CreateScaleMatrix(new_bounds.CalcDimensions());
                    const auto R = Matrix4::I;
                    const auto T = Matrix4::CreateTranslationMatrix(new_bounds.CalcCenter());
                    const auto M = Matrix4::MakeSRT(S, R, T);
                    renderer->DrawQuad2D(M, borderColor);
                } else {
                    renderer->DrawFilledRoundedRectangle2D(new_bounds, borderColor, tl_cr);
                }
            }
            if(borderTop > 0.0f) {
                auto new_bounds = bounds;
                new_bounds.maxs.y = new_bounds.mins.y + borderTop;
                const auto tl_cr = command->renderData.border.cornerRadius.topLeft;
                const auto tr_cr = command->renderData.border.cornerRadius.topRight;
                const auto br_cr = command->renderData.border.cornerRadius.bottomRight;
                const auto bl_cr = command->renderData.border.cornerRadius.bottomLeft;
                std::vector corners{tl_cr, tr_cr, br_cr, bl_cr};
                if(std::all_of(std::cbegin(corners), std::cend(corners), [](float value) { return MathUtils::IsEquivalentToZero(value); })) {
                    renderer->SetMaterial(renderer->GetMaterial("__2D"));
                    const auto S = Matrix4::CreateScaleMatrix(new_bounds.CalcDimensions());
                    const auto R = Matrix4::I;
                    const auto T = Matrix4::CreateTranslationMatrix(new_bounds.CalcCenter());
                    const auto M = Matrix4::MakeSRT(S, R, T);
                    renderer->DrawQuad2D(M, borderColor);
                } else {
                    renderer->DrawFilledRoundedRectangle2D(new_bounds, borderColor, tl_cr);
                }
            }
            if(borderBottom > 0.0f) {
                auto new_bounds = bounds;
                new_bounds.mins.y = new_bounds.maxs.y - borderBottom;
                const auto tl_cr = command->renderData.border.cornerRadius.topLeft;
                const auto tr_cr = command->renderData.border.cornerRadius.topRight;
                const auto br_cr = command->renderData.border.cornerRadius.bottomRight;
                const auto bl_cr = command->renderData.border.cornerRadius.bottomLeft;
                std::vector corners{tl_cr, tr_cr, br_cr, bl_cr};
                if(std::all_of(std::cbegin(corners), std::cend(corners), [](float value) { return MathUtils::IsEquivalentToZero(value); })) {
                    renderer->SetMaterial(renderer->GetMaterial("__2D"));
                    const auto S = Matrix4::CreateScaleMatrix(new_bounds.CalcDimensions());
                    const auto R = Matrix4::I;
                    const auto T = Matrix4::CreateTranslationMatrix(new_bounds.CalcCenter());
                    const auto M = Matrix4::MakeSRT(S, R, T);
                    renderer->DrawQuad2D(M, borderColor);
                } else {
                    renderer->DrawFilledRoundedRectangle2D(new_bounds, borderColor, tl_cr);
                }
            }
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_TEXT: // The renderer should draw text.
        {
            const auto& config = command->renderData.text;
            const auto str = std::string(config.stringContents.chars, config.stringContents.length);
            const auto r = config.textColor.r / 255.0f;
            const auto g = config.textColor.g / 255.0f;
            const auto b = config.textColor.b / 255.0f;
            const auto a = config.textColor.a / 255.0f;
            const auto top_left = Vector2(command->boundingBox.x, command->boundingBox.y);
            const auto bottom_right = top_left + Vector2(command->boundingBox.width, command->boundingBox.height);
            const auto bounds = AABB2(top_left, bottom_right);
            auto color = Rgba{r, g, b, a};
            const auto* font = static_cast<const KerningFont*>(command->userData);
            const auto S = Matrix4::I;
            const auto R = Matrix4::I;
            const auto T = Matrix4::CreateTranslationMatrix(bounds.CalcCenter() - Vector2(bounds.CalcDimensions().x, -bounds.CalcDimensions().y) * 0.5f);
            const auto M = Matrix4::MakeSRT(S, R, T);
            renderer->DrawTextLine(M, font, str, color);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: // The renderer should draw an image.
        {
            const auto& config = command->renderData.image;
            auto* mat = static_cast<Material*>(config.imageData);
            const auto top_left = Vector2(command->boundingBox.x, command->boundingBox.y);
            const auto bottom_right = top_left + Vector2(config.sourceDimensions.width, config.sourceDimensions.height);
            const auto bounds = AABB2(top_left, bottom_right);
            const auto S = Matrix4::CreateScaleMatrix(bounds.CalcDimensions());
            const auto R = Matrix4::I;
            const auto T = Matrix4::CreateTranslationMatrix(bounds.CalcCenter());
            const auto M = Matrix4::MakeSRT(S, R, T);
            const auto tint = [config]() -> const Rgba {
                const auto r = config.backgroundColor.r / 255.0f;
                const auto g = config.backgroundColor.g / 255.0f;
                const auto b = config.backgroundColor.b / 255.0f;
                const auto a = config.backgroundColor.a / 255.0f;
                const auto config_clr = Rgba{r, g, b, a};
                if(config_clr == Rgba::NoAlpha) {
                    return Rgba::White;
                }
                return config_clr;
            }();
            renderer->SetMaterial(mat);
            renderer->DrawQuad2D(M, tint);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: // The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
        {
            const auto bounds = AABB2(command->boundingBox.x, command->boundingBox.y, command->boundingBox.x + command->boundingBox.width, command->boundingBox.y + command->boundingBox.height);
            renderer->SetScissor(bounds);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: // The renderer should finish any previously active clipping, and begin rendering elements in full again.
        {
            renderer->SetScissorAsPercent();
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: // The renderer should provide a custom implementation for handling this render command based on its .customData
        {
            break;
        }
        default:
            break;
        }
    }
}

bool UISystem::HasFocus() const noexcept {
    auto& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

bool UISystem::WantsInputCapture() const noexcept {
    return WantsInputKeyboardCapture() || WantsInputMouseCapture();
}

bool UISystem::WantsInputKeyboardCapture() const noexcept {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool UISystem::WantsInputMouseCapture() const noexcept {
    return ImGui::GetIO().WantCaptureMouse;
}

bool UISystem::IsImguiDemoWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    return m_show_imgui_demo_window;
#else
    return false;
#endif
}

void UISystem::ToggleImguiDemoWindow() noexcept {
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
    m_show_imgui_demo_window = !m_show_imgui_demo_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
#endif
}

bool UISystem::IsImguiMetricsWindowVisible() const noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    return m_show_imgui_metrics_window;
#else
    return false;
#endif
}

void UISystem::ToggleImguiMetricsWindow() noexcept {
#if !defined(IMGUI_DISABLE_METRICS_WINDOW)
    m_show_imgui_metrics_window = !m_show_imgui_metrics_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
#endif
}

bool UISystem::IsAnyImguiDebugWindowVisible() const noexcept {
#ifdef UI_DEBUG
    return IsImguiDemoWindowVisible() || IsImguiMetricsWindowVisible();
#else
    return false;
#endif
}

bool UISystem::IsAnyDebugWindowVisible() const noexcept {
#ifdef UI_DEBUG
    return IsAnyImguiDebugWindowVisible() || IsClayDebugWindowVisible();
#else
    return false;
#endif
}
