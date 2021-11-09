#pragma once

#include "Engine/Core/TimeUtils.hpp"

class IGPanel {
public:
    virtual ~IGPanel() noexcept = 0;
    virtual void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept = 0;
protected:
private:
};
