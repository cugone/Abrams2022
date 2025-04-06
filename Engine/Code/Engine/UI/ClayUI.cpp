#include "Engine/UI/ClayUI.hpp"

#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Input/KeyCode.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <algorithm>
#include <utility>

#ifndef UI_DEBUG
    #define CLAY_DISABLE_DEBUG_WINDOW
#else
    #undef CLAY_DISABLE_DEBUG_WINDOW
#endif


namespace Clay {

static inline Clay_Dimensions MeasureText(Clay_StringSlice text, [[maybe_unused]] Clay_TextElementConfig* config, void* userData) noexcept;

Clay_Color RgbaToClayColor(Rgba color) noexcept {
    const auto&& [r, g, b, a] = color.GetAsFloats();
    return {r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f};
}
Clay_String StrToClayString(const std::string& str) noexcept {
    return Clay_String{static_cast<int32_t>(str.size()), str.data()};
}

Clay_Dimensions Vector2ToClayDimensions(Vector2 v) noexcept {
    return Clay_Dimensions{v.x, v.y};
}

Clay_Vector2 Vector2ToClayVector2(Vector2 v) noexcept {
    return Clay_Vector2{v.x, v.y};
}

Rgba ClayColorToRgba(Clay_Color textColor) noexcept {
    const auto r = textColor.r / 255.0f;
    const auto g = textColor.g / 255.0f;
    const auto b = textColor.b / 255.0f;
    const auto a = textColor.a / 255.0f;
    return Rgba{r, g, b, a};
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text, [[maybe_unused]] Clay_TextElementConfig* config, void* userData) noexcept {
    // Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing etc
    // Note: Clay_String->chars is not guaranteed to be null terminated
    if(userData == nullptr || text.chars == nullptr) {
        return Clay_Dimensions{0.0f, 0.0f};
    }
    if(KerningFont* font = static_cast<KerningFont*>(userData); font != nullptr) {
        const auto str_text = std::string(text.chars, text.length);
        return {font->CalculateTextWidth(str_text), font->CalculateTextHeight()};
    } else {
        return Clay_Dimensions{0.0f, 0.0f};
    }
}

} // namespace Clay

void ClayUI::Initialize() noexcept {
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
    std::size_t totalMemorySize = Clay_MinMemorySize();
    m_clayMemoryBlock = std::make_unique<char[]>(totalMemorySize);
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, m_clayMemoryBlock.get());

    auto* renderer = ServiceLocator::get<IRendererService>();
    m_clayContext = Clay_Initialize(clayMemory, Clay::Vector2ToClayDimensions(Vector2(renderer->GetOutput()->GetDimensions())), Clay_ErrorHandler{error_f});
    Clay_SetMeasureTextFunction(Clay::MeasureText, renderer->GetFont("System32"));
}

void ClayUI::BeginFrame() noexcept {
    /* DO NOTHING */
}

void ClayUI::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();
    auto* input = ServiceLocator::get<IInputService>();
    Clay_SetLayoutDimensions(Clay::Vector2ToClayDimensions(Vector2(renderer->GetOutput()->GetDimensions())));
    const auto coords = input->GetMouseCoords();
    const auto isMouseDown = input->IsKeyDown(KeyCode::LButton);
    Clay_SetPointerState(Clay::Vector2ToClayVector2(coords), isMouseDown);
    const auto scrollDelta = Vector2(IntVector2(input->GetMouseWheelHorizontalPositionNormalized(), input->GetMouseWheelPositionNormalized())) * m_clayScrollSpeed;
    Clay_UpdateScrollContainers(true, Clay_Vector2{scrollDelta.x, scrollDelta.y}, deltaSeconds.count());
}

void ClayUI::Render() const noexcept {
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
            const auto& bb = command->boundingBox;
            const auto width = bb.width;
            const auto height = bb.height;
            const auto top = bb.y;
            const auto left = bb.x;
            const auto bottom = top + height;
            const auto right = left + width;
            const auto top_left = Vector2(left, top);
            const auto top_right = Vector2(right, top);
            const auto bottom_right = Vector2(right, bottom);
            const auto bottom_left = Vector2(left, bottom);
            const auto bounds = AABB2(top_left, bottom_right);
            const auto fillColor = Clay::ClayColorToRgba(config.backgroundColor);
            const auto half_extents = Vector2(bounds.CalcDimensions().x * 0.5f, bounds.CalcDimensions().y * 0.5f);
            const auto tl_cr = std::clamp(command->renderData.rectangle.cornerRadius.topLeft, 0.0f, (std::min)(half_extents.x, half_extents.y));
            const auto tr_cr = std::clamp(command->renderData.rectangle.cornerRadius.topRight, 0.0f, (std::min)(half_extents.x, half_extents.y));
            const auto br_cr = std::clamp(command->renderData.rectangle.cornerRadius.bottomRight, 0.0f, (std::min)(half_extents.x, half_extents.y));
            const auto bl_cr = std::clamp(command->renderData.rectangle.cornerRadius.bottomLeft, 0.0f, (std::min)(half_extents.x, half_extents.y));
            renderer->SetMaterial(renderer->GetMaterial("__2D"));
            renderer->DrawFilledRoundedRectangle2D(bounds, fillColor, tl_cr, tr_cr, br_cr, bl_cr);
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
            const auto top_left = Vector2(command->boundingBox.x, command->boundingBox.y);
            const auto bottom_right = top_left + Vector2(command->boundingBox.width, command->boundingBox.height);
            const auto bounds = AABB2(top_left, bottom_right);
            auto color = Clay::ClayColorToRgba(config.textColor);
            auto* font = static_cast<KerningFont*>(command->userData);
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
            const auto clay_color = Clay::ClayColorToRgba(config.backgroundColor);
            const auto tint = clay_color == Rgba::NoAlpha ? Rgba::White : clay_color;
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
            const auto& config = command->renderData.custom;
            if(m_clayCustomCallback) {
                m_clayCustomCallback(config.customData);
            }
            break;
        }
        default:
            break;
        }
    }
}

void ClayUI::EndFrame() noexcept {
    /* DO NOTHING */
}

void ClayUI::SetClayLayoutCallback(std::function<void()>&& layoutCallback) noexcept {
    m_clayLayoutCallback = std::move(layoutCallback);
}

void ClayUI::SetFontToMeasure(KerningFont* font) noexcept {
    Clay_SetMeasureTextFunction(Clay::MeasureText, font);
}

bool ClayUI::IsClayDebugWindowVisible() const noexcept {
#if !defined(CLAY_DISABLE_DEBUG_WINDOW)
    return m_show_clay_debug_window;
#else
    return false;
#endif
}

void ClayUI::ToggleClayDebugWindow() noexcept {
#if !defined(CLAY_DISABLE_DEBUG_WINDOW)
    m_show_clay_debug_window = !m_show_clay_debug_window;
    auto* input = ServiceLocator::get<IInputService>();
    if(!input->IsMouseCursorVisible()) {
        input->ShowMouseCursor();
    }
    Clay_SetDebugModeEnabled(m_show_clay_debug_window);
#endif
}
