#include "aosh/shell.h"
#include "aosh/parser.h"
#include "aosh/color.h"
#include "aosh/version.h"
#include "aosh/commands/echo.h"
#include "aosh/commands/filesystem.h"
#include "aosh/commands/help.h"
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace aosh {

#ifdef _WIN32
// Fully raw line reader for Windows console.
// All console modes (echo, line editing, processed input) are disabled.
// We handle echo, backspace, Enter, and Ctrl+C manually so nothing leaks.
static bool win_read_line(std::string& out) {
    out.clear();
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // Save and set raw mode — no echo, no line edit, no Ctrl+C signal
    DWORD orig_mode = 0;
    GetConsoleMode(hIn, &orig_mode);
    SetConsoleMode(hIn, 0);

    while (true) {
        INPUT_RECORD rec;
        DWORD count = 0;
        if (!ReadConsoleInputW(hIn, &rec, 1, &count) || count == 0) {
            SetConsoleMode(hIn, orig_mode);
            return false;
        }

        // Only care about key-down events
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) {
            continue;
        }

        wchar_t wch = rec.Event.KeyEvent.uChar.UnicodeChar;
        WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;

        // Ctrl+C
        if (wch == L'\x03' || (vk == 'C' && (rec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))) {
            // Erase the typed text visually
            if (!out.empty()) {
                CONSOLE_SCREEN_BUFFER_INFO info;
                GetConsoleScreenBufferInfo(hOut, &info);
                COORD pos = info.dwCursorPosition;
                pos.X -= static_cast<SHORT>(out.size());
                SetConsoleCursorPosition(hOut, pos);
                DWORD written;
                FillConsoleOutputCharacterA(hOut, ' ', static_cast<DWORD>(out.size()), pos, &written);
                SetConsoleCursorPosition(hOut, pos);
            }
            out.clear();
            // Print ^C and newline
            DWORD written;
            WriteConsoleA(hOut, "^C\r\n", 4, &written, nullptr);
            SetConsoleMode(hIn, orig_mode);
            return true; // not EOF, just cancelled
        }

        // Enter — don't echo newline, the clear_screen will reset the view
        if (wch == L'\r' || wch == L'\n') {
            SetConsoleMode(hIn, orig_mode);
            return true;
        }

        // Backspace
        if (wch == L'\b' || vk == VK_BACK) {
            if (!out.empty()) {
                out.pop_back();
                // Move cursor back, overwrite with space, move back again
                DWORD written;
                WriteConsoleA(hOut, "\b \b", 3, &written, nullptr);
            }
            continue;
        }

        // Ignore non-printable / special keys (arrows, F-keys, etc.)
        if (wch == 0 || wch < L' ') {
            continue;
        }

        // Normal character — echo and append
        char ch = static_cast<char>(wch);
        out += ch;
        DWORD written;
        WriteConsoleA(hOut, &ch, 1, &written, nullptr);
    }
}
#endif

static void clear_screen() {
    // \033[2J  — clear visible screen
    // \033[3J  — clear scrollback buffer (Windows Terminal, modern terminals)
    // \033[H   — move cursor to top-left
    std::cout << "\033[2J\033[3J\033[H" << std::flush;
}

Shell::Shell() {
    color::init();

    commands::register_echo(registry_);
    commands::register_filesystem(registry_);
    commands::register_help(registry_);

    registry_.add("pwd", "Print current working directory", [](const std::vector<std::string>&) {
        std::cout << std::filesystem::current_path().string() << "\n";
    });

    registry_.add("clear", "Clear the screen", [](const std::vector<std::string>&) {
        clear_screen();
    });

    registry_.add("exit", "Exit the shell", [this](const std::vector<std::string>&) {
        running_ = false;
    });
}

void Shell::run() {
    clear_screen();
    std::cout << color::bold << color::cyan << "aosh" << color::reset
              << " v" << AOSH_VERSION << "\nType 'help' for available commands.\n\n";

    std::string line;
    while (running_) {
        std::cout << color::bold << color::cyan << "aosh " << color::reset
                  << std::filesystem::current_path().string() << "> "
                  << std::flush;

#ifdef _WIN32
        if (!win_read_line(line)) {
            break;
        }
#else
        if (!std::getline(std::cin, line)) {
            break;
        }
#endif
        if (!line.empty()) {
            std::cout << "\n";
            execute(line);
        }
    }
}

void Shell::execute(const std::string& input) {
    auto [name, args] = parser::parse(input);

    auto* cmd = registry_.find(name);
    if (!cmd) {
        std::cerr << color::red << "aosh: unknown command: " << name
                  << color::reset << "\n";
        return;
    }
    // 'clear' already clears the screen, skip the default clear-before-prompt
    cmd->handler(args);
}

} // namespace aosh
