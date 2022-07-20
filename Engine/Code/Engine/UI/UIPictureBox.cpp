#include "Engine/UI/UIPictureBox.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include "Engine/UI/UIPanel.hpp"
#include "Engine/UI/UIWidget.hpp"

UIPictureBox::UIPictureBox(UIPanel* parent /*= nullptr*/)
: UIElement(parent) {
    /* DO NOTHING */
}
UIPictureBox::UIPictureBox(const XMLElement& elem, UIPanel* parent /*= nullptr*/)
: UIElement(parent) {
    GUARANTEE_OR_DIE(LoadFromXml(elem), "PictureBox constructor failed to load.");
}

void UIPictureBox::SetImage(std::unique_ptr<AnimatedSprite> sprite) noexcept {
    m_sprite.reset(sprite.release());
    GetSlot()->CalcPivot();
}

const AnimatedSprite* const UIPictureBox::GetImage() const noexcept {
    return m_sprite.get();
}

void UIPictureBox::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(IsDisabled()) {
        return;
    }
    m_sprite->Update(deltaSeconds);
}

void UIPictureBox::Render() const {
    if(IsHidden()) {
        return;
    }
    auto* material = m_sprite->GetMaterial();
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetModelMatrix(GetWorldTransform());
    renderer->SetMaterial(material);
    const auto cur_tc = m_sprite->GetCurrentTexCoords();
    Vector4 tex_coords(cur_tc.mins, cur_tc.maxs);
    renderer->DrawQuad2D(tex_coords);
}

void UIPictureBox::DebugRender() const {
    UIElement::DebugRender();
}

Vector4 UIPictureBox::CalcDesiredSize() const noexcept {
    const auto dims = m_sprite->GetFrameDimensions();
    const auto w = static_cast<float>(dims.x);
    const auto h = static_cast<float>(dims.y);
    return Vector4{Vector2::Zero, Vector2{w, h}};
}

bool UIPictureBox::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "picturebox", "", "name,src", "");
    m_name = DataUtils::ParseXmlAttribute(elem, "name", m_name);
    if(const auto src = DataUtils::ParseXmlAttribute(elem, "src", std::string{}); FileUtils::IsSafeReadPath(src)) {
        auto* renderer = ServiceLocator::get<IRendererService>();
        m_sprite = renderer->CreateAnimatedSprite(src);
        return true;
    }
    return false;
}

