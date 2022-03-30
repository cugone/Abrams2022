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
    std::size_t m_count = 0;
    std::size_t m_max = 0;
    T* m_data = nullptr;
    T* m_ptr = nullptr;
};

template<typename T, std::size_t maxSize>
[[nodiscard]] void* MemoryPool<T, maxSize>::allocate(std::size_t size) noexcept {
    std::size_t elems = size / sizeof(T);
    if(m_count + elems < m_max) {
        auto front = m_ptr;
        m_count += elems;
        m_ptr += m_count;
        return static_cast<void*>(front);
    }
    return nullptr;
}

template<typename T, std::size_t maxSize>
void MemoryPool<T, maxSize>::deallocate(void* ptr, std::size_t size) noexcept {
    const auto elems = static_cast<int>(size / sizeof(T));
    if(0 < m_count - elems) {
        m_ptr -= elems;
        m_count -= elems;
    } else {
        m_ptr = m_data;
        m_count = 0;
    }
}

template<typename T, std::size_t maxSize>
MemoryPool<T, maxSize>::~MemoryPool() noexcept {
    std::free(m_data);
    m_data = nullptr;
    m_ptr = nullptr;
    m_count = 0;
    m_max = 0;
}

template<typename T, std::size_t maxSize>
MemoryPool<T, maxSize>::MemoryPool() noexcept {
    m_data = static_cast<T*>(std::malloc(maxSize * sizeof(T)));
    m_ptr = m_data;
    m_count = 0;
    m_max = maxSize;
}
