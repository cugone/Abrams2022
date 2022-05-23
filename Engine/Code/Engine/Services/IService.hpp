#pragma once

class IService {
public:
    virtual ~IService() noexcept = 0;
};

class NullService : public IService {
    virtual ~NullService() noexcept;
};
