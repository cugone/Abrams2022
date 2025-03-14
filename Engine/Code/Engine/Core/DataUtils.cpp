#include "Engine/Core/DataUtils.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Profiling/ProfileLogScope.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace DataUtils {

namespace detail {

bool to_bool(const std::string& value) noexcept {
    if(auto lowercase = StringUtils::ToLowerCase(StringUtils::TrimWhitespace(value)); lowercase == "false" || lowercase == "true") {
        if(lowercase == "false")
            return false;
        if(lowercase == "true")
            return true;
    }
    try {
        if(auto asInt = std::stoi(value); !asInt) {
            return false;
        }
        return true;
    } catch(...) {
        return false;
    }
}

} // namespace detail

void ValidateXmlElement(const XMLElement& element,
                        std::string name,
                        std::string requiredChildElements,
                        std::string requiredAttributes,
                        std::string optionalChildElements /*= std::string("")*/,
                        std::string optionalAttributes /*= std::string("")*/) noexcept {
    GUARANTEE_OR_DIE(!name.empty(), "FAILED: Element name is required.");
    {
        const auto* xmlNameAsCStr = element.Name();
        const auto xml_name = std::string{xmlNameAsCStr ? xmlNameAsCStr : ""};
        GUARANTEE_OR_DIE(xml_name == name, "FAILED: Element name does not match valid name.\n");
    }

    //Get list of required/optional attributes/children
    //Sort
    //Remove duplicates
    //Rational for not using std:set:
    //Profiled code takes average of 10 microseconds to complete.
    auto requiredAttributeNames = StringUtils::Split(requiredAttributes);
    std::sort(requiredAttributeNames.begin(), requiredAttributeNames.end());
    requiredAttributeNames.erase(std::unique(requiredAttributeNames.begin(), requiredAttributeNames.end()), requiredAttributeNames.end());

    auto requiredChildElementNames = StringUtils::Split(requiredChildElements);
    std::sort(requiredChildElementNames.begin(), requiredChildElementNames.end());
    requiredChildElementNames.erase(std::unique(requiredChildElementNames.begin(), requiredChildElementNames.end()), requiredChildElementNames.end());

    auto optionalChildElementNames = StringUtils::Split(optionalChildElements);
    std::sort(optionalChildElementNames.begin(), optionalChildElementNames.end());
    optionalChildElementNames.erase(std::unique(optionalChildElementNames.begin(), optionalChildElementNames.end()), optionalChildElementNames.end());

    auto optionalAttributeNames = StringUtils::Split(optionalAttributes);
    std::sort(optionalAttributeNames.begin(), optionalAttributeNames.end());
    optionalAttributeNames.erase(std::unique(optionalAttributeNames.begin(), optionalAttributeNames.end()), optionalAttributeNames.end());

    auto actualChildElementNames = GetChildElementNames(element);
    std::sort(actualChildElementNames.begin(), actualChildElementNames.end());
    actualChildElementNames.erase(std::unique(actualChildElementNames.begin(), actualChildElementNames.end()), actualChildElementNames.end());

    auto actualAttributeNames = GetAttributeNames(element);
    std::sort(actualAttributeNames.begin(), actualAttributeNames.end());
    actualAttributeNames.erase(std::unique(actualAttributeNames.begin(), actualAttributeNames.end()), actualAttributeNames.end());

    //Difference between actual attribute names and required list is list of actual optional attributes.
    std::vector<std::string> actualOptionalAttributeNames;
    std::set_difference(actualAttributeNames.begin(), actualAttributeNames.end(),
                        requiredAttributeNames.begin(), requiredAttributeNames.end(),
                        std::back_inserter(actualOptionalAttributeNames));
    std::sort(actualOptionalAttributeNames.begin(), actualOptionalAttributeNames.end());

    //Difference between actual child names and required list is list of actual optional children.
    std::vector<std::string> actualOptionalChildElementNames;
    std::set_difference(actualChildElementNames.begin(), actualChildElementNames.end(),
                        requiredChildElementNames.begin(), requiredChildElementNames.end(),
                        std::back_inserter(actualOptionalChildElementNames));
    std::sort(actualOptionalChildElementNames.begin(), actualOptionalChildElementNames.end());

    const auto get_xml_list_as_string = [](const auto& list) -> const std::string {
        std::string s{};
        for(const auto& a : list) {
            s += '\t' + a + '\n';
        }
        return s;
    };

    //Find missing attributes
    std::vector<std::string> missingRequiredAttributes;
    std::set_difference(requiredAttributeNames.begin(), requiredAttributeNames.end(),
                        actualAttributeNames.begin(), actualAttributeNames.end(),
                        std::back_inserter(missingRequiredAttributes));
    {
        const auto list_s = get_xml_list_as_string(missingRequiredAttributes);
        const auto err_ss = std::format("\nFAILED: Missing required attributes(s):\n{}\n", name, list_s);
        GUARANTEE_OR_DIE(missingRequiredAttributes.empty(), err_ss.c_str());
    }

    //Find missing children
    std::vector<std::string> missingRequiredChildren;
    std::set_difference(requiredChildElementNames.begin(), requiredChildElementNames.end(),
                        actualChildElementNames.begin(), actualChildElementNames.end(),
                        std::back_inserter(missingRequiredChildren));
    {
        const auto list_s = get_xml_list_as_string(missingRequiredChildren);
        const auto err_ss = std::format("\nFAILED: Missing required child element(s):\n{}\n", name, list_s);
        GUARANTEE_OR_DIE(missingRequiredChildren.empty(), err_ss.c_str());
    }

#ifdef DEBUG_BUILD
    //Find extra attributes
    std::vector<std::string> extraOptionalAttributes;
    std::set_difference(actualOptionalAttributeNames.begin(), actualOptionalAttributeNames.end(),
                        optionalAttributeNames.begin(), optionalAttributeNames.end(),
                        std::back_inserter(extraOptionalAttributes));

    if(!extraOptionalAttributes.empty()) {
        const auto list_s = get_xml_list_as_string(extraOptionalAttributes);
        DebuggerPrintf(std::format("\nWARNING: Found unknown attributes. Verify attributes are correct:\n{}\n", name, list_s));
    }

    //Find extra children
    std::vector<std::string> extraOptionalChildren;
    std::set_difference(actualOptionalChildElementNames.begin(), actualOptionalChildElementNames.end(),
                        optionalChildElementNames.begin(), optionalChildElementNames.end(),
                        std::back_inserter(extraOptionalChildren));

    if(!extraOptionalChildren.empty()) {
        const auto list_s = get_xml_list_as_string(extraOptionalChildren);
        DebuggerPrintf(std::format("\nWARNING: Found unknown children. Verify child elements are correct:\n{}\n", name, list_s));
    }
#endif //#if DEBUG_BUILD
}

void ValidateXmlAttribute(const XMLElement& elem, std::string attributeName, std::string validValuesList) noexcept {
    {
        const auto has_attribute = HasAttribute(elem, attributeName);
        const auto elem_name = GetElementName(elem);
        const auto msg = std::format("\nFAILED: Element \"{}\" is missing attribute with name: {}", elem_name, attributeName);
        GUARANTEE_OR_DIE(has_attribute, msg.c_str());
    }
    const auto attribute_is_valid = [&elem, &validValuesList, &attributeName]() {
        const auto attribute_value = DataUtils::GetAttributeAsString(elem, attributeName);
        for(const auto& attribute : StringUtils::Split(validValuesList)) {
            if(attribute == attribute_value) {
                return true;
            }
        }
        return false;
    }(); //IIIL
    {
        const auto msg = std::format("\nFAILED: Attribute \"{}\" value is invalid. Must be one of: {}", attributeName, validValuesList);
        GUARANTEE_OR_DIE(attribute_is_valid, msg.c_str());
    }
}

std::string EscapeGlyphToXmlCharacterEntity(const char glyph) noexcept {
    switch(glyph) {
    case '\"': return "&quot;";
    case '&': return "&amp;";
    case '\'': return "&apos;";
    case '<': return "&lt;";
    case '>': return "&gt;";
    default: return {glyph};
    }
}

std::size_t GetAttributeCount(const XMLElement& element) noexcept {
    std::size_t attributeCount = 0u;
    for(auto* attribute = element.FirstAttribute(); attribute != nullptr; attribute = attribute->Next()) {
        ++attributeCount;
    }
    return attributeCount;
}

std::vector<std::string> GetAttributeNames(const XMLElement& element) noexcept {
    std::vector<std::string> attributeNames{};
    attributeNames.reserve(GetAttributeCount(element));
    ForEachAttribute(element,
                     [&](const XMLAttribute& attribute) {
                         attributeNames.emplace_back(attribute.Name());
                     });
    return attributeNames;
}

bool HasAttribute(const XMLElement& element) noexcept {
    return GetAttributeCount(element) != 0;
}

bool HasAttribute(const XMLElement& element, const std::string& name) {
    bool result = false;
    ForEachAttribute(element, [&name, &result](const XMLAttribute& attribute) {
        if(attribute.Name() == name) {
            result = true;
            return;
        }
    });
    return result;
}

std::size_t GetChildElementCount(const XMLElement& element, const std::string& elementName /*= std::string("")*/) noexcept {
    std::size_t childCount = 0u;
    const auto childNameAsCStr = elementName.empty() ? nullptr : elementName.c_str();
    for(auto* xml_iter = element.FirstChildElement(childNameAsCStr); xml_iter != nullptr; xml_iter = xml_iter->NextSiblingElement(childNameAsCStr)) {
        ++childCount;
    }
    return childCount;
}

std::vector<std::string> GetChildElementNames(const XMLElement& element) noexcept {
    std::vector<std::string> childElementNames{};
    childElementNames.reserve(GetChildElementCount(element));
    ForEachChildElement(element, std::string{},
                        [&](const XMLElement& elem) {
                            childElementNames.emplace_back(elem.Name());
                        });
    return childElementNames;
}

bool HasChild(const XMLElement& elem) noexcept {
    return elem.FirstChildElement() != nullptr;
}
bool HasChild(const XMLElement& elem, const std::string& name) noexcept {
    return elem.FirstChildElement(name.empty() ? nullptr : name.c_str()) != nullptr;
}

std::string GetElementName(const XMLElement& elem) noexcept {
    auto* name = elem.Name();
    if(name) {
        return {name};
    }
    return {};
}

std::string GetAttributeName(const XMLAttribute& attrib) noexcept {
    auto* name = attrib.Name();
    if(name) {
        return {name};
    }
    return {};
}

std::string GetElementTextAsString(const XMLElement& element) {
    const auto* txtAsCStr = element.GetText();
    return std::string{txtAsCStr ? txtAsCStr : ""};
}

std::string GetAttributeAsString(const XMLElement& element, const std::string& attributeName) {
    const auto* attrAsCStr = element.Attribute(attributeName.c_str());
    return std::string{attrAsCStr ? attrAsCStr : ""};
}

} // namespace DataUtils
