#include "Engine/Services/IService.hpp"

IService::~IService() {
    /* DO NOTHING */
}

NullService::~NullService() noexcept {
    /* DO NOTHING */
}
