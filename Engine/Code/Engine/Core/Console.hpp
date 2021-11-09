#pragma once

#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Engine/Services/IConsoleService.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

class Camera2D;
class KerningFont;
class Renderer;

class Console : public EngineSubsystem, public IConsoleService {
public:
    struct Command : public ConsoleCommand {
        virtual ~Command() noexcept = default;
        std::string command_name{};
        std::string help_text_short{};
        std::string help_text_long{};
        std::function<void(const std::string& args)> command_function = [](const std::string& /*args*/) {};
    };
    class CommandList : public ConsoleCommandList {
    public:
        explicit CommandList(Console* console = nullptr) noexcept;
        CommandList(Console* console, const std::vector<Command>& commands) noexcept;
        virtual ~CommandList() noexcept;
        void AddCommand(const Command& command);
        void RemoveCommand(const std::string& name);
        void RemoveAllCommands() noexcept;
        [[nodiscard]] const std::vector<Command>& GetCommands() const noexcept;

    private:
        Console* _console = nullptr;
        std::vector<Command> _commands{};
    };
    Console() noexcept;
    Console(const Console& other) = delete;
    Console(Console&& other) = delete;
    Console& operator=(const Console& other) = delete;
    Console& operator=(Console&& other) = delete;
    virtual ~Console() noexcept;

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;
    [[nodiscard]] bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;

    void RunCommand(const std::string& name_and_args) noexcept override;
    void RegisterCommand(const ConsoleCommand& command) noexcept override;
    void UnregisterCommand(const std::string& command_name) noexcept override;

    void PushCommandList(const ConsoleCommandList& list) noexcept override;
    void PopCommandList(const ConsoleCommandList& list) noexcept override;

    void PrintMsg(const std::string& msg) noexcept override;
    void WarnMsg(const std::string& msg) noexcept override;
    void ErrorMsg(const std::string& msg) noexcept override;

    [[nodiscard]] void* GetAcceleratorTable() const noexcept override;
    [[nodiscard]] bool IsOpen() const noexcept override;
    [[nodiscard]] bool IsClosed() const noexcept override;

protected:
private:
    struct OutputEntry {
        std::string str{};
        Rgba color = Rgba::White;
    };
    void PostEntryLine() noexcept;
    void PushEntrylineToOutputBuffer() noexcept;
    void PushEntrylineToBuffer() noexcept;
    void ClearEntryLine() noexcept;
    void MoveCursorLeft(std::string::difference_type distance = 1) noexcept;
    void MoveCursorRight(std::string::difference_type distance = 1) noexcept;
    void MoveCursorToEnd() noexcept;
    void MoveCursorToFront() noexcept;
    void UpdateSelectedRange(std::string::difference_type distance) noexcept;

    [[nodiscard]] bool HandleLeftKey() noexcept;
    [[nodiscard]] bool HandleRightKey() noexcept;
    [[nodiscard]] bool HandleDelKey() noexcept;
    [[nodiscard]] bool HandleHomeKey() noexcept;
    [[nodiscard]] bool HandleEndKey() noexcept;
    [[nodiscard]] bool HandleTildeKey() noexcept;
    [[nodiscard]] bool HandleReturnKey() noexcept;
    [[nodiscard]] bool HandleUpKey() noexcept;
    [[nodiscard]] bool HandleDownKey() noexcept;
    [[nodiscard]] bool HandleBackspaceKey() noexcept;
    [[nodiscard]] bool HandleEscapeKey() noexcept;
    [[nodiscard]] bool HandleTabKey() noexcept;
    [[nodiscard]] bool HandleClipboardCopy() const noexcept;
    void HandleClipboardPaste() noexcept;
    void HandleClipboardCut() noexcept;
    void HandleSelectAll() noexcept;

    void HistoryUp() noexcept;
    void HistoryDown() noexcept;

    void InsertCharInEntryLine(unsigned char c) noexcept;
    void PopConsoleBuffer() noexcept;
    void RemoveTextInFrontOfCaret() noexcept;
    void RemoveTextBehindCaret() noexcept;
    void RemoveText(std::string::const_iterator start, std::string::const_iterator end) noexcept;
    [[nodiscard]] std::string CopyText(std::string::const_iterator start, std::string::const_iterator end) const noexcept;
    void PasteText(const std::string& text, std::string::const_iterator loc) noexcept;
    void DrawBackground(const Vector2& view_half_extents) const noexcept;
    void DrawEntryLine(const Vector2& view_half_extents) const noexcept;
    void DrawCursor(const Vector2& view_half_extents) const noexcept;
    void DrawOutput(const Vector2& view_half_extents) const noexcept;

    void OutputMsg(const std::string& msg, const Rgba& color) noexcept;

    void RegisterDefaultCommands() noexcept;
    void UnregisterAllCommands() noexcept;

    void ToggleConsole() noexcept;
    void Open() noexcept;
    void Close() noexcept;

    void ToggleHighlightMode() noexcept;
    void SetHighlightMode(bool value) noexcept;
    [[nodiscard]] bool IsHighlighting() const noexcept;
    void SetOutputChanged(bool value) noexcept;
    void SetSkipNonWhitespaceMode(bool value) noexcept;

    void AutoCompleteEntryline() noexcept;

    [[nodiscard]] Vector2 SetupViewFromCamera() const noexcept;

    [[nodiscard]] int GetMouseWheelPositionNormalized() const noexcept;

    [[nodiscard]] bool WasMouseWheelJustScrolledUp() const noexcept;
    [[nodiscard]] bool WasMouseWheelJustScrolledDown() const noexcept;

    std::unique_ptr<Camera2D> _camera{};
    std::map<std::string, std::unique_ptr<ConsoleCommand>> _commands{};
    std::vector<std::string> _entryline_buffer{};
    std::vector<OutputEntry> _output_buffer{};
    std::string _entryline{};
    std::string::const_iterator _cursor_position{_entryline.begin()};
    std::string::const_iterator _selection_position{_entryline.begin()};
    decltype(_entryline_buffer)::const_iterator _current_history_position{};
    unsigned int _default_blink_rate = 4u;
    unsigned int _blink_rate = _default_blink_rate;
    int _mouseWheelPosition{0};
    mutable Vector2 _outputStartPosition{};
    Stopwatch _cursor_timer = Stopwatch(_blink_rate);
    uint8_t _show_cursor : 1;
    uint8_t _is_open : 1;
    uint8_t _highlight_mode : 1;
    uint8_t _skip_nonwhitespace_mode : 1;
    uint8_t _dirty_text : 1;
    uint8_t _non_rendering_char : 1;
    uint8_t _entryline_changed : 1;
    uint8_t _output_changed : 1;
};
