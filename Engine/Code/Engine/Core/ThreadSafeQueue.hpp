#pragma once

#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
public:
    void push(const T& t) noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        _queue.push(t);
    }
    void pop() noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        _queue.pop();
    }

    template<class... Args>
    decltype(auto) emplace(Args&&... args) {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.emplace(std::forward<Args>(args)...);
    }

    [[nodiscard]] decltype(auto) size() const noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.empty();
    }

    [[nodiscard]] T& back() const noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.back();
    }

    [[nodiscard]] T& back() noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.back();
    }

    [[nodiscard]] T& front() const noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.front();
    }

    [[nodiscard]] T& front() noexcept {
        std::scoped_lock<std::mutex> lock(_cs);
        return _queue.front();
    }

    void swap(ThreadSafeQueue<T>& b) noexcept {
        std::scoped_lock<std::mutex, std::mutex> lock(_cs, b._cs);
        _queue.swap(b._queue);
    }

protected:
private:
    mutable std::mutex _cs{};
    std::queue<T> _queue{};
};
