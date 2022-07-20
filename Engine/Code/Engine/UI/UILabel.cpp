#include "Engine/UI/UILabel.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Renderer/Camera2D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Engine/UI/UICanvas.hpp"
#include "Engine/UI/UIPanel.hpp"
#include "Engine/UI/UITypes.hpp"
#include "Engine/UI/UIWidget.hpp"

UILabel::UILabel(UIPanel* parent)
: UIElement(parent) {
    /* DO NOTHING */
}

UILabel::UILabel(UIPanel* parent, KerningFont* font, const std::string& text /*= "Label"*/)
: UIElement(parent)
, m_font(font)
, m_text(text) {
    const auto desired_size = CalcDesiredSize();
    m_bounds.mins = desired_size.GetXY();
    m_bounds.maxs = desired_size.GetZW();
}

UILabel::UILabel(const XMLElement& elem, UIPanel* parent /*= nullptr*/)
: UIElement(parent) {
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Label constructor failed to load.");
}

void UILabel::Render() const {
    if(IsHidden()) {
        return;
    }
    const auto world_transform = GetWorldTransform();
    const auto inv_scale = 1.0f / world_transform.GetScale();
    const auto inv_scale_matrix = Matrix4::CreateScaleMatrix(inv_scale);
    const auto model = Matrix4::MakeRT(inv_scale_matrix, world_transform);
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetModelMatrix(model);
    renderer->SetMaterial(m_font->GetMaterial());
    renderer->DrawMultilineText(m_font, m_text, m_color);
}

const KerningFont* const UILabel::GetFont() const {
    return m_font;
}

void UILabel::SetFont(KerningFont* font) {
    m_font = font;
    DirtyElement();
    CalcBounds();
}

void UILabel::SetText(const std::string& text) {
    m_text = text;
    DirtyElement();
    CalcBounds();
}

const std::string& UILabel::GetText() const {
    return m_text;
}

std::string& UILabel::GetText() {
    return m_text;
}

void UILabel::SetColor(const Rgba& color) {
    m_color = color;
}

const Rgba& UILabel::GetColor() const {
    return m_color;
}

Rgba& UILabel::GetColor() {
    return m_color;
}

void UILabel::SetScale(float value) {
    m_scale = value;
    DirtyElement();
    CalcBounds();
}

float UILabel::GetScale() const {
    return m_scale;
}

float UILabel::GetScale() {
    return static_cast<const UILabel&>(*this).GetScale();
}

void UILabel::SetPosition(const Vector4& position) {
    UIElement::SetPosition(position);
}

void UILabel::SetPositionOffset(const Vector2& offset) {
    UIElement::SetPositionOffset(offset);
}

void UILabel::SetPositionRatio(const Vector2& ratio) {
    UIElement::SetPositionRatio(ratio);
}

Vector4 UILabel::CalcDesiredSize() const noexcept {
    const auto desired_size = CalcBoundsFromFont(m_font);
    return Vector4{Vector2::Zero, desired_size};
}

Vector2 UILabel::CalcBoundsFromFont(KerningFont* font) const {
    if(font == nullptr) {
        return {};
    }
    const float width = font->CalculateTextWidth(m_text, m_scale);
    const float height = font->CalculateTextHeight(m_text, m_scale);
    return Vector2{width, height};
}

bool UILabel::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "label", "", "name", "canvas,label,panel,picturebox,button,slot", "font,value");
    m_name = DataUtils::ParseXmlAttribute(elem, "name", m_name);
    m_fontname = DataUtils::ParseXmlAttribute(elem, "font", m_fontname);
    m_font = ServiceLocator::get<IRendererService>()->GetFont(m_fontname);
    m_text = DataUtils::ParseXmlAttribute(elem, "value", std::string{"TEXT"});

    if(auto* xml_slot = elem.FirstChildElement("slot")) {
        if(auto* parent = GetParent()) {
            parent->AddChildFromXml(*xml_slot, this);
            parent->RemoveChild(this);
        }
    }
    return true;
}

