#include "aosh/shell.h"
#include "aosh/parser.h"
#include "aosh/color.h"
#include "aosh/version.h"
#include "aosh/commands/echo.h"
#include "aosh/commands/filesystem.h"
#include "aosh/commands/help.h"
#include "aosh/process.h"
#include <algorithm>
#include <conio.h>
#include <filesystem>
#include <iostream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace aosh {

// ---------------------------------------------------------------------------
// Console helpers — use Win32 API for reliable cursor positioning
// ---------------------------------------------------------------------------

static HANDLE h_out() {
    static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    return h;
}

static COORD get_cursor() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h_out(), &info);
    return info.dwCursorPosition;
}

static void set_cursor(COORD pos) {
    SetConsoleCursorPosition(h_out(), pos);
}

static int term_width() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(h_out(), &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
    return 80;
}

// Clear from pos to end of screen
static void clear_from(COORD pos) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h_out(), &info);
    DWORD total = info.dwSize.X * (info.dwSize.Y - pos.Y) - pos.X;
    DWORD written;
    FillConsoleOutputCharacterA(h_out(), ' ', total, pos, &written);
    FillConsoleOutputAttribute(h_out(), info.wAttributes, total, pos, &written);
}

// Write string at current cursor, advancing cursor
static void con_write(const std::string& s) {
    DWORD written;
    WriteConsoleA(h_out(), s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr);
}

// ---------------------------------------------------------------------------
// Candidate box rendering
// ---------------------------------------------------------------------------

struct CandidateBox {
    COORD origin;       // top-left of the candidate area (line below prompt)
    size_t num_lines;   // how many lines the box occupies
    bool visible;

    CandidateBox() : origin{0, 0}, num_lines(0), visible(false) {}

    void draw(const std::vector<std::string>& candidates, size_t active,
              COORD prompt_cursor) {
        int width = term_width();

        size_t max_len = 0;
        for (const auto& c : candidates) max_len = std::max(max_len, c.size());
        size_t col_width = max_len + 2;
        size_t cols = std::max<size_t>(1, static_cast<size_t>(width) / col_width);
        num_lines = (candidates.size() + cols - 1) / cols;

        // Position: one line below the prompt
        origin = {0, static_cast<SHORT>(prompt_cursor.Y + 1)};

        // Make room: scroll if near bottom of visible window
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(h_out(), &info);
        SHORT visible_bottom = info.srWindow.Bottom;
        SHORT needed_bottom = origin.Y + static_cast<SHORT>(num_lines) - 1;
        if (needed_bottom > visible_bottom) {
            // Print enough newlines to scroll the buffer
            set_cursor({0, visible_bottom});
            SHORT scroll = needed_bottom - visible_bottom;
            for (SHORT i = 0; i < scroll; ++i) con_write("\r\n");
            // Adjust all positions after scroll
            origin.Y -= scroll;
            prompt_cursor.Y -= scroll;
        }

        // Clear the candidate area
        set_cursor(origin);
        clear_from(origin);

        // Draw candidates in columns
        set_cursor(origin);
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (i == active) {
                // White background, black text, bold
                con_write("\033[47m\033[30m\033[1m");
            }
            std::string padded = candidates[i];
            if (padded.size() < col_width) padded.resize(col_width, ' ');
            con_write(padded);
            if (i == active) {
                con_write("\033[0m");
            }

            bool end_of_row = ((i + 1) % cols == 0);
            bool last = (i + 1 == candidates.size());
            if (end_of_row && !last) {
                COORD cur = get_cursor();
                set_cursor({0, static_cast<SHORT>(cur.Y + 1)});
            }
        }

        // Restore cursor to prompt line
        set_cursor(prompt_cursor);
        visible = true;
    }

    void hide(COORD prompt_cursor) {
        if (!visible) return;
        // Clear everything from the candidate area down
        clear_from(origin);
        set_cursor(prompt_cursor);
        visible = false;
        num_lines = 0;
    }
};

// ---------------------------------------------------------------------------
// Line reader with Tab completion
// ---------------------------------------------------------------------------

static bool read_line(std::string& out, completion::Completer& completer,
                      const std::string& prompt) {
    out.clear();
    completer.reset();

    CandidateBox box;
    std::string saved_line;     // line before completion started
    COORD line_start = get_cursor();  // cursor at start of user input (after prompt)

    // Compute cursor position at end of current input
    auto input_cursor = [&]() -> COORD {
        COORD c = line_start;
        c.X += static_cast<SHORT>(out.size());
        // Handle line wrap
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(h_out(), &info);
        if (c.X >= info.dwSize.X) {
            c.Y += c.X / info.dwSize.X;
            c.X = c.X % info.dwSize.X;
        }
        return c;
    };

    // Replace input from word_start onward, update screen
    auto replace_word = [&](size_t word_start, const std::string& replacement) {
        // Erase visible input from word_start onward
        COORD erase_pos = line_start;
        erase_pos.X += static_cast<SHORT>(word_start);
        set_cursor(erase_pos);
        // Clear from here to end of line
        DWORD written;
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(h_out(), &info);
        DWORD chars_to_clear = static_cast<DWORD>(out.size() - word_start);
        FillConsoleOutputCharacterA(h_out(), ' ', chars_to_clear, erase_pos, &written);
        // Write replacement
        set_cursor(erase_pos);
        con_write(replacement);
        // Update buffer
        out = out.substr(0, word_start) + replacement;
    };

    while (true) {
        int ch = _getch();

        // Special keys (0x00 or 0xE0 prefix): consume and cancel
        if (ch == 0 || ch == 0xE0) {
            _getch();
            if (box.visible) {
                // Cancel: restore original text
                auto ctx = completion::build_context(out, out.size());
                replace_word(ctx.word_start, saved_line.substr(ctx.word_start));
                out = saved_line;
                box.hide(input_cursor());
                completer.reset();
            }
            continue;
        }

        // ---- Tab: show/cycle completion ----
        if (ch == '\t') {
            if (!box.visible) {
                // Save current line before completion modifies it
                saved_line = out;
            }

            auto result = completer.on_tab(out, out.size());

            if (!result.replacement.empty()) {
                auto ctx = completion::build_context(
                    box.visible ? out : saved_line,
                    box.visible ? out.size() : saved_line.size());
                replace_word(ctx.word_start, result.replacement);
            }

            if (result.show_candidates && !result.candidates.empty()) {
                box.draw(result.candidates, result.active_index, input_cursor());
                // Recalculate line_start in case scrolling happened
                COORD cur = get_cursor();
                line_start.Y = cur.Y;
                line_start.X = cur.X - static_cast<SHORT>(out.size());
            }
            continue;
        }

        // ---- Enter ----
        if (ch == '\r' || ch == '\n') {
            if (box.visible) {
                // Accept: hide box, keep current completion in place
                box.hide(input_cursor());
                completer.reset();
                // Don't return — let user see the completed line and confirm
                continue;
            }
            completer.reset();
            return true;
        }

        // ---- Ctrl+C ----
        if (ch == 3) {
            if (box.visible) {
                box.hide(input_cursor());
                completer.reset();
            }
            // Clear line
            set_cursor(line_start);
            DWORD written;
            FillConsoleOutputCharacterA(h_out(), ' ', static_cast<DWORD>(out.size()),
                                        line_start, &written);
            set_cursor(line_start);
            out.clear();
            con_write("^C\r\n");
            completer.reset();
            return true;
        }

        // ---- Any other key while box visible: cancel ----
        if (box.visible) {
            auto ctx = completion::build_context(saved_line, saved_line.size());
            replace_word(ctx.word_start, saved_line.substr(ctx.word_start));
            out = saved_line;
            box.hide(input_cursor());
            completer.reset();
            // Fall through to process the key normally
        }

        // ---- Backspace ----
        if (ch == '\b' || ch == 127) {
            if (!out.empty()) {
                out.pop_back();
                con_write("\b \b");
            }
            completer.reset();
            continue;
        }

        // Ignore other control characters
        if (ch < ' ') {
            continue;
        }

        // ---- Normal printable character ----
        char c = static_cast<char>(ch);
        out += c;
        con_write(std::string(1, c));
        completer.reset();
    }
}

// ---------------------------------------------------------------------------
// Shell
// ---------------------------------------------------------------------------

static void clear_screen() {
    std::cout << "\033[2J\033[3J\033[H" << std::flush;
}

Shell::Shell() : completer_(registry_) {
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

std::string Shell::prompt_string() const {
    return std::string(color::bold) + color::cyan + "aosh " + color::reset
         + std::filesystem::current_path().string() + "> ";
}

void Shell::run() {
    clear_screen();
    std::cout << color::bold << color::cyan << "aosh" << color::reset
              << " v" << AOSH_VERSION << "\nType 'help' for available commands.\n\n";

    std::string line;
    while (running_) {
        auto prompt = prompt_string();
        std::cout << prompt << std::flush;

        if (!read_line(line, completer_, prompt)) {
            break;
        }
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
        int rc = process::run(name, args);
        if (rc == -1) {
            std::cerr << color::red << "aosh: unknown command: " << name
                      << color::reset << "\n";
        }
        return;
    }
    cmd->handler(args);
}

} // namespace aosh
