#pragma once

#include "Engine/Platform/Win.hpp"

#include <string>

class Clipboard {
public:
    Clipboard() = default;
    explicit Clipboard(void* hwnd) noexcept;
    Clipboard(const Clipboard& other) = default;
    Clipboard(Clipboard&& other) = default;
    Clipboard& operator=(const Clipboard& other) = default;
    Clipboard& operator=(Clipboard&& other) = default;
    ~Clipboard() noexcept;

    [[nodiscard]] bool Open(void* hwnd) noexcept;
    [[nodiscard]] bool IsOpen() const noexcept;
    [[nodiscard]] bool IsClosed() const noexcept;
    [[nodiscard]] static bool HasText() noexcept;
    [[nodiscard]] static bool HasUnicodeText() noexcept;
    [[nodiscard]] bool Copy(const std::string& text) noexcept;
    [[nodiscard]] bool Copy(const std::wstring& text) noexcept;

    [[nodiscard]] std::string Paste() noexcept;
    [[nodiscard]] bool Empty() noexcept;
    void Close() noexcept;

protected:
private:
    template<typename T>
    [[nodiscard]] bool Copy_helper(const T& text);

    HWND _hwnd{};
    bool _is_open{false};
};

template<typename T>
bool Clipboard::Copy_helper(const T& text) {
    bool did_copy = false;
    if(text.empty()) {
        return did_copy;
    }
    if(!HasText()) {
        return did_copy;
    }
    if(Open(_hwnd)) {
        if(Empty()) {
            if(auto hgblcopy = ::GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(T::value_type))) {
                if(auto lpstrcopy = static_cast<LPTSTR>(::GlobalLock(hgblcopy))) {
                    std::memcpy(lpstrcopy, text.data(), text.size() + 1);
                    lpstrcopy[text.size() + 1] = '\0';
                    did_copy = true;
                }
                ::GlobalUnlock(hgblcopy);
                ::SetClipboardData(CF_TEXT, hgblcopy);
            }
        }
        Close();
    }
    return did_copy;
}
