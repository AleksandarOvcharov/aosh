#include "aosh/commands/ls.h"
#include "aosh/color.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace aosh::commands {

namespace fs = std::filesystem;

// ---- Flag parsing ---------------------------------------------------------

struct LsFlags {
    bool all       = false;  // -a  show hidden
    bool long_fmt  = false;  // -l  long format
    bool human     = false;  // -h  human-readable sizes
    bool recursive = false;  // -R  recursive
    bool sort_size = false;  // -S  sort by size
    bool sort_time = false;  // -t  sort by modification time
    bool reverse   = false;  // -r  reverse sort
};

static LsFlags parse_flags(const std::vector<std::string>& args,
                            std::vector<std::string>& paths) {
    LsFlags f;
    for (const auto& arg : args) {
        if (!arg.empty() && arg[0] == '-' && arg.size() > 1) {
            for (size_t i = 1; i < arg.size(); ++i) {
                switch (arg[i]) {
                    case 'a': f.all = true; break;
                    case 'l': f.long_fmt = true; break;
                    case 'h': f.human = true; break;
                    case 'R': f.recursive = true; break;
                    case 'S': f.sort_size = true; break;
                    case 't': f.sort_time = true; break;
                    case 'r': f.reverse = true; break;
                    default:
                        std::cerr << color::red << "ls: unknown flag: -"
                                  << arg[i] << color::reset << "\n";
                        break;
                }
            }
        } else {
            paths.push_back(arg);
        }
    }
    return f;
}

// ---- Helpers --------------------------------------------------------------

static bool is_hidden(const fs::directory_entry& entry) {
    // Windows hidden attribute
    DWORD attrs = GetFileAttributesW(entry.path().c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_HIDDEN)) {
        return true;
    }
    // Dotfile convention
    auto name = entry.path().filename().string();
    return !name.empty() && name[0] == '.';
}

static std::string human_size(uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && u < 4) {
        size /= 1024.0;
        ++u;
    }
    std::ostringstream oss;
    if (u == 0) {
        oss << bytes << " B";
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[u];
    }
    return oss.str();
}

static std::string format_size(uintmax_t bytes, bool human) {
    if (human) return human_size(bytes);
    return std::to_string(bytes);
}

static std::string format_time(fs::file_time_type ftime) {
    // Convert to system_clock time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm_buf;
    localtime_s(&tm_buf, &tt);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%b %d %H:%M", &tm_buf);
    return buf;
}

static std::string permissions_string(fs::perms p) {
    std::string s;
    auto bit = [](fs::perms p, fs::perms mask, char c) { return (p & mask) != fs::perms::none ? c : '-'; };
    s += bit(p, fs::perms::owner_read,    'r');
    s += bit(p, fs::perms::owner_write,   'w');
    s += bit(p, fs::perms::owner_exec,    'x');
    s += bit(p, fs::perms::group_read,    'r');
    s += bit(p, fs::perms::group_write,   'w');
    s += bit(p, fs::perms::group_exec,    'x');
    s += bit(p, fs::perms::others_read,   'r');
    s += bit(p, fs::perms::others_write,  'w');
    s += bit(p, fs::perms::others_exec,   'x');
    return s;
}

// Colorize entry name based on type
static std::string colorize(const fs::directory_entry& entry) {
    std::string name = entry.path().filename().string();
    std::error_code ec;
    if (entry.is_directory(ec)) {
        return std::string(color::bold) + color::blue + name + color::reset;
    }
    if (entry.is_symlink(ec)) {
        return std::string(color::bold) + color::cyan + name + color::reset;
    }
    // Executables
    auto ext = entry.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".exe" || ext == ".bat" || ext == ".cmd" || ext == ".com") {
        return std::string(color::bold) + color::green + name + color::reset;
    }
    return name;
}

// ---- Listing logic --------------------------------------------------------

struct EntryInfo {
    fs::directory_entry entry;
    uintmax_t size;
    fs::file_time_type mtime;
};

static void gather_entries(const fs::path& dir, const LsFlags& flags,
                           std::vector<EntryInfo>& entries) {
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!flags.all && is_hidden(e)) continue;

        EntryInfo info;
        info.entry = e;
        info.size = e.is_regular_file(ec) ? e.file_size(ec) : 0;
        info.mtime = e.last_write_time(ec);
        entries.push_back(std::move(info));
    }
}

static void sort_entries(std::vector<EntryInfo>& entries, const LsFlags& flags) {
    if (flags.sort_size) {
        std::sort(entries.begin(), entries.end(),
                  [](const EntryInfo& a, const EntryInfo& b) { return a.size > b.size; });
    } else if (flags.sort_time) {
        std::sort(entries.begin(), entries.end(),
                  [](const EntryInfo& a, const EntryInfo& b) { return a.mtime > b.mtime; });
    } else {
        // Alphabetical (case-insensitive)
        std::sort(entries.begin(), entries.end(), [](const EntryInfo& a, const EntryInfo& b) {
            std::string na = a.entry.path().filename().string();
            std::string nb = b.entry.path().filename().string();
            std::transform(na.begin(), na.end(), na.begin(), ::tolower);
            std::transform(nb.begin(), nb.end(), nb.begin(), ::tolower);
            return na < nb;
        });
    }
    if (flags.reverse) {
        std::reverse(entries.begin(), entries.end());
    }
}

static void print_long(const std::vector<EntryInfo>& entries, const LsFlags& flags) {
    // Calculate column widths
    size_t max_size_width = 0;
    for (const auto& e : entries) {
        std::string s = format_size(e.size, flags.human);
        if (s.size() > max_size_width) max_size_width = s.size();
    }

    for (const auto& e : entries) {
        std::error_code ec;

        std::string type_label = e.entry.is_directory(ec) ? "Folder" : "File  ";

        std::string sz = format_size(e.size, flags.human);
        std::string time_str = format_time(e.mtime);
        std::string name = colorize(e.entry);

        std::cout << color::cyan << type_label << color::reset << "  "
                  << std::setw(static_cast<int>(max_size_width)) << std::right << sz << "  "
                  << time_str << "  "
                  << name << "\n";
    }
}

static void print_columns(const std::vector<EntryInfo>& entries) {
    if (entries.empty()) return;

    // Get terminal width
    CONSOLE_SCREEN_BUFFER_INFO info;
    int width = 80;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        width = info.srWindow.Right - info.srWindow.Left + 1;
    }

    // Find the widest name (visible length without ANSI codes)
    size_t max_name = 0;
    std::vector<std::string> names;
    for (const auto& e : entries) {
        std::string n = colorize(e.entry);
        std::string raw = e.entry.path().filename().string();
        names.push_back(n);
        if (raw.size() > max_name) max_name = raw.size();
    }

    size_t col_width = max_name + 2;
    size_t cols = std::max<size_t>(1, static_cast<size_t>(width) / col_width);

    for (size_t i = 0; i < names.size(); ++i) {
        std::string raw = entries[i].entry.path().filename().string();
        std::cout << names[i];
        // Pad based on raw name length (excluding ANSI codes)
        if ((i + 1) % cols != 0 && i + 1 != names.size()) {
            size_t pad = col_width - raw.size();
            std::cout << std::string(pad, ' ');
        }
        if ((i + 1) % cols == 0 || i + 1 == names.size()) {
            std::cout << "\n";
        }
    }
}

static void list_directory(const fs::path& dir, const LsFlags& flags, bool show_header) {
    if (show_header) {
        std::cout << color::bold << dir.string() << ":" << color::reset << "\n";
    }

    std::vector<EntryInfo> entries;
    gather_entries(dir, flags, entries);
    sort_entries(entries, flags);

    if (flags.long_fmt) {
        std::cout << "Total " << entries.size() << "\n\n";
        print_long(entries, flags);
    } else {
        print_columns(entries);
    }

    if (flags.recursive) {
        for (const auto& e : entries) {
            std::error_code ec;
            if (e.entry.is_directory(ec)) {
                std::cout << "\n";
                list_directory(e.entry.path(), flags, true);
            }
        }
    }
}

// ---- Registration ---------------------------------------------------------

void register_ls(CommandRegistry& registry) {
    registry.add("ls", "List directory contents  [-a -l -h -R -S -t -r]",
        [](const std::vector<std::string>& args) {
            std::vector<std::string> paths;
            LsFlags flags = parse_flags(args, paths);

            if (paths.empty()) {
                paths.push_back(".");
            }

            bool multi = paths.size() > 1;
            for (size_t i = 0; i < paths.size(); ++i) {
                fs::path p(paths[i]);
                std::error_code ec;

                if (!fs::exists(p, ec)) {
                    std::cerr << color::red << "ls: " << paths[i]
                              << ": no such file or directory" << color::reset << "\n";
                    continue;
                }

                if (fs::is_regular_file(p, ec)) {
                    // Single file — just print it
                    std::cout << paths[i] << "\n";
                    continue;
                }

                if (multi && i > 0) std::cout << "\n";
                list_directory(p, flags, multi || flags.recursive);
            }
        });
}

} // namespace aosh::commands
