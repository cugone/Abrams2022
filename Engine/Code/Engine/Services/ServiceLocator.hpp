#pragma once

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Services/IService.hpp"
#include "Engine/Services/IAudioService.hpp"
#include "Engine/Services/IConfigService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IInputService.hpp"
#include "Engine/Services/IJobSystemService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <mutex>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

class ServiceLocator {
public:
    template<typename ServiceInterface>
    static ServiceInterface& get() noexcept {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Requested type is not a Service.");
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(_cs);
            return static_cast<ServiceInterface&>(*found->second);
        }
        ERROR_AND_DIE("Service type not previously provided.");
    };
    template<typename ServiceInterface>
    static const ServiceInterface& const_get() noexcept {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Requested type is not a Service.");
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(const auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(_cs);
            return static_cast<const ServiceInterface&>(*found->second);
        }
        ERROR_AND_DIE("Service type not previously provided.");
    };

    template<typename ServiceInterface>
    static void provide(ServiceInterface& service) {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Provided type is not a Service.");
        std::scoped_lock lock(_cs);
        auto provided_typeindex = std::type_index(typeid(ServiceInterface));
        if(auto [it, result] = m_services.try_emplace(provided_typeindex, &service); !result) {
            m_services.erase(it);
            m_services.try_emplace(provided_typeindex, &service);
        }

    }
protected:
private:
    static inline std::unordered_map<std::type_index, IService*> m_services{};
    static inline std::mutex _cs{};
};
