#include "Engine/UI/UIWidget.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Engine/UI/UICanvas.hpp"
#include "Engine/UI/UILabel.hpp"
#include "Engine/UI/UIPanel.hpp"
#include "Engine/UI/UIPictureBox.hpp"

#include <Thirdparty/TinyXML2/tinyxml2.h>

#include <memory>
#include <typeinfo>

UIWidget::UIWidget(const std::filesystem::path& path)
{
    {
        auto err_msg{"Failed loading Widget:\n" + path.string() + "\n is ill-formed."};
        GUARANTEE_OR_DIE(LoadFromXML(path), err_msg.c_str());
    }
}

UIWidget::~UIWidget() {
    _elements.clear();
    _elements.shrink_to_fit();

    _panel = nullptr;
}

void UIWidget::BeginFrame() {
    /* DO NOTHING */
}

void UIWidget::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(_panel) {
        _panel->Update(deltaSeconds);
    }
}

void UIWidget::Render() const {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetMaterial(renderer.GetMaterial("__2D"));
    if(_panel) {
        _panel->Render();
    }
}

void UIWidget::DebugRender() const {
    if(_panel) {
        _panel->DebugRender();
    }
}

void UIWidget::EndFrame() {
    /* DO NOTHING */
}

bool UIWidget::LoadFromXML(const std::filesystem::path& path) {
    if(!FileUtils::IsSafeReadPath(path)) {
        return false;
    }
    if(tinyxml2::XMLDocument doc; tinyxml2::XML_SUCCESS == doc.LoadFile(path.string().c_str())) {
        if(auto* xml_ui = doc.RootElement()) {
            if(!HasPanelChild(*xml_ui)) {
                return false;
            }
            LoadUI(*xml_ui);
            return true;
        }
    }
    return false;
}

void UIWidget::LoadUI(const XMLElement& element) {
    DataUtils::ValidateXmlElement(element, "ui", "", "name", "canvas");
    name = DataUtils::ParseXmlAttribute(element, "name", name);
    const auto load_children = [this](const XMLElement& elem) {
        const auto* c_name = elem.Name();
        const auto strName = std::string{c_name ? c_name : ""};
        _elements.emplace_back(CreateWigetTypeFromTypename(strName, elem));
    };
    const auto load_all_children = [this, &load_children](const XMLElement& elem) {
        load_children(elem);
        DataUtils::ForEachChildElement(elem, std::string{}, load_children);
    };
    DataUtils::ForEachChildElement(element, std::string{}, load_all_children);
}

std::shared_ptr<UIElement> UIWidget::CreateWigetTypeFromTypename(std::string nameString, const XMLElement& elem) {
    const auto childname = StringUtils::ToLowerCase(nameString);
    if(childname == "canvas") {
        auto c = std::make_shared<UICanvas>(this, elem);
        _panel = c.get();
        return c;
    } else if(childname == "label") {
        if(const auto* parent = elem.Parent(); parent->ToElement()) {
            const auto parent_name = DataUtils::ParseXmlAttribute(*parent->ToElement(), "name", std::string{});
            const auto found = std::find_if(std::begin(_elements), std::end(_elements), [&parent_name](std::shared_ptr<UIElement>& element) { return element->GetName() == parent_name; });
            if(found != std::end(_elements)) {
                if(auto* foundAsPanel = dynamic_cast<UIPanel*>(found->get())) {
                    auto lbl = std::make_shared<UILabel>(elem, foundAsPanel);
                    return lbl;
                }
            }
        }
        return std::make_shared<UILabel>(elem);
    } else if(childname == "picturebox") {
        if(const auto* parent = elem.Parent(); parent->ToElement()) {
            const auto parent_name = DataUtils::ParseXmlAttribute(*parent->ToElement(), "name", std::string{});
            const auto found = std::find_if(std::begin(_elements), std::end(_elements), [&parent_name](std::shared_ptr<UIElement>& element) { return element->GetName() == parent_name; });
            if(found != std::end(_elements)) {
                if(auto* foundAsPanel = dynamic_cast<UIPanel*>(found->get())) {
                    auto pic = std::make_shared<UIPictureBox>(elem, foundAsPanel);
                    return pic;
                }
            }
        }
        return std::make_shared<UIPictureBox>(elem);
    } else {
        return nullptr;
    }
}

bool UIWidget::HasPanelChild(const XMLElement& elem) {
    if(const auto* first_child = elem.FirstChildElement()) {
        const auto* elem_name_cstr = first_child->Name();
        const auto elem_name = std::string{elem_name_cstr ? elem_name_cstr : ""};
        return elem_name == "canvas" || "grid";
    }
    return false;
}
