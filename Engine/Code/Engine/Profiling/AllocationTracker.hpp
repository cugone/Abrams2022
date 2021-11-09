#pragma once
//https://www.youtube.com/watch?v=e2ZQyYr0Oi0
//C++17 - The Best Features - Nicolai Josuttis [ACCU 2018]

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include <array>
#include <charconv>
#include <cstring>
#include <iostream>
#include <new>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

class AllocationTracker {
public:
    struct status_t {
        std::size_t leaked_objs = 0u;
        std::size_t leaked_bytes = 0u;
        operator bool() const noexcept {
            return leaked_objs || leaked_bytes;
        }
        friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const status_t& s) noexcept {
#ifdef TRACK_MEMORY
            static std::array<char, 25> obj_count{"%f"};
            std::to_chars_result count_result;
            if(count_result = std::to_chars(obj_count.data(), obj_count.data() + obj_count.size(), s.leaked_objs);
               count_result.ec == std::errc::value_too_large) {
                DebuggerPrintf("Memory profiler could not convert total leaked objects for printing: Value too large.\n");
                return os;
            }
            static std::array<char, 25> obj_bytes{"%f"};
            std::to_chars_result size_result;
            if(size_result = std::to_chars(obj_bytes.data(), obj_bytes.data() + obj_bytes.size(), s.leaked_bytes);
               size_result.ec == std::errc::value_too_large) {
                DebuggerPrintf("Memory profiler could not convert total leaked bytes value for printing: Value too large.\n");
                return os;
            }
            os << std::string_view{"Leaked objects "} << std::string_view(obj_count.data(), count_result.ptr - obj_count.data()) << std::string_view{" for "} << std::string_view(obj_bytes.data(), size_result.ptr - obj_bytes.data()) << std::string_view{" bytes."};
#endif
            return os;
        }
    };
    struct status_frame_t {
        std::size_t frame_id = 0u;
        std::size_t leaked_objs = 0u;
        std::size_t leaked_bytes = 0u;
        operator bool() const noexcept {
            return leaked_objs || leaked_bytes;
        }
        friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const status_frame_t& s) noexcept {
#ifdef TRACK_MEMORY
            static std::array<char, 25> frame_id{"%f"};
            std::to_chars_result frame_result;
            if(frame_result = std::to_chars(frame_id.data(), frame_id.data() + frame_id.size(), s.frame_id);
               frame_result.ec == std::errc::value_too_large) {
                DebuggerPrintf("Memory profiler could not convert frame id value for printing: Value too large.\n", frame_result.ec);
                return os;
            }
            static std::array<char, 25> obj_count{"%f"};
            std::to_chars_result objects_result;
            if(objects_result = std::to_chars(obj_count.data(), obj_count.data() + obj_count.size(), s.leaked_objs);
               objects_result.ec == std::errc::value_too_large) {
                DebuggerPrintf("Memory profiler could not convert frame leaked objects value for printing: Value too large.\n", objects_result.ec);
                return os;
            }
            static std::array<char, 25> bytes_count{"%f"};
            std::to_chars_result bytes_result;
            if(bytes_result = std::to_chars(bytes_count.data(), bytes_count.data() + bytes_count.size(), s.leaked_bytes);
               bytes_result.ec == std::errc::value_too_large) {
                DebuggerPrintf("Memory profiler could not convert frame leaked bytes value for printing: Value too large.\n", bytes_result.ec);
                return os;
            }
            os << std::string_view{"Frame "} << std::string_view(frame_id.data(), frame_result.ptr - frame_id.data()) << std::string_view{": Leaked objects "} << std::string_view(obj_count.data(), objects_result.ptr - obj_count.data()) << std::string_view{" for "} << std::string_view(bytes_count.data(), bytes_result.ptr - bytes_count.data()) << std::string_view{" bytes."};
#endif
            return os;
        }
    };

    [[nodiscard]] static void* allocate(std::size_t n) noexcept {
        if(is_enabled()) {
            ++frameCount;
            frameSize += n;
            ++allocCount;
            allocSize += n;
            if(maxSize < allocSize) {
                maxSize = allocSize;
            }
            if(maxCount < allocCount) {
                maxCount = allocCount;
            }
        }
        return std::malloc(n);
    }

    static void deallocate(void* ptr, std::size_t size) noexcept {
        if(is_enabled()) {
            ++framefreeCount;
            framefreeSize += size;
            ++freeCount;
            freeSize += size;
        }
        std::free(ptr);
    }

    static void enable([[maybe_unused]] bool e) noexcept {
#ifdef TRACK_MEMORY
        _active = e;
        if(_active) {
            resetallcounters();
        }
#endif
    }

    [[nodiscard]] static bool is_enabled() noexcept {
#ifdef TRACK_MEMORY
        return _active;
#else
        return false;
#endif
    }

    static void trace([[maybe_unused]] bool doTrace) noexcept {
#ifdef TRACK_MEMORY
        _trace = doTrace;
#endif
    }

    static void tick() noexcept {
#ifdef TRACK_MEMORY
        if(is_enabled()) {
            if(auto f = AllocationTracker::frame_status()) {
                std::cout << f << '\n';
            }
            ++frameCounter;
            resetframecounters();
        }
#endif
    }

    static void resetframecounters() noexcept {
#ifdef TRACK_MEMORY
        frameSize = 0u;
        frameCount = 0u;
        framefreeCount = 0u;
        framefreeSize = 0u;
#endif
    }

    static void resetstatuscounters() noexcept {
#ifdef TRACK_MEMORY
        maxSize = 0u;
        maxCount = 0u;
        allocSize = 0u;
        allocCount = 0u;
        freeCount = 0u;
        freeSize = 0u;
#endif
    }

    static void resetallcounters() noexcept {
#ifdef TRACK_MEMORY
        resetframecounters();
        resetstatuscounters();
#endif
    }

    [[nodiscard]] static status_t status() noexcept {
        return {allocCount - freeCount, allocSize - freeSize};
    }

    [[nodiscard]] static status_frame_t frame_status() noexcept {
        return {frameCounter, frameCount - framefreeCount, frameSize - framefreeSize};
    }

    inline static std::size_t maxSize = 0u;
    inline static std::size_t maxCount = 0u;
    inline static std::size_t allocSize = 0u;
    inline static std::size_t allocCount = 0u;
    inline static std::size_t frameSize = 0u;
    inline static std::size_t frameCount = 0u;
    inline static std::size_t frameCounter = 0u;
    inline static std::size_t freeCount = 0u;
    inline static std::size_t freeSize = 0u;
    inline static std::size_t framefreeCount = 0u;
    inline static std::size_t framefreeSize = 0u;

protected:
private:
    inline static bool _active = false;
    inline static bool _trace = false;
};

#ifdef TRACK_MEMORY

    #pragma warning(push)
    #pragma warning(disable : 28251)

void* operator new(std::size_t size);
void* operator new[](std::size_t size);
void operator delete(void* ptr, std::size_t size) noexcept;
void operator delete[](void* ptr, std::size_t size) noexcept;

    #pragma warning(pop)

#endif
