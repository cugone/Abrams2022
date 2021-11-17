#pragma once

#include "Engine/Renderer/VertexCircle2D.hpp"
#include "Engine/Renderer/ArrayBuffer.hpp"

#include <vector>

class RHIDevice;
class RHIDeviceContext;

class VertexCircleBuffer : public ArrayBuffer<VertexCircle2D> {
public:
    VertexCircleBuffer(const RHIDevice& owner, const buffer_t& buffer, const BufferUsage& usage, const BufferBindUsage& bindUsage) noexcept;
    virtual ~VertexCircleBuffer() noexcept;

    void Update(RHIDeviceContext& context, const buffer_t& buffer) noexcept;

protected:
private:
};
