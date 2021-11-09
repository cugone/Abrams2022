#pragma once

#include "Engine/Renderer/Vertex3DInstanced.hpp"
#include "Engine/Renderer/ArrayBuffer.hpp"

#include <vector>

class RHIDevice;
class RHIDeviceContext;

class VertexBufferInstanced : public ArrayBuffer<Vertex3DInstanced> {
public:
    VertexBufferInstanced(const RHIDevice& owner, const buffer_t& buffer, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept;
    virtual ~VertexBufferInstanced() noexcept;

    void Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept;

protected:
private:
};
