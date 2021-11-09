#pragma once

#include "Engine/Services/IService.hpp"

class IAppService : public IService {
public:
    virtual ~IAppService() noexcept {};
    virtual void InitializeService() = 0;
    virtual void RunFrame() = 0;
    [[nodiscard]] virtual bool IsQuitting() const = 0;
    virtual void SetIsQuitting([[maybe_unused]] bool value) = 0;
    [[nodiscard]] virtual bool HasFocus() const = 0;
    [[nodiscard]] virtual bool LostFocus() const = 0;
    [[nodiscard]] virtual bool GainedFocus() const = 0;
protected:
private:
    
};


class NullAppService : public IAppService {
public:
    virtual ~NullAppService() noexcept {};
    virtual void InitializeService() override {};
    virtual void RunFrame() override {};
    [[nodiscard]] virtual bool IsQuitting() const override { return true; };
    virtual void SetIsQuitting([[maybe_unused]] bool value) override {};
    [[nodiscard]] virtual bool HasFocus() const  override { return false; };
    [[nodiscard]] virtual bool LostFocus() const override { return false; };
    [[nodiscard]] virtual bool GainedFocus() const   override { return false; };

protected:
private:
};
