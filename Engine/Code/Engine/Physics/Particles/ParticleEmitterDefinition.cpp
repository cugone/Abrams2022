#include "Engine/Physics/Particles/ParticleEmitterDefinition.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

ParticleEmitterDefinition::ParticleEmitterDefinition(const XMLElement& element) noexcept {
    DataUtils::ValidateXmlElement(element, "emitter", "", "name", "lifetime,position,velocity,acceleration,initial_burst,per_second,particle_lifetime,color,scale,prewarm,material");

    _name = DataUtils::ParseXmlAttribute(element, "name", std::string{"UNNAMED_PARTICLE_EMITTER"});

    if(auto* xml_lifetime = element.FirstChildElement("lifetime"); xml_lifetime) {
        std::string lifetime_as_str = "undefined";
        lifetime_as_str = DataUtils::ParseXmlElementText(*xml_lifetime, lifetime_as_str);
        if(StringUtils::ToUpperCase(lifetime_as_str) == "INFINITY") {
            _lifetime = (std::numeric_limits<float>::max)();
        } else {
            _lifetime = DataUtils::ParseXmlElementText(*xml_lifetime, _lifetime);
        }
    }

    if(auto* xml_position = element.FirstChildElement("position"); xml_position) {
        DataUtils::ValidateXmlElement(*xml_position, "position", "", "", "in_line,in_disc,in_cone,in_sphere");
        if(bool has_children = xml_position->FirstChildElement() != nullptr; !has_children) {
            _emitterPositionDefinition.type = EmitterType::Point;
            _position = DataUtils::ParseXmlElementText(*xml_position, _position);
        } else {
            if(auto* xml_inline = xml_position->FirstChildElement("in_line"); xml_inline) {
                DataUtils::ValidateXmlElement(*xml_inline, "in_line", "", "start,end");
                _emitterPositionDefinition.type = EmitterType::Line;
                _emitterPositionDefinition.start = DataUtils::ParseXmlAttribute(*xml_inline, "start", _emitterPositionDefinition.start);
                _emitterPositionDefinition.end = DataUtils::ParseXmlAttribute(*xml_inline, "end", _emitterPositionDefinition.end);
            }
            if(auto* xml_indisc = xml_position->FirstChildElement("in_disc"); xml_indisc) {
                DataUtils::ValidateXmlElement(*xml_indisc, "in_disc", "", "normal,radius");
                _emitterPositionDefinition.type = EmitterType::Disc;
                _emitterPositionDefinition.normal = DataUtils::ParseXmlAttribute(*xml_indisc, "normal", _emitterPositionDefinition.normal);
                _emitterPositionDefinition.radius = DataUtils::ParseXmlAttribute(*xml_indisc, "radius", _emitterPositionDefinition.radius);
            }
            if(auto* xml_incone = xml_position->FirstChildElement("in_cone"); xml_incone) {
                DataUtils::ValidateXmlElement(*xml_incone, "in_cone", "", "normal,length,theta");
                _emitterPositionDefinition.type = EmitterType::Cone;
                _emitterPositionDefinition.normal = DataUtils::ParseXmlAttribute(*xml_incone, "normal", _emitterPositionDefinition.normal);
                _emitterPositionDefinition.length = DataUtils::ParseXmlAttribute(*xml_incone, "length", _emitterPositionDefinition.length);
                _emitterPositionDefinition.theta = DataUtils::ParseXmlAttribute(*xml_incone, "theta", _emitterPositionDefinition.theta);
            }
            if(auto* xml_insphere = xml_position->FirstChildElement("in_sphere"); xml_insphere) {
                DataUtils::ValidateXmlElement(*xml_insphere, "in_sphere", "", "position,radius");
                _emitterPositionDefinition.type = EmitterType::Sphere;
                _emitterPositionDefinition.start = DataUtils::ParseXmlAttribute(*xml_insphere, "position", _emitterPositionDefinition.start);
                _emitterPositionDefinition.radius = DataUtils::ParseXmlAttribute(*xml_insphere, "radius", _emitterPositionDefinition.radius);
            }
        }
    }
    if(auto* xml_velocity = element.FirstChildElement("velocity"); xml_velocity) {
        DataUtils::ValidateXmlElement(*xml_velocity, "velocity", "", "", "in_line,in_disc,in_cone,in_sphere");
        if(bool has_children = xml_velocity->FirstChildElement() != nullptr; !has_children) {
            _emitterVelocityDefinition.type = EmitterType::Point;
            _velocity = DataUtils::ParseXmlElementText(*xml_velocity, _position);
        } else {
            if(auto* xml_inline = xml_velocity->FirstChildElement("in_line"); xml_inline) {
                DataUtils::ValidateXmlElement(*xml_inline, "in_line", "", "start,end");
                _emitterVelocityDefinition.type = EmitterType::Line;
                _emitterVelocityDefinition.start = DataUtils::ParseXmlAttribute(*xml_inline, "start", _emitterVelocityDefinition.start);
                _emitterVelocityDefinition.end = DataUtils::ParseXmlAttribute(*xml_inline, "end", _emitterVelocityDefinition.end);
            }
            if(auto* xml_indisc = xml_velocity->FirstChildElement("in_disc"); xml_indisc) {
                DataUtils::ValidateXmlElement(*xml_indisc, "in_disc", "", "normal,radius");
                _emitterVelocityDefinition.type = EmitterType::Disc;
                _emitterVelocityDefinition.normal = DataUtils::ParseXmlAttribute(*xml_indisc, "normal", _emitterVelocityDefinition.normal);
                _emitterVelocityDefinition.radius = DataUtils::ParseXmlAttribute(*xml_indisc, "radius", _emitterVelocityDefinition.radius);
            }
            if(auto* xml_incone = xml_velocity->FirstChildElement("in_cone"); xml_incone) {
                DataUtils::ValidateXmlElement(*xml_incone, "in_cone", "", "normal,length,theta");
                _emitterVelocityDefinition.type = EmitterType::Cone;
                _emitterVelocityDefinition.normal = DataUtils::ParseXmlAttribute(*xml_incone, "normal", _emitterVelocityDefinition.normal);
                _emitterVelocityDefinition.length = DataUtils::ParseXmlAttribute(*xml_incone, "length", _emitterVelocityDefinition.length);
                _emitterVelocityDefinition.theta = DataUtils::ParseXmlAttribute(*xml_incone, "theta", _emitterVelocityDefinition.theta);
            }
            if(auto* xml_insphere = xml_velocity->FirstChildElement("in_sphere"); xml_insphere) {
                DataUtils::ValidateXmlElement(*xml_insphere, "in_sphere", "", "position,radius");
                _emitterVelocityDefinition.type = EmitterType::Sphere;
                _emitterVelocityDefinition.start = DataUtils::ParseXmlAttribute(*xml_insphere, "position", _emitterVelocityDefinition.start);
                _emitterVelocityDefinition.radius = DataUtils::ParseXmlAttribute(*xml_insphere, "radius", _emitterVelocityDefinition.radius);
            }
        }
    }

    if(auto* xml_acceleration = element.FirstChildElement("acceleration"); xml_acceleration) {
        _acceleration = DataUtils::ParseXmlElementText(*xml_acceleration, _acceleration);
    }

    if(auto* xml_initial_burst = element.FirstChildElement("initial_burst"); xml_initial_burst) {
        _initialBurst = DataUtils::ParseXmlElementText(*xml_initial_burst, _initialBurst);
    }

    if(auto* xml_spawn_per_second = element.FirstChildElement("per_second"); xml_spawn_per_second) {
        _spawnPerSecond = DataUtils::ParseXmlElementText(*xml_spawn_per_second, _spawnPerSecond);
    }

    if(auto* xml_particle_lifetime = element.FirstChildElement("particle_lifetime"); xml_particle_lifetime) {
        _particleLifetime = DataUtils::ParseXmlElementText(*xml_particle_lifetime, _particleLifetime);
    }

    if(auto* xml_color = element.FirstChildElement("color"); xml_color) {
        DataUtils::ValidateXmlElement(*xml_color, "color", "", "", "linear");
        if(bool has_children = DataUtils::GetChildElementCount(*xml_color, "linear"); !has_children) {
            Rgba color = DataUtils::ParseXmlElementText(*xml_color, Rgba::White);
            _particleRenderState.SetColors(color, color);
        } else {
            auto xml_color_linear = xml_color->FirstChildElement("linear");
            DataUtils::ValidateXmlElement(*xml_color_linear, "linear", "", "start,end");
            Rgba start_color = DataUtils::ParseXmlAttribute(*xml_color_linear, "start", Rgba::White);
            Rgba end_color = DataUtils::ParseXmlAttribute(*xml_color_linear, "end", Rgba::White);
            _particleRenderState.SetColors(start_color, end_color);
        }
    }

    if(auto* xml_scale = element.FirstChildElement("scale"); xml_scale) {
        DataUtils::ValidateXmlElement(*xml_scale, "scale", "", "", "linear");
        if(bool has_children = DataUtils::GetChildElementCount(*xml_scale, "linear"); !has_children) {
            Vector3 scale = DataUtils::ParseXmlElementText(*xml_scale, Vector3::One);
            _particleRenderState.SetScales(scale, scale);
        } else {
            auto xml_scale_linear = xml_scale->FirstChildElement("linear");
            DataUtils::ValidateXmlElement(*xml_scale_linear, "linear", "", "start,end");
            float start = DataUtils::ParseXmlAttribute(*xml_scale_linear, "start", 1.0f);
            float end = DataUtils::ParseXmlAttribute(*xml_scale_linear, "end", 1.0f);
            Vector3 end_scale(end, end, end);
            Vector3 start_scale(start, start, start);
            _particleRenderState.SetScales(start_scale, end_scale);
        }
    }

    if(auto* xml_prewarm = element.FirstChildElement("prewarm"); xml_prewarm) {
        DataUtils::ValidateXmlElement(*xml_prewarm, "prewarm", "", "");
        _isPrewarmed = DataUtils::ParseXmlElementText(*xml_prewarm, _isPrewarmed);
    }

    if(auto* xml_material = element.FirstChildElement("material"); xml_material) {
        DataUtils::ValidateXmlElement(*xml_material, "material", "", "src");
        std::string material_src = DataUtils::ParseXmlAttribute(*xml_material, "src", std::string{"__2D"});
        _materialName = material_src;
    }
}

ParticleEmitterDefinition* ParticleEmitterDefinition::CreateOrGetParticleEmitterDefinition(const std::string& name, const XMLElement& element) noexcept {
    if(auto found_iter = s_particleEmitterDefintions.find(name); found_iter == s_particleEmitterDefintions.end()) {
        return CreateAndRegisterParticleEmitterDefinition(element);
    }
    return GetParticleEmitterDefinition(name);
}

ParticleEmitterDefinition* ParticleEmitterDefinition::CreateAndRegisterParticleEmitterDefinition(const XMLElement& element) noexcept {
    auto def = CreateParticleEmitterDefinition(element);
    const std::string name_copy = def->_name;
    auto* ptr = def.get();
    s_particleEmitterDefintions.insert_or_assign(name_copy, std::move(def));
    return ptr;
}

ParticleEmitterDefinition* ParticleEmitterDefinition::GetParticleEmitterDefinition(const std::string& name) noexcept {
    if(const auto found_iter = s_particleEmitterDefintions.find(name); found_iter != std::end(s_particleEmitterDefintions)) {
        return found_iter->second.get();
    }
    return nullptr;
}

std::unique_ptr<ParticleEmitterDefinition> ParticleEmitterDefinition::CreateParticleEmitterDefinition(const XMLElement& element) noexcept {
    return std::make_unique<ParticleEmitterDefinition>(element);
}
