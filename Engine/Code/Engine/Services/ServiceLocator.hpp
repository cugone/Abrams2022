#pragma once

#include "Engine/Core/BuildConfig.hpp"
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
    requires(std::derived_from<ServiceInterface, IService>)
    static ServiceInterface* get() noexcept {
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(m_cs);
            return dynamic_cast<ServiceInterface*>(found->second);
        }
        return GetNullService<ServiceInterface>();
    };
    template<typename ServiceInterface>
    requires(std::derived_from<ServiceInterface, IService>)
    static const ServiceInterface* const const_get() noexcept {
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(const auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(m_cs);
            return dynamic_cast<const ServiceInterface*>(found->second);
        }
        return GetNullService_const<ServiceInterface>();
    };

    template<typename ServiceInterface, typename NullService>
    requires(std::derived_from<ServiceInterface, IService> && std::derived_from<NullService, ServiceInterface>)
    static void provide(ServiceInterface& service, NullService& null_service) {
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto [it, inserted] = m_services.try_emplace(provided_typeindex, &service); !inserted) {
                m_services.erase(it);
                m_services.try_emplace(provided_typeindex, &service);
            }
        }
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto [it, inserted] = m_NullServices.try_emplace(provided_typeindex, &null_service); !inserted) {
                m_NullServices.erase(it);
                m_NullServices.try_emplace(provided_typeindex, &null_service);
            }
        }
    }

    template<typename ServiceInterface>
    requires(std::derived_from<ServiceInterface, IService>)
    static void revoke() {
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto it = m_services.find(provided_typeindex); it != m_services.end()) {
                it->second = GetNullService<ServiceInterface>();
            }
        }
    }

    template<typename ServiceInterface>
    requires(std::derived_from<ServiceInterface, IService>)
    static void remove() {
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto it = m_services.find(provided_typeindex); it != m_services.end()) {
                m_services.erase(it);
            }
        }
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto it = m_NullServices.find(provided_typeindex); it != m_NullServices.end()) {
                m_NullServices.erase(it);
            }
        }
    }
    
    static void remove_all() {
        m_services.clear();
        m_NullServices.clear();
    }

protected:
private:

    template<typename ServiceInterface>
    static ServiceInterface* GetNullService() noexcept {
        const auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(const auto found = m_NullServices.find(requested_typeindex); found != std::end(m_NullServices)) {
            return dynamic_cast<ServiceInterface*>(found->second);
        }
        return nullptr;
    }

    template<typename ServiceInterface>
    static const ServiceInterface* GetNullService_const() noexcept {
        const auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(const auto found = m_NullServices.find(requested_typeindex); found != std::end(m_NullServices)) {
            return dynamic_cast<const ServiceInterface*>(found->second);
        }
        return nullptr;
    }

    static inline std::unordered_map<std::type_index, IService*> m_services = std::unordered_map<std::type_index, IService*>{};
    static inline std::unordered_map<std::type_index, IService*> m_NullServices = std::unordered_map<std::type_index, IService*>{};
    static inline std::mutex m_cs = std::mutex{};
};
