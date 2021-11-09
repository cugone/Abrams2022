#pragma once

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Renderer/ArrayBuffer.hpp"

#include <vector>

class RHIDevice;
class RHIDeviceContext;

class IndexBuffer : public ArrayBuffer<unsigned int> {
public:
    IndexBuffer(const RHIDevice& owner, const buffer_t& buffer, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept;
    virtual ~IndexBuffer() noexcept;

    void Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept;

protected:
private:
};
