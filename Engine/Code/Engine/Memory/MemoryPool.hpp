#pragma once

#include <cstdlib>

template<typename T, std::size_t maxSize>
class MemoryPool {
public:
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& other) = delete;
    MemoryPool(MemoryPool&& other) = delete;
    MemoryPool& operator=(const MemoryPool& rhs) = delete;
    MemoryPool& operator=(MemoryPool&& rhs) = delete;
    ~MemoryPool() noexcept;

    [[nodiscard]] void* allocate(std::size_t size) noexcept;
    void deallocate(void* ptr, std::size_t size) noexcept;

protected:
private:
    std::size_t _count = 0;
    std::size_t _max = 0;
    T* _data = nullptr;
    T* _ptr = nullptr;
};

template<typename T, std::size_t maxSize>
[[nodiscard]] void* MemoryPool<T, maxSize>::allocate(std::size_t size) noexcept {
    std::size_t elems = size / sizeof(T);
    if(_count + elems < _max) {
        auto front = _ptr;
        _count += elems;
        _ptr += _count;
        return static_cast<void*>(front);
    }
    return nullptr;
}

template<typename T, std::size_t maxSize>
void MemoryPool<T, maxSize>::deallocate(void* ptr, std::size_t size) noexcept {
    const auto elems = static_cast<int>(size / sizeof(T));
    if(0 < _count - elems) {
        _ptr -= elems;
        _count -= elems;
    } else {
        _ptr = _data;
        _count = 0;
    }
}

template<typename T, std::size_t maxSize>
MemoryPool<T, maxSize>::~MemoryPool() noexcept {
    std::free(_data);
    _data = nullptr;
    _ptr = nullptr;
    _count = 0;
    _max = 0;
}

template<typename T, std::size_t maxSize>
MemoryPool<T, maxSize>::MemoryPool() noexcept {
    _data = static_cast<T*>(std::malloc(maxSize * sizeof(T)));
    _ptr = _data;
    _count = 0;
    _max = maxSize;
}
