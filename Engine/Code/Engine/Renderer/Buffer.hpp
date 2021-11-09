#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

template<typename T>
class Buffer {
public:
    using buffer_t = T;
    virtual ~Buffer() noexcept = 0;
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> GetDxBuffer() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;

protected:
    Microsoft::WRL::ComPtr<ID3D11Buffer> _dx_buffer{};

private:
};

template<typename T>
Buffer<T>::~Buffer() noexcept {
    if(IsValid()) {
        _dx_buffer.Reset();
        _dx_buffer = nullptr;
    }
}

template<typename T>
Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer<T>::GetDxBuffer() const noexcept {
    return _dx_buffer;
}

template<typename T>
bool Buffer<T>::IsValid() const noexcept {
    return _dx_buffer != nullptr;
}
