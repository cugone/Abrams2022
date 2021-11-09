#include "Engine/Profiling/StackTrace.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string_view>

#ifdef PROFILE_BUILD
    #include <DbgHelp.h>

static constexpr auto MAX_FILENAME_LENGTH = 1024u;
static constexpr auto MAX_CALLSTACK_LINES = 128ul;
static constexpr auto MAX_CALLSTACK_STR_LENGTH = 2048u;
static constexpr auto MAX_SYMBOL_NAME_LENGTH = 128u;
static constexpr auto MAX_DEPTH = 128u;
static constexpr auto SYMBOL_INFO_SIZE = sizeof(SYMBOL_INFO);

using SymSetOptions_t = bool(__stdcall*)(DWORD SymOptions);
using SymInitialize_t = bool(__stdcall*)(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess);
using SymCleanup_t = bool(__stdcall*)(HANDLE hProcess);
using SymFromAddr_t = bool(__stdcall*)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);
using SymGetLineFromAddr64_t = bool(__stdcall*)(HANDLE hProcess, DWORD64 qwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line64);

static HMODULE debugHelpModule;
static HANDLE process;
static SYMBOL_INFO* symbol = nullptr;
static SymSetOptions_t LSymSetOptions;
static SymInitialize_t LSymInitialize;
static SymFromAddr_t LSymFromAddr;
static SymGetLineFromAddr64_t LSymGetLineFromAddr64;
static SymCleanup_t LSymCleanup;

#endif

std::atomic_uint64_t StackTrace::_refs(0);
std::shared_mutex StackTrace::_cs{};
std::atomic_bool StackTrace::_did_init(false);

StackTrace::StackTrace() noexcept
: StackTrace(1ul, 30ul) {
    /* DO NOTHING */
}

StackTrace::StackTrace([[maybe_unused]] unsigned long framesToSkip,
                       [[maybe_unused]] unsigned long framesToCapture) noexcept {
#ifdef PROFILE_BUILD
    if(!_refs) {
        Initialize();
    }
    ++_refs;
    unsigned long count = ::CaptureStackBackTrace(1ul + framesToSkip, framesToCapture, _frames.data(), &hash);
    if(!count) {
        DebuggerPrintf("StackTrace unavailable. All frames were skipped.\n");
        return;
    }
    _frame_count = (std::min)(count, MAX_FRAMES_PER_CALLSTACK);

    GetLines(this, MAX_CALLSTACK_LINES);
#else
    DebuggerPrintf("StackTrace unavailable. Attempting to call StackTrace in non-profile-able build. \n");
#endif
}

StackTrace::~StackTrace() noexcept {
#ifdef PROFILE_BUILD
    --_refs;
    if(!_refs) {
        Shutdown();
    }
#endif
}

void StackTrace::Initialize() noexcept {
#ifdef PROFILE_BUILD
    debugHelpModule = ::LoadLibraryA("DbgHelp.dll");
    GUARANTEE_OR_DIE(debugHelpModule, "Could not initialize StackTrace. Missing required DLL!\n");

    LSymSetOptions = reinterpret_cast<SymSetOptions_t>(::GetProcAddress(debugHelpModule, "SymSetOptions"));
    LSymInitialize = reinterpret_cast<SymInitialize_t>(::GetProcAddress(debugHelpModule, "SymInitialize"));
    LSymFromAddr = reinterpret_cast<SymFromAddr_t>(::GetProcAddress(debugHelpModule, "SymFromAddr"));
    LSymGetLineFromAddr64 = reinterpret_cast<SymGetLineFromAddr64_t>(::GetProcAddress(debugHelpModule, "SymGetLineFromAddr64"));
    LSymCleanup = reinterpret_cast<SymCleanup_t>(::GetProcAddress(debugHelpModule, "SymCleanup"));

    {
        std::scoped_lock<std::shared_mutex> lock(_cs);
        LSymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    }
    process = ::GetCurrentProcess();
    {
        std::scoped_lock<std::shared_mutex> lock(_cs);
        if(!_did_init) {
            _did_init = LSymInitialize(process, nullptr, true);
        }
    }
    if(!_did_init) {
        --_refs;
        Shutdown();
        ERROR_AND_DIE("Could not initialize StackTrace!\n");
    }
    symbol = (SYMBOL_INFO*)std::malloc(SYMBOL_INFO_SIZE + MAX_FILENAME_LENGTH * sizeof(char));
    GUARANTEE_OR_DIE(symbol, "Failed to allocate symbol memory.");

    symbol->MaxNameLen = MAX_FILENAME_LENGTH;
    symbol->SizeOfStruct = SYMBOL_INFO_SIZE;
#endif
}

void StackTrace::GetLines([[maybe_unused]] StackTrace* st,
                          [[maybe_unused]] unsigned long max_lines) noexcept {
#ifndef PROFILE_BUILD
    return;
#else
    IMAGEHLP_LINE64 line_info{};
    DWORD line_offset = 0;
    line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    uint32_t count = std::min(max_lines, st->_frame_count);
    if(!count) {
        DebuggerPrintf("StackTrace unavailable. No stack to trace.\n");
        return;
    }
    for(uint32_t i = 0; i < count; ++i) {
        auto ptr = reinterpret_cast<DWORD64>(st->_frames[i]);
        bool got_addr = false;
        {
            std::scoped_lock<std::shared_mutex> lock(_cs);
            got_addr = LSymFromAddr(process, ptr, nullptr, symbol);
        }
        if(!got_addr) {
            DebuggerPrintf("StackTrace unavailable. No stack to trace.\n");
            return;
        }
        bool got_line = false;
        {
            std::scoped_lock<std::shared_mutex> lock(_cs);
            got_line = LSymGetLineFromAddr64(process, ptr, &line_offset, &line_info);
        }
        if(got_line) {
            const auto name_length_plus_one = static_cast<std::size_t>(symbol->NameLen) + 1u;
            auto* s = static_cast<char*>(std::malloc(name_length_plus_one));
            if(s) {
                ::strcpy_s(s, name_length_plus_one, symbol->Name);
                s[symbol->NameLen] = '\0';
                DebuggerPrintf("\t%s(%d): %s\n", line_info.FileName, line_info.LineNumber, s);
                std::free(s);
            }
        } else {
            const auto name_length_plus_one = static_cast<std::size_t>(symbol->NameLen) + 1u;
            auto* s = static_cast<char*>(std::malloc(name_length_plus_one));
            if(s) {
                ::strcpy_s(s, name_length_plus_one, symbol->Name);
                s[symbol->NameLen] = '\0';
                DebuggerPrintf("\tN/A(%d): %s\n", 0, s);
                std::free(s);
            }
        }
    }
#endif
}

void StackTrace::Shutdown() noexcept {
#ifdef PROFILE_BUILD
    if(symbol) {
        std::free(symbol);
        symbol = nullptr;
    }

    {
        std::scoped_lock<std::shared_mutex> lock(_cs);
        LSymCleanup(process);
    }

    ::FreeLibrary(debugHelpModule);
    debugHelpModule = nullptr;
    _did_init = false;
#endif
}

bool StackTrace::operator!=(const StackTrace& rhs) const noexcept {
    return !(*this == rhs);
}

bool StackTrace::operator==(const StackTrace& rhs) const noexcept {
    return hash == rhs.hash;
}
