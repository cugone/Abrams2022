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
    template<typename ServiceInterface, typename NullableName>
    static ServiceInterface* get() noexcept {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Requested type is not a Service.");
        static_assert(std::is_base_of_v<IService, NullableName>, "Requested Null type is not a Service.");
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(m_cs);
            return dynamic_cast<ServiceInterface*>(found->second);
        }
        return GetNullService<NullableName>();
    };
    template<typename ServiceInterface, typename NullableName>
    static const ServiceInterface* const const_get() noexcept {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Requested type is not a Service.");
        static_assert(std::is_base_of_v<IService, NullableName>, "Requested Null type is not a Service.");
        auto requested_typeindex = std::type_index(typeid(ServiceInterface));
        if(const auto found = m_services.find(requested_typeindex); found != std::end(m_services)) {
            std::scoped_lock lock(m_cs);
            return dynamic_cast<const ServiceInterface*>(found->second);
        }
        return GetNullService_const<NullableName>();
    };

    template<typename ServiceInterface, typename NullService>
    static void provide(ServiceInterface& service, NullService& null_service) {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Provided type is not a Service.");
        static_assert(std::is_base_of_v<ServiceInterface, NullService>, "Provided Null type is not a derived from provided Service.");
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
            auto provided_typeindex = std::type_index(typeid(NullService));
            if(auto [it, inserted] = m_NullServices.try_emplace(provided_typeindex, &null_service); !inserted) {
                m_NullServices.erase(it);
                m_NullServices.try_emplace(provided_typeindex, &null_service);
            }
        }
    }

    template<typename ServiceInterface, typename NullService>
    static void revoke() {
        static_assert(std::is_base_of_v<IService, ServiceInterface>, "Provided type is not a Service.");
        static_assert(std::is_base_of_v<ServiceInterface, NullService>, "Provided Null type is not a derived from provided Service.");
        {
            std::scoped_lock lock(m_cs);
            auto provided_typeindex = std::type_index(typeid(ServiceInterface));
            if(auto it = m_services.find(provided_typeindex); it != m_services.end()) {
                it->second = GetNullService<NullService>();
            }
        }
    }


    }
protected:
private:

    template<typename NullServiceInterface>
    static NullServiceInterface* GetNullService() noexcept {
        const auto requested_typeindex = std::type_index(typeid(NullServiceInterface));
        if(const auto found = m_NullServices.find(requested_typeindex); found != std::end(m_NullServices)) {
            return dynamic_cast<NullServiceInterface*>(found->second);
        }
        return nullptr;
    }

    template<typename NullServiceInterface>
    static const NullServiceInterface* GetNullService_const() noexcept {
        const auto requested_typeindex = std::type_index(typeid(NullServiceInterface));
        if(const auto found = m_NullServices.find(requested_typeindex); found != std::end(m_NullServices)) {
            return dynamic_cast<const NullServiceInterface*>(found->second);
        }
        return nullptr;
    }

    static inline std::unordered_map<std::type_index, IService*> m_services = std::unordered_map<std::type_index, IService*>{};
    static inline std::unordered_map<std::type_index, IService*> m_NullServices = std::unordered_map<std::type_index, IService*>{};
    static inline std::mutex m_cs = std::mutex{};
};
