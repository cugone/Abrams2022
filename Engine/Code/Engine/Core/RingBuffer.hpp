#pragma once

#include <array>
#include <utility>

struct RingBufferDesc {
    bool NoWriteOnFull{false};
};

template<typename T, std::size_t Size>
class RingBuffer {
public:
    explicit RingBuffer(RingBufferDesc desc = RingBufferDesc{})
    : m_desc{desc}
    {
        /* DO NOTHING */
    }

    RingBuffer(const RingBuffer& other) = default;
    RingBuffer(RingBuffer&& other) = default;
    RingBuffer& operator=(const RingBuffer& other) = default;
    RingBuffer& operator=(RingBuffer&& other) = default;
    ~RingBuffer() = default;


    void Push(const T& object) noexcept {
        if(m_desc.NoWriteOnFull && full()) {
            return;
        }
        *m_tailIdx = object;
        IncrementTail();
    }
    void Push(T&& object) noexcept {
        if(m_desc.NoWriteOnFull && full()) {
            return;
        }
        *m_tailIdx = std::move(object);
        IncrementTail();
    }

    std::pair<bool, T*> Get() noexcept {
        if(empty()) {
            return {false, nullptr};
        }
        auto next = std::pair{true, &m_ringbuffer[m_headIdx]};
        IncrementHead();
        return next;
    }

    bool empty() const noexcept {
        return m_headIdx == m_tailIdx;
    }
    bool full() const noexcept {
        return GetNext(m_tailIdx) == m_headIdx;
    }

    auto begin() noexcept {
        return m_headIdx;
    }
    auto end() noexcept {
        return m_tailIdx;
    }
    
    auto cbegin() noexcept {
        return m_headIdx;
    }
    auto cend() noexcept {
        return m_tailIdx;
    }

protected:

private:
    auto GetNext(std::array<T, Size>::iterator iter) const noexcept {
        return std::next(iter) == std::cend(m_ringbuffer) ? std::cbegin(m_ringbuffer) : std::next(iter);
    }
    auto GetNext(std::array<T, Size>::iterator iter) noexcept {
        return std::next(iter) == std::end(m_ringbuffer) ? std::begin(m_ringbuffer) : std::next(iter);
    }
    void IncrementHead() noexcept {
        m_headIdx = GetNext(m_headIdx);
    }
    void IncrementTail() noexcept {
        m_tailIdx = GetNext(m_tailIdx);
    }
    std::array<T, Size> m_ringbuffer;
    std::array<T, Size>::iterator m_headIdx = std::begin(m_ringbuffer);
    std::array<T, Size>::iterator m_tailIdx = std::begin(m_ringbuffer);
    RingBufferDesc m_desc{};
};
