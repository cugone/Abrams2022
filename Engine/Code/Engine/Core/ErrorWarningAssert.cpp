//-----------------------------------------------------------------------------------------------
// ErrorWarningAssert.cpp
//

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <stdarg.h>

//-----------------------------------------------------------------------------------------------
bool IsDebuggerAvailable() noexcept {
#if defined(PLATFORM_WINDOWS)
    typedef BOOL(CALLBACK IsDebuggerPresentFunc)();

    // Get a handle to KERNEL32.DLL
    static HINSTANCE hInstanceKernel32 = GetModuleHandle(TEXT("KERNEL32"));
    if(!hInstanceKernel32)
        return false;

    // Get a handle to the IsDebuggerPresent() function in KERNEL32.DLL
    static IsDebuggerPresentFunc* isDebuggerPresentFunc = (IsDebuggerPresentFunc*)GetProcAddress(hInstanceKernel32, "IsDebuggerPresent");
    if(!isDebuggerPresentFunc)
        return false;

    // Now CALL that function and return its result
    static BOOL isDebuggerAvailable = isDebuggerPresentFunc();
    return (isDebuggerAvailable == TRUE);
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------------------------
void DebuggerPrintf(const std::string& messageFormat) noexcept {
#if defined(PLATFORM_WINDOWS)
    if(IsDebuggerAvailable()) {
        OutputDebugStringA(messageFormat.c_str());
    }
#endif

    std::cout << messageFormat;
}
//-----------------------------------------------------------------------------------------------
// Converts a SeverityLevel to a Windows MessageBox icon type (MB_etc)
//
#if defined(PLATFORM_WINDOWS)
UINT GetWindowsMessageBoxIconFlagForSeverityLevel(SeverityLevel severity) noexcept {
    switch(severity) {
    case SeverityLevel::Information: return MB_ICONASTERISK; // blue circle with 'i' in Windows 7
    case SeverityLevel::Question: return MB_ICONQUESTION;    // blue circle with '?' in Windows 7
    case SeverityLevel::Warning: return MB_ICONEXCLAMATION;  // yellow triangle with '!' in Windows 7
    case SeverityLevel::Fatal: return MB_ICONHAND;           // red circle with 'x' in Windows 7
    default: return MB_ICONEXCLAMATION;
    }
}
#endif

//-----------------------------------------------------------------------------------------------
void SystemDialogue_Okay(const std::string& messageTitle, const std::string& messageText, SeverityLevel severity) noexcept {
#if defined(PLATFORM_WINDOWS)
    {
        ShowCursor(TRUE);
        UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel(severity);
        MessageBoxA(NULL, messageText.c_str(), messageTitle.c_str(), MB_OK | dialogueIconTypeFlag | MB_TOPMOST);
        ShowCursor(FALSE);
    }
#endif
}

//-----------------------------------------------------------------------------------------------
// Returns true if OKAY was chosen, false if CANCEL was chosen.
//
bool SystemDialogue_OkayCancel(const std::string& messageTitle, const std::string& messageText, SeverityLevel severity) noexcept {
    bool isAnswerOkay = true;

#if defined(PLATFORM_WINDOWS)
    {
        ShowCursor(TRUE);
        UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel(severity);
        int buttonClicked = MessageBoxA(NULL, messageText.c_str(), messageTitle.c_str(), MB_OKCANCEL | dialogueIconTypeFlag | MB_TOPMOST);
        isAnswerOkay = (buttonClicked == IDOK);
        ShowCursor(FALSE);
    }
#endif

    return isAnswerOkay;
}

//-----------------------------------------------------------------------------------------------
// Returns true if YES was chosen, false if NO was chosen.
//
bool SystemDialogue_YesNo(const std::string& messageTitle, const std::string& messageText, SeverityLevel severity) noexcept {
    bool isAnswerYes = true;

#if defined(PLATFORM_WINDOWS)
    {
        ShowCursor(TRUE);
        UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel(severity);
        int buttonClicked = MessageBoxA(NULL, messageText.c_str(), messageTitle.c_str(), MB_YESNO | dialogueIconTypeFlag | MB_TOPMOST);
        isAnswerYes = (buttonClicked == IDYES);
        ShowCursor(FALSE);
    }
#endif

    return isAnswerYes;
}

//-----------------------------------------------------------------------------------------------
// Returns 1 if YES was chosen, 0 if NO was chosen, -1 if CANCEL was chosen.
//
int SystemDialogue_YesNoCancel(const std::string& messageTitle, const std::string& messageText, SeverityLevel severity) noexcept {
    int answerCode = 1;

#if defined(PLATFORM_WINDOWS)
    {
        ShowCursor(TRUE);
        UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel(severity);
        int buttonClicked = MessageBoxA(NULL, messageText.c_str(), messageTitle.c_str(), MB_YESNOCANCEL | dialogueIconTypeFlag | MB_TOPMOST);
        answerCode = (buttonClicked == IDYES ? 1 : (buttonClicked == IDNO ? 0 : -1));
        ShowCursor(FALSE);
    }
#endif

    return answerCode;
}

//-----------------------------------------------------------------------------------------------
[[noreturn]] void FatalError(const char* filePath, const char* functionName, int lineNum, const std::string& reasonForError, const char* conditionText) {
    std::string errorMessage = reasonForError;
    if(reasonForError.empty()) {
        if(conditionText)
            errorMessage = std::format("ERROR: \"{}\" is false!", conditionText);
        else
            errorMessage = "Unspecified fatal error";
    }

    const auto fileName = std::filesystem::path(filePath ? filePath : "").filename();
    const auto appName = std::string{"Unnamed Application"};
    const auto fullMessageTitle = appName + " :: Error";
    std::string fullMessageText = errorMessage;
    fullMessageText += "\n\nThe application will now close.\n";
    bool isDebuggerPresent = (IsDebuggerPresent() == TRUE);
    if(isDebuggerPresent) {
        fullMessageText += "\nDEBUGGER DETECTED!\nWould you like to break and debug?\n  (Yes=debug, No=quit)\n";
    }

    fullMessageText += std::format("{:-<10}{:^29}{:->10}", '\n', "Debugging Details Follow", '\n');
    if(conditionText) {
        fullMessageText += std::format("\nThis error was triggered by a run-time condition check:\n  {}\n  from {}(), line {} in {}\n",
                                                conditionText, functionName, lineNum, fileName.string());
    } else {
        fullMessageText += std::format("\nThis was an unconditional error triggered by reaching\n line {} of {}, in {}()\n",
                                                lineNum, fileName.string(), functionName);
    }

    DebuggerPrintf(std::format("\n{:=>80}", '\n'));
    DebuggerPrintf(std::format("RUN-TIME FATAL ERROR on line {} of {}, in {}()\n", lineNum, fileName.string(), functionName));
    DebuggerPrintf(std::format("{}({}): {}\n", filePath, lineNum, errorMessage)); // Use this specific format so Visual Studio users can double-click to jump to file-and-line of error
    DebuggerPrintf(std::format("\n{:=>80}", '\n'));

    if(auto* logger = ServiceLocator::get<IFileLoggerService>(); logger != nullptr) {
        logger->LogError(fullMessageText);
        logger->LogLineAndFlush("Shutting down");
        logger->SaveLog();
    } else {
        std::cout << fullMessageText;
        std::cout << "Shutting down";
    }

    if(isDebuggerPresent) {
        bool isAnswerYes = SystemDialogue_YesNo(fullMessageTitle, fullMessageText, SeverityLevel::Fatal);
        ShowCursor(TRUE);
        if(isAnswerYes) {
            __debugbreak();
        }
    } else {
        SystemDialogue_Okay(fullMessageTitle, fullMessageText, SeverityLevel::Fatal);
        ShowCursor(TRUE);
    }
    exit(0);
}

//-----------------------------------------------------------------------------------------------
void RecoverableWarning(const char* filePath, const char* functionName, int lineNum, const std::string& reasonForWarning, const char* conditionText) noexcept {
    std::string errorMessage = reasonForWarning;
    if(reasonForWarning.empty()) {
        if(conditionText)
            errorMessage = std::format("WARNING: \"{}\" is false!", conditionText);
        else
            errorMessage = "Unspecified warning";
    }

    std::string fileName = std::filesystem::path(filePath ? filePath : "").filename().string();
    std::string appName = "Unnamed Application";
    std::string fullMessageTitle = appName + " :: Warning";
    std::string fullMessageText = errorMessage;

    bool isDebuggerPresent = (IsDebuggerPresent() == TRUE);
    if(isDebuggerPresent) {
        fullMessageText += "\n\nDEBUGGER DETECTED!\nWould you like to continue running?\n  (Yes=continue, No=quit, Cancel=debug)\n";
    } else {
        fullMessageText += "\n\nWould you like to continue running?\n  (Yes=continue, No=quit)\n";
    }

    fullMessageText += "\n---------- Debugging Details Follow ----------\n";
    if(conditionText) {
        fullMessageText += std::format("\nThis warning was triggered by a run-time condition check:\n  {}\n  from {}(), line {} in {}\n",
                                       conditionText, functionName, lineNum, fileName.c_str());
    } else {
        fullMessageText += std::format("\nThis was an unconditional warning triggered by reaching\n line {} of {}, in {}()\n",
                                       lineNum, fileName.c_str(), functionName);
    }

    DebuggerPrintf("\n------------------------------------------------------------------------------\n");
    DebuggerPrintf(std::format("RUN-TIME RECOVERABLE WARNING on line {} of {}, in {}()\n", lineNum, fileName.c_str(), functionName));
    DebuggerPrintf(std::format("{}({}): {}\n", filePath, lineNum, errorMessage.c_str())); // Use this specific format so Visual Studio users can double-click to jump to file-and-line of error
    DebuggerPrintf("------------------------------------------------------------------------------\n\n");

    auto* logger = ServiceLocator::get<IFileLoggerService>();
    if(logger) {
        logger->LogWarnLine(fullMessageText);
    } else {
        std::cout << fullMessageText << '\n';
    }

    if(isDebuggerPresent) {
        int answerCode = SystemDialogue_YesNoCancel(fullMessageTitle, fullMessageText, SeverityLevel::Warning);
        ShowCursor(TRUE);
        if(answerCode == 0) // "NO"
        {
            if(logger) {
                logger->LogLineAndFlush("Shutting down");
                logger->SaveLog();
            } else {
                std::cout << "Shutting down" << std::endl;
            }
            exit(0);
        } else if(answerCode == -1) // "CANCEL"
        {
            if(logger) {
                logger->Flush();
                logger->SaveLog();
            } else {
                std::cout.flush();
            }
            __debugbreak();
        }
    } else {
        bool isAnswerYes = SystemDialogue_YesNo(fullMessageTitle, fullMessageText, SeverityLevel::Warning);
        ShowCursor(TRUE);
        if(!isAnswerYes) {
            if(logger) {
                logger->LogLineAndFlush("Shutting down");
                logger->SaveLog();
            } else {
                std::cout << "Shutting down" << std::endl;
            }
            exit(0);
        }
    }
}
