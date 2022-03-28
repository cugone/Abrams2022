#include "Engine/Core/Console.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/Clipboard.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Input/KeyCode.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <iterator>
#include <sstream>
#include <string>
#include <utility>

constexpr const uint16_t IDM_COPY = 0;
constexpr const uint16_t IDM_PASTE = 1;
constexpr const uint16_t IDM_CUT = 2;
constexpr const uint16_t IDM_SELECTALL = 3;
HACCEL hAcceleratorTable{};

void* Console::GetAcceleratorTable() const noexcept {
    return static_cast<void*>(hAcceleratorTable);
}

Console::Console() noexcept
: EngineSubsystem()
, m_show_cursor(false)
, m_is_open(false)
, m_highlight_mode(false)
, m_skip_nonwhitespace_mode(false)
, m_dirty_text(false)
, m_non_rendering_char(false)
, m_entryline_changed(false)
, m_output_changed(false)

{
    ACCEL copy{};
    copy.fVirt = FCONTROL | FVIRTKEY;
    copy.key = ConvertKeyCodeToWinVK(KeyCode::C);
    copy.cmd = IDM_COPY;

    ACCEL paste{};
    paste.fVirt = FCONTROL | FVIRTKEY;
    paste.key = ConvertKeyCodeToWinVK(KeyCode::V);
    paste.cmd = IDM_PASTE;

    ACCEL cut{};
    cut.fVirt = FCONTROL | FVIRTKEY;
    cut.key = ConvertKeyCodeToWinVK(KeyCode::X);
    cut.cmd = IDM_CUT;

    ACCEL select_all{};
    select_all.fVirt = FCONTROL | FVIRTKEY;
    select_all.key = ConvertKeyCodeToWinVK(KeyCode::A);
    select_all.cmd = IDM_SELECTALL;

    std::vector<ACCEL> accelerators = {copy, paste, cut, select_all};
    hAcceleratorTable = ::CreateAcceleratorTableA(accelerators.data(), static_cast<int>(accelerators.size()));
}

Console::~Console() noexcept {
    ::DestroyAcceleratorTable(hAcceleratorTable);
    m_commands.clear();
}

bool Console::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    LPARAM lp = msg.lparam;
    WPARAM wp = msg.wparam;
    switch(msg.wmMessageCode) {
    case WindowsSystemMessage::Menu_SysCommand:
    case WindowsSystemMessage::Menu_Command: {
        bool is_lp_not_valid = lp ? true : false;
        bool is_closed = IsClosed();
        bool is_not_from_accelerator = HIWORD(wp) == 0;
        bool wont_handle = is_closed || is_lp_not_valid || is_not_from_accelerator;
        if(wont_handle) {
            return false;
        }
        auto id = LOWORD(wp);
        switch(id) {
        case IDM_COPY:
            (void)HandleClipboardCopy(); //Return value only useful for Cut operation
            break;
        case IDM_PASTE:
            HandleClipboardPaste();
            break;
        case IDM_CUT:
            HandleClipboardCut();
            break;
        case IDM_SELECTALL:
            HandleSelectAll();
            break;
        }
        return true;
    }
    case WindowsSystemMessage::Keyboard_SysKeyDown: {
        m_non_rendering_char = false;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 1 if the ALT key is down;
        //   0 if the WM_SYSKEYDOWN message is posted to the active window because no window has the keyboard focus.
        //P: previous state: 1 for already down
        //T: transition state: 0 for KEYDOWN
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        if(key < 32 || key == 127) { //Control and Del chars
            m_non_rendering_char = true;
        }
        auto my_key = ConvertWinVKToKeyCode(key);
        if(is_extended_key) {
            if(IsClosed()) {
                return false;
            }
            switch(my_key) {
            case KeyCode::Alt: DebuggerPrintf("Pressing Alt.\n"); return true;
            case KeyCode::LAlt: DebuggerPrintf("Pressing LAlt.\n"); return true;
            case KeyCode::RAlt: DebuggerPrintf("Pressing RAlt.\n"); return true;
            default: return false;
            }
        }
        if(!m_non_rendering_char) {
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Keyboard_KeyDown: {
        m_non_rendering_char = false;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 0 for KEYDOWN
        //P: previous state: 1 for already down
        //T: transition state: 0 for KEYDOWN
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        if(key < 32 || key == 127) { //Control and Del chars
            m_non_rendering_char = true;
        }
        auto my_key = ConvertWinVKToKeyCode(key);
        if(is_extended_key) {
            if(IsClosed()) {
                return false;
            }
            switch(my_key) {
            case KeyCode::Alt: return true;
            case KeyCode::Ctrl: SetSkipNonWhitespaceMode(true); return true;
            //On Num Pad
            case KeyCode::Return: return HandleReturnKey();
            case KeyCode::LWin: return true;
            case KeyCode::RWin: return true;
            case KeyCode::Shift: SetHighlightMode(true); return true;
            //Left Arrow in cluster on 108-key keyboards
            case KeyCode::Left: return HandleLeftKey();
            //Right Arrow in cluster on 108-key keyboards
            case KeyCode::Right: return HandleRightKey();
            //Up Arrow in cluster on 108-key keyboards
            case KeyCode::Up: return HandleUpKey();
            //Down Arrow in cluster on 108-key keyboards
            case KeyCode::Down: return HandleDownKey();
            //Delete key in cluster on 108-key keyboards
            case KeyCode::Del: return HandleDelKey();
            //Nav cluster on 108-key keyboards
            case KeyCode::Home: return HandleHomeKey();
            //Nav cluster in 108-key keyboards
            case KeyCode::End: return HandleEndKey();
            default: return false;
            }
        }
        if(my_key == KeyCode::Tilde)
            return HandleTildeKey();
        if(IsClosed())
            return false;
        switch(my_key) {
        case KeyCode::Return: return HandleReturnKey();
        case KeyCode::Backspace: return HandleBackspaceKey();
        case KeyCode::Del: return HandleDelKey();
        case KeyCode::Escape: return HandleEscapeKey();
        //On Num Pad
        case KeyCode::Up: return HandleUpKey();
        //On Num Pad
        case KeyCode::Down: return HandleDownKey();
        //On Num Pad
        case KeyCode::Left: return HandleLeftKey();
        case KeyCode::Right: return HandleRightKey();
        //on Num Pad
        case KeyCode::Home: return HandleHomeKey();
        //on Num Pad
        case KeyCode::End: return HandleEndKey();
        case KeyCode::Ctrl: SetSkipNonWhitespaceMode(true); return true;
        case KeyCode::Shift: SetHighlightMode(true); return true;
        case KeyCode::Tab: return HandleTabKey();
        case KeyCode::F1: RunCommand(std::string("help ") + m_entryline); return true;
        default: {
            if(!m_non_rendering_char) {
                return true;
            }
            return false;
        }
        }
    }
    case WindowsSystemMessage::Keyboard_Char: {
        if(IsClosed() || m_non_rendering_char) {
            return false;
        }
        m_entryline_changed = false;
        auto char_code = static_cast<unsigned char>(wp);
        //uint32_t lpBits = lp;
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 1 if Alt is already down, 0 otherwise
        //P: previous state: 1 for already down
        //T: transition state: 1 if being released, 0 if being pressed
        //constexpr uint32_t repeat_count_mask     = 0b0000'0000'0000'0000'1111'1111'1111'1111; //0x0000FFFF;
        //constexpr uint32_t scan_code_mask        = 0b0000'0000'1111'1111'0000'0000'0000'0000; //0x00FF0000;
        //constexpr uint32_t extended_key_mask     = 0b0000'0001'0000'0000'0000'0000'0000'0000; //0x01000000;
        //constexpr uint32_t reserved_mask         = 0b0001'1110'0000'0000'0000'0000'0000'0000; //0x1E000000;
        //constexpr uint32_t context_code_mask     = 0b0010'0000'0000'0000'0000'0000'0000'0000; //0x20000000;
        //constexpr uint32_t previous_state_mask   = 0b0100'0000'0000'0000'0000'0000'0000'0000; //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        //bool is_extended_key = (lpBits & extended_key_mask) != 0;
        //bool is_alt_down = (lpBits & context_code_mask) != 0;
        //auto my_key = ConvertWinVKToKeyCode(char_code);
        InsertCharInEntryLine(char_code);
        return true;
    }
    case WindowsSystemMessage::Keyboard_SysKeyUp:
    case WindowsSystemMessage::Keyboard_KeyUp: {
        if(IsClosed()) {
            return false;
        }
        m_entryline_changed = false;
        auto char_code = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 0 for KEYUP
        //P: previous state: 1 for KEYUP
        //T: transition state: 1 for KEYUP
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        //bool is_alt_down = (lpBits & context_code_mask) != 0;
        auto my_key = ConvertWinVKToKeyCode(char_code);
        if(is_extended_key) {
            switch(my_key) {
            case KeyCode::Ctrl: SetSkipNonWhitespaceMode(false); return true;
            case KeyCode::Shift: SetHighlightMode(false); return true;
            //Numpad Enter
            case KeyCode::Return: SetOutputChanged(true); return true;
            default: return false;
            }
        }
        switch(my_key) {
        case KeyCode::Shift: SetHighlightMode(false); return true;
        case KeyCode::Ctrl: SetSkipNonWhitespaceMode(false); return true;
        case KeyCode::Return: SetOutputChanged(true); return true;
        default: return false;
        }
    }
    case WindowsSystemMessage::Mouse_MouseWheel: {
        if(IsClosed()) {
            return false;
        }
        m_mouseWheelPosition = GET_WHEEL_DELTA_WPARAM(wp);
        return true;
    }
    case WindowsSystemMessage::Window_Size: {
    }
    default: {
        return false;
    }
    }
}

bool Console::HandleClipboardCopy() const noexcept {
    bool did_copy = false;
    if(Clipboard::HasText()) {
        auto hwnd = static_cast<HWND>(ServiceLocator::get<IRendererService>().GetOutput()->GetWindow()->GetWindowHandle());
        Clipboard c{hwnd};
        if(m_cursor_position != m_selection_position) {
            std::string copied_text = CopyText(m_cursor_position, m_selection_position);
            did_copy = c.Copy(copied_text);
        } else {
            did_copy = c.Copy(m_entryline);
        }
    }
    return did_copy;
}

void Console::HandleClipboardPaste() noexcept {
    if(Clipboard::HasText()) {
        auto hwnd = static_cast<HWND>(ServiceLocator::get<IRendererService>().GetOutput()->GetWindow()->GetWindowHandle());
        Clipboard c{hwnd};
        auto string_to_paste = c.Paste();
        PasteText(string_to_paste, m_cursor_position);
    }
}

void Console::HandleClipboardCut() noexcept {
    if(HandleClipboardCopy()) {
        RemoveText(m_cursor_position, m_selection_position);
    }
}

void Console::HandleSelectAll() noexcept {
    m_cursor_position = m_entryline.end();
    m_selection_position = m_entryline.begin();
}

bool Console::HandleEscapeKey() noexcept {
    m_entryline.empty() ? Close() : ClearEntryLine();
    return true;
}

bool Console::HandleTabKey() noexcept {
    AutoCompleteEntryline();
    return true;
}

void Console::AutoCompleteEntryline() noexcept {
    if(m_entryline.empty()) {
        return;
    }
    for(const auto& entry : m_commands) {
        if(StringUtils::StartsWith(entry.first, m_entryline)) {
            m_entryline = entry.first;
            MoveCursorToEnd();
        }
    }
}

bool Console::HandleBackspaceKey() noexcept {
    if(m_cursor_position != m_selection_position) {
        RemoveText(m_cursor_position, m_selection_position);
    } else {
        RemoveTextBehindCaret();
    }
    return true;
}

bool Console::HandleUpKey() noexcept {
    HistoryUp();
    return true;
}

bool Console::HandleDownKey() noexcept {
    HistoryDown();
    return true;
}

bool Console::HandleReturnKey() noexcept {
    PostEntryLine();
    return true;
}

bool Console::HandleTildeKey() noexcept {
    ToggleConsole();
    if(IsOpen()) {
        m_mouseWheelPosition = 0;
        m_outputStartPosition = Vector2::Zero;
        m_non_rendering_char = true;
        m_entryline.clear();
        m_cursor_position = m_entryline.begin();
        m_selection_position = m_cursor_position;
    }
    return true;
}

void Console::SetHighlightMode(bool value) noexcept {
    m_highlight_mode = value;
}

void Console::SetOutputChanged(bool value) noexcept {
    m_output_changed = value;
}

void Console::SetSkipNonWhitespaceMode(bool value) noexcept {
    m_skip_nonwhitespace_mode = value;
}

bool Console::HandleEndKey() noexcept {
    const auto offset = std::distance(m_cursor_position, std::cend(m_entryline));
    MoveCursorRight(offset);
    return true;
}

bool Console::HandleHomeKey() noexcept {
    const auto offset = std::distance(std::cbegin(m_entryline), m_cursor_position);
    MoveCursorLeft(offset);
    return true;
}

bool Console::HandleDelKey() noexcept {
    if(m_cursor_position != m_selection_position) {
        RemoveText(m_cursor_position, m_selection_position);
    } else {
        RemoveTextInFrontOfCaret();
    }
    return true;
}

bool Console::HandleRightKey() noexcept {
    const auto offset = std::distance(std::cbegin(m_entryline), m_cursor_position);
    const auto offset_from_next_space = m_entryline.find_first_of(' ', offset);
    MoveCursorRight(m_skip_nonwhitespace_mode ? offset + offset_from_next_space : 1);
    return true;
}

bool Console::HandleLeftKey() noexcept {
    const auto offset = std::distance(std::cbegin(m_entryline), m_cursor_position);
    const auto offset_from_previous_space = m_entryline.find_last_of(' ', offset - 1);
    MoveCursorLeft(m_skip_nonwhitespace_mode ? offset - offset_from_previous_space : 1);
    return true;
}

void Console::RunCommand(const std::string& name_and_args) noexcept {
    if(name_and_args.empty()) {
        return;
    }
    const auto trimmed_name_and_args = StringUtils::TrimWhitespace(name_and_args);
    const auto [command, args] = StringUtils::SplitOnFirst(trimmed_name_and_args, ' ');
    const auto iter = m_commands.find(command);
    if(iter == m_commands.end()) {
        ErrorMsg("INVALID COMMAND");
        return;
    }
    auto* asConsoleCommand = dynamic_cast<Console::Command*>(iter->second.get());
    asConsoleCommand->command_function(args);
}

void Console::RegisterCommand(const ConsoleCommand& command) noexcept {
    const auto& asConsoleCommand = static_cast<const Console::Command&>(command);
    if(asConsoleCommand.command_name.empty()) {
        return;
    }
    const auto iter = m_commands.find(asConsoleCommand.command_name);
    if(iter == m_commands.end()) {
        auto newConsoleCommand = std::make_unique<Console::Command>();
        newConsoleCommand->command_name = asConsoleCommand.command_name;
        newConsoleCommand->help_text_short = asConsoleCommand.help_text_short;
        newConsoleCommand->help_text_long = asConsoleCommand.help_text_long;
        newConsoleCommand->command_function = asConsoleCommand.command_function;
        m_commands.insert_or_assign(asConsoleCommand.command_name, std::move(newConsoleCommand));
    }
}

void Console::UnregisterCommand(const std::string& command_name) noexcept {
    const auto iter = m_commands.find(command_name);
    if(iter != m_commands.end()) {
        m_commands.erase(command_name);
    }
}

void Console::PushCommandList(const ConsoleCommandList& list) noexcept {
    const auto& asConsoleCommandList = static_cast<const Console::CommandList&>(list);
    for(const auto& command : asConsoleCommandList.GetCommands()) {
        RegisterCommand(command);
    }
}

void Console::PopCommandList(const ConsoleCommandList& list) noexcept {
    const auto& asConsoleCommandList = static_cast<const Console::CommandList&>(list);
    for(const auto& command : asConsoleCommandList.GetCommands()) {
        UnregisterCommand(command.command_name);
    }
}

void Console::UnregisterAllCommands() noexcept {
    m_commands.clear();
}

void Console::ToggleConsole() noexcept {
    m_is_open = !m_is_open;
}

bool Console::IsOpen() const noexcept {
    return m_is_open;
}

bool Console::IsClosed() const noexcept {
    return !m_is_open;
}

void Console::Open() noexcept {
    m_is_open = true;
}

void Console::Close() noexcept {
    m_is_open = false;
}

void Console::ToggleHighlightMode() noexcept {
    m_highlight_mode = !m_highlight_mode;
}

bool Console::IsHighlighting() const noexcept {
    return m_highlight_mode;
}

void Console::PostEntryLine() noexcept {
    if(m_entryline.empty()) {
        return;
    }
    PushEntrylineToOutputBuffer();
    PushEntrylineToBuffer();
    RunCommand(m_entryline);
    ClearEntryLine();
}

void Console::PushEntrylineToOutputBuffer() noexcept {
    PrintMsg(m_entryline);
}

void Console::PushEntrylineToBuffer() noexcept {
    const auto already_in_buffer = !m_entryline_buffer.empty() && m_entryline_buffer.back() == m_entryline;
    if(already_in_buffer) {
        return;
    }
    m_entryline_buffer.push_back(m_entryline);
    m_current_history_position = m_entryline_buffer.end();
}

void Console::ClearEntryLine() noexcept {
    m_entryline.clear();
    m_cursor_position = std::begin(m_entryline);
    m_selection_position = std::begin(m_entryline);
}

void Console::MoveCursorLeft(std::string::difference_type distance /*= 1*/) noexcept {
    if(m_cursor_position != m_entryline.begin()) {
        if(!m_highlight_mode) {
            if(std::distance(std::cbegin(m_entryline), m_cursor_position) > distance) {
                m_cursor_position -= distance;
            } else {
                m_cursor_position = std::begin(m_entryline);
            }
            m_selection_position = m_cursor_position;
        } else {
            UpdateSelectedRange(-distance);
        }
    }
}

void Console::MoveCursorRight(std::string::difference_type distance /*= 1*/) noexcept {
    if(m_cursor_position != m_entryline.end()) {
        if(!m_highlight_mode) {
            if(distance < std::distance(m_cursor_position, std::cend(m_entryline))) {
                m_cursor_position += distance;
            } else {
                m_cursor_position = std::end(m_entryline);
            }
            m_selection_position = m_cursor_position;
        } else {
            UpdateSelectedRange(distance);
        }
    }
}

void Console::MoveCursorToEnd() noexcept {
    MoveCursorRight(m_entryline.size() + 1);
}

void Console::MoveCursorToFront() noexcept {
    MoveCursorLeft(m_entryline.size() + 1);
}

void Console::UpdateSelectedRange(std::string::difference_type distance) noexcept {
    if(distance > 0) {
        const auto distance_from_end = std::distance(m_cursor_position, std::cend(m_entryline));
        if(distance_from_end > std::abs(distance)) {
            m_cursor_position += distance;
        } else {
            m_cursor_position = std::end(m_entryline);
        }

        std::string::const_iterator rangeStart = m_cursor_position;
        std::string::const_iterator rangeEnd = m_selection_position;
        if(!m_highlight_mode && m_selection_position < m_cursor_position) {
            rangeStart = m_selection_position;
            rangeEnd = m_cursor_position;
        }
        m_cursor_position = rangeStart;
        m_selection_position = rangeEnd;
    } else if(distance < 0) {
        const auto distance_from_beginning = std::distance(std::cbegin(m_entryline), m_cursor_position);
        if(distance_from_beginning > std::abs(distance)) {
            m_cursor_position += distance;
        } else {
            m_cursor_position = std::begin(m_entryline);
        }

        std::string::const_iterator rangeStart = m_cursor_position;
        std::string::const_iterator rangeEnd = m_selection_position;
        if(!m_highlight_mode && m_selection_position < m_cursor_position) {
            rangeStart = m_selection_position;
            rangeEnd = m_cursor_position;
        }
        m_cursor_position = rangeStart;
        m_selection_position = rangeEnd;
    }
}

void Console::RemoveTextInFrontOfCaret() noexcept {
    if(!m_entryline.empty()) {
        if(m_cursor_position != m_entryline.end()) {
            m_cursor_position = m_entryline.erase(m_cursor_position);
            m_selection_position = m_cursor_position;
            m_entryline_changed = true;
        }
    }
}

void Console::PopConsoleBuffer() noexcept {
    if(!m_entryline.empty()) {
        if(m_cursor_position == m_entryline.end()) {
            m_entryline.pop_back();
            --m_cursor_position;
            m_entryline_changed = true;
        }
    }
}
void Console::RemoveTextBehindCaret() noexcept {
    if(!m_entryline.empty()) {
        if(m_cursor_position != m_entryline.end()) {
            if(m_cursor_position != m_entryline.begin()) {
                m_cursor_position = m_entryline.erase(m_cursor_position - 1);
                m_entryline_changed = true;
            }
        } else {
            PopConsoleBuffer();
        }
        m_selection_position = m_cursor_position;
    }
}

void Console::RemoveText(std::string::const_iterator start, std::string::const_iterator end) noexcept {
    if(end < start) {
        std::swap(start, end);
    }
    m_cursor_position = m_entryline.erase(start, end);
    m_selection_position = m_cursor_position;
    m_entryline_changed = true;
}

std::string Console::CopyText(std::string::const_iterator start, std::string::const_iterator end) const noexcept {
    if(end < start) {
        std::swap(start, end);
    }
    return std::string(start, end);
}

void Console::PasteText(const std::string& text, std::string::const_iterator loc) noexcept {
    if(text.empty()) {
        return;
    }
    if(m_cursor_position != m_selection_position) {
        RemoveText(m_cursor_position, m_selection_position);
    }
    m_cursor_position = m_entryline.insert(loc, std::begin(text), std::end(text)) + text.size();
    m_selection_position = m_cursor_position;
    m_entryline_changed = true;
}

void Console::Initialize() noexcept {
    m_camera = std::make_unique<Camera2D>();
    RegisterDefaultCommands();
}

void Console::RegisterDefaultCommands() noexcept {
    Console::Command help{};
    help.command_name = "help";
    help.help_text_short = "Displays every command with brief description.";
    help.help_text_long = "help [command|string]: Displays command's long description or all commands starting with \'string\'.";
    help.command_function = [this](const std::string& args) -> void {
        ArgumentParser arg_set(args);
        std::string cur_arg{};
        if(arg_set >> cur_arg) { //help ...
            cur_arg = StringUtils::TrimWhitespace(cur_arg);
            const auto found_iter = m_commands.find(cur_arg);
            if(found_iter != m_commands.end()) {
                const auto* asCommand = dynamic_cast<const Console::Command*>(found_iter->second.get());
                PrintMsg(std::string{asCommand->command_name + ": " + asCommand->help_text_short});
                return;
            }
            for(auto& [key, value] : m_commands) {
                if(StringUtils::StartsWith(key, cur_arg)) {
                    const auto* asCommand = dynamic_cast<const Console::Command*>(value.get());
                    PrintMsg(std::string{asCommand->command_name + ": " + asCommand->help_text_short});
                }
            }
        } else {
            for(auto& [_, value] : m_commands) {
                const auto* asCommand = dynamic_cast<const Console::Command*>(value.get());
                PrintMsg(std::string{asCommand->command_name + ": " + asCommand->help_text_short});
            }
        }
    };
    RegisterCommand(help);

    Console::Command echo{};
    echo.command_name = "echo";
    echo.help_text_short = "Displays text as arguments.";
    echo.help_text_long = "echo [text]: Displays text as if they were arguments, each on a separate line.";
    echo.command_function = [this](const std::string& args) -> void {
        ArgumentParser arg_set(args);
        std::string cur_arg{};
        while(arg_set >> cur_arg) {
            PrintMsg(cur_arg);
            cur_arg.clear();
        }
    };
    RegisterCommand(echo);

    Console::Command clear{};
    clear.command_name = "clear";
    clear.help_text_short = "Clears the output buffer.";
    clear.help_text_long = clear.help_text_short;
    clear.command_function = [this](const std::string& /*args*/) -> void {
        m_output_changed = true;
        m_output_buffer.clear();
    };
    RegisterCommand(clear);
}

void Console::BeginFrame() noexcept {
    if(m_cursor_timer.CheckAndReset()) {
        m_show_cursor = !m_show_cursor;
    }
}

void Console::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    /* DO NOTHING */
}

void Console::Render() const noexcept {
    if(IsClosed()) {
        return;
    }
    auto&& renderer = ServiceLocator::get<IRendererService>();
    renderer.ResetModelViewProjection();
    renderer.SetRenderTarget();
    renderer.SetViewportAsPercent(0.0f, 0.0f, 1.0f, 0.957f);

    const auto view_half_extents = SetupViewFromCamera();
    DrawBackground(view_half_extents);
    DrawOutput(view_half_extents);
    DrawEntryLine(view_half_extents);
    DrawCursor(view_half_extents);
}

void Console::DrawCursor(const Vector2& view_half_extents) const noexcept {
    if(!m_show_cursor) {
        return;
    }
    auto&& renderer = ServiceLocator::get<IRendererService>();
    const auto textline_bottom = view_half_extents.y * 0.99f;
    const auto textline_left = -view_half_extents.x * 0.99f;
    const auto font = renderer.GetFont("System32");
    const auto first = m_entryline.begin();
    const auto has_text = !m_entryline.empty();
    const auto text_left_of_cursor = has_text ? std::string(first, m_cursor_position) : std::string("");
    const auto xPosOffsetToCaret = font->CalculateTextWidth(text_left_of_cursor);
    const auto cursor_t = Matrix4::CreateTranslationMatrix(Vector3(textline_left + xPosOffsetToCaret, textline_bottom, 0.0f));
    const auto model_cursor_mat = cursor_t;
    renderer.SetModelMatrix(model_cursor_mat);
    renderer.SetMaterial(font->GetMaterial());
    renderer.DrawTextLine(font, "|", Rgba::White);
}

void Console::DrawOutput(const Vector2& view_half_extents) const noexcept {
    if(m_output_buffer.empty()) {
        return;
    }
    std::vector<Vertex3D> vbo{};
    std::vector<unsigned int> ibo{};
    auto&& renderer = ServiceLocator::get<IRendererService>();
    auto* font = renderer.GetFont("System32");
    const auto max_vertical_start_position = (m_output_buffer.size() * (1 + font->GetLineHeight()) - view_half_extents.y * 2.0f);
    if(m_outputStartPosition.y <= max_vertical_start_position && WasMouseWheelJustScrolledUp()) {
        m_outputStartPosition.y += font->GetLineHeight();
    }
    if(m_outputStartPosition.y && WasMouseWheelJustScrolledDown()) {
        m_outputStartPosition.y -= font->GetLineHeight();
    }
    {
        const auto draw_x = -view_half_extents.x;
        const auto draw_y = view_half_extents.y;
        auto draw_loc = m_outputStartPosition + Vector2(draw_x * 0.99f, draw_y * 0.99f);
        for(auto iter = m_output_buffer.cbegin(); iter != m_output_buffer.cend(); ++iter) {
            draw_loc.y -= font->CalculateTextHeight(iter->str);
            renderer.AppendMultiLineTextBuffer(font, iter->str, draw_loc, iter->color, vbo, ibo);
        }
    }
    renderer.SetMaterial(font->GetMaterial());
    renderer.EnableScissorTest();
    renderer.SetScissorAsPercent(0.0f, 0.0f, 1.0f, 0.921f);
    renderer.SetModelMatrix(Matrix4::I);
    renderer.DrawIndexed(PrimitiveType::Triangles, vbo, ibo);
    renderer.DisableScissorTest();
    renderer.SetScissorAsPercent();
}

void Console::OutputMsg(const std::string& msg, const Rgba& color) noexcept {
    m_output_changed = true;
    m_output_buffer.push_back({msg, color});
}

void Console::HistoryUp() noexcept {
    if(m_current_history_position == m_entryline_buffer.begin()) {
        return;
    }
    --m_current_history_position;
    m_entryline = *m_current_history_position;
    MoveCursorToEnd();
}

void Console::HistoryDown() noexcept {
    if(m_current_history_position != m_entryline_buffer.end()) {
        ++m_current_history_position;
        if(m_current_history_position == m_entryline_buffer.end()) {
            ClearEntryLine();
        } else {
            m_entryline = *m_current_history_position;
        }
    }
    MoveCursorToEnd();
}

void Console::InsertCharInEntryLine(unsigned char c) noexcept {
    m_entryline_changed = true;
    if(!m_entryline.empty()) {
        if(m_cursor_position != m_selection_position) {
            RemoveText(m_cursor_position, m_selection_position);
        }
        if(m_cursor_position == m_entryline.end()) {
            m_entryline.push_back(c);
            m_cursor_position = m_entryline.end();
        } else {
            m_entryline.insert(m_cursor_position, c);
            ++m_cursor_position;
        }
    } else {
        m_entryline += c;
        m_cursor_position = m_entryline.end();
    }
    m_selection_position = m_cursor_position;
}

void Console::PrintMsg(const std::string& msg) noexcept {
    OutputMsg(msg, Rgba::White);
}

void Console::WarnMsg(const std::string& msg) noexcept {
    OutputMsg(msg, Rgba::Yellow);
}

void Console::ErrorMsg(const std::string& msg) noexcept {
    OutputMsg(msg, Rgba::Red);
}

void Console::DrawBackground(const Vector2& view_half_extents) const noexcept {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetModelMatrix(Matrix4::CreateScaleMatrix(view_half_extents * 2.0f));
    renderer.SetMaterial(renderer.GetMaterial("__2D"));
    renderer.DrawQuad2D(Rgba(0, 0, 0, 128));
}

void Console::DrawEntryLine(const Vector2& view_half_extents) const noexcept {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    const auto font = renderer.GetFont("System32");
    const float textline_bottom = view_half_extents.y * 0.99f;
    const float textline_left = -view_half_extents.x * 0.99f;

    const auto entryline_t = Matrix4::CreateTranslationMatrix(Vector3(textline_left, textline_bottom, 0.0f));
    const auto model_entryline_mat = entryline_t;

    if(m_cursor_position != m_selection_position) {
        auto xPosOffsetToCaret = font->CalculateTextWidth(std::string(std::begin(m_entryline), m_cursor_position));
        auto xPosOffsetToSelect = font->CalculateTextWidth(std::string(std::begin(m_entryline), m_selection_position));
        std::string::const_iterator rangeStart = m_cursor_position;
        std::string::const_iterator rangeEnd = m_selection_position;
        if(m_selection_position < m_cursor_position) {
            std::swap(rangeStart, rangeEnd);
            std::swap(xPosOffsetToCaret, xPosOffsetToSelect);
        }

        renderer.SetModelMatrix(Matrix4::CreateScaleMatrix(Vector2(500.0f, 500.0f)));
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        renderer.DrawQuad2D();

        renderer.SetModelMatrix(model_entryline_mat);
        renderer.SetMaterial(font->GetMaterial());

        renderer.DrawTextLine(font, std::string(m_entryline, 0, std::distance(std::cbegin(m_entryline), rangeStart)), Rgba::White);
        auto rightside_t = Matrix4::CreateTranslationMatrix(Vector3(xPosOffsetToSelect, 0.0f, 0.0f));
        rightside_t = Matrix4::MakeRT(model_entryline_mat, rightside_t);
        renderer.SetModelMatrix(rightside_t);
        renderer.DrawTextLine(font, std::string(m_entryline, std::distance(std::cbegin(m_entryline), rangeEnd), std::distance(rangeEnd, std::cend(m_entryline))), Rgba::White);

        const auto xPosOffsetToStart = font->CalculateTextWidth(std::string(std::begin(m_entryline), rangeStart));
        const auto blacktext_t = Matrix4::CreateTranslationMatrix(Vector3(xPosOffsetToStart, 0.0f, 0.0f));
        auto model_mat_blacktext = Matrix4::MakeRT(model_entryline_mat, blacktext_t);
        renderer.SetModelMatrix(model_mat_blacktext);
        renderer.DrawTextLine(font, std::string(rangeStart, rangeEnd), Rgba::Black);

    } else {
        renderer.SetModelMatrix(model_entryline_mat);
        renderer.SetMaterial(font->GetMaterial());
        renderer.DrawTextLine(font, m_entryline, Rgba::White);
    }
}

Vector2 Console::SetupViewFromCamera() const noexcept {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    const auto& window = renderer.GetOutput();
    const auto& window_dimensions = window->GetDimensions();
    const auto& aspect = window->GetAspectRatio();
    const auto window_width = static_cast<float>(window_dimensions.x);
    const auto window_height = static_cast<float>(window_dimensions.y);
    const auto view_half_width = window_width * 0.50f;
    const auto view_half_height = window_height * 0.50f;
    const auto leftBottom = Vector2(-view_half_width, view_half_height);
    const auto rightTop = Vector2(view_half_width, -view_half_height);
    const auto nearFar = Vector2(0.0f, 1.0f);
    m_camera->SetupView(leftBottom, rightTop, nearFar, aspect);

    renderer.SetViewMatrix(m_camera->GetViewMatrix());
    renderer.SetProjectionMatrix(m_camera->GetProjectionMatrix());

    return Vector2(view_half_width, view_half_height);
}

int Console::GetMouseWheelPositionNormalized() const noexcept {
    if(m_mouseWheelPosition) {
        return m_mouseWheelPosition / std::abs(m_mouseWheelPosition);
    }
    return 0;
}

bool Console::WasMouseWheelJustScrolledUp() const noexcept {
    return GetMouseWheelPositionNormalized() > 0;
}

bool Console::WasMouseWheelJustScrolledDown() const noexcept {
    return GetMouseWheelPositionNormalized() < 0;
}

void Console::EndFrame() noexcept {
    m_mouseWheelPosition = 0;
}

Console::CommandList::CommandList(Console* console /*= nullptr*/) noexcept
: m_console(console) {
    /* DO NOTHING */
}

Console::CommandList::CommandList(Console* console, const std::vector<Command>& commands) noexcept
: m_console(console)
, m_commands(commands) {
    if(!m_console) {
        return;
    }
    for(const auto& command : m_commands) {
        m_console->RegisterCommand(command);
    }
}

Console::CommandList::~CommandList() noexcept {
    if(!m_console) {
        return;
    }
    for(const auto& command : m_commands) {
        m_console->UnregisterCommand(command.command_name);
    }
}

void Console::CommandList::AddCommand(const Command& command) {
    m_commands.emplace_back(command);
}

void Console::CommandList::RemoveCommand(const std::string& name) {
    m_commands.erase(std::remove_if(std::begin(m_commands), std::end(m_commands), [&name](const Console::Command& command) { return name == command.command_name; }), std::end(m_commands));
}

void Console::CommandList::RemoveAllCommands() noexcept {
    m_commands.clear();
}

const std::vector<Console::Command>& Console::CommandList::GetCommands() const noexcept {
    return m_commands;
}
