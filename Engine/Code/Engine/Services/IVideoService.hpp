#pragma once

#include "Engine/Services/IService.hpp"

class RHIVideoDevice;
class RHIVideoContext;
class RHIVideoDecoder;

class IVideoService : public IService {
public:
    virtual ~IVideoService() noexcept = default;

    virtual RHIVideoDevice* GetVideoDevice() noexcept = 0;
    virtual RHIVideoContext* GetVideoContext() noexcept = 0;
    virtual RHIVideoDecoder* GetVideoDecoder() noexcept = 0;

protected:
private:

};

class NullVideoService : public IVideoService {
public:
    NullVideoService() noexcept = default;
    virtual ~NullVideoService() noexcept = default;

    RHIVideoDevice* GetVideoDevice() noexcept override { return nullptr; };
    RHIVideoContext* GetVideoContext() noexcept override { return nullptr; };
    RHIVideoDecoder* GetVideoDecoder() noexcept override { return nullptr; };

protected:
private:
};
