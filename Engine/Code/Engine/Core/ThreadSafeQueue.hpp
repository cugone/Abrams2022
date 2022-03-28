#pragma once

#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
public:
    void push(const T& t) noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        m_queue.push(t);
    }
    void pop() noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        m_queue.pop();
    }

    template<class... Args>
    decltype(auto) emplace(Args&&... args) {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.emplace(std::forward<Args>(args)...);
    }

    [[nodiscard]] decltype(auto) size() const noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.empty();
    }

    [[nodiscard]] T& back() const noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.back();
    }

    [[nodiscard]] T& back() noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.back();
    }

    [[nodiscard]] T& front() const noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.front();
    }

    [[nodiscard]] T& front() noexcept {
        std::scoped_lock<std::mutex> lock(m_cs);
        return m_queue.front();
    }

    void swap(ThreadSafeQueue<T>& b) noexcept {
        std::scoped_lock<std::mutex, std::mutex> lock(m_cs, b.m_cs);
        m_queue.swap(b.m_queue);
    }

protected:
private:
    mutable std::mutex m_cs{};
    std::queue<T> m_queue{};
};
