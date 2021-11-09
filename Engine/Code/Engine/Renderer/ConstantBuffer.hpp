#pragma once

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Renderer/Buffer.hpp"

#include <vector>

class RHIDevice;
class RHIDeviceContext;

class ConstantBuffer : public Buffer<void*> {
public:
    ConstantBuffer(const RHIDevice& owner, const buffer_t& buffer, const std::size_t& buffer_size, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept;
    virtual ~ConstantBuffer() noexcept;

    void Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept;

protected:
private:
    std::size_t _buffer_size = 0;
};
