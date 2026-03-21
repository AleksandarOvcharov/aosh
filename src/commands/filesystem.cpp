#include "aosh/commands/filesystem.h"
#include "aosh/color.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace aosh::commands {

void register_filesystem(CommandRegistry& registry) {
    registry.add("mkdir", "Create a directory", [](const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << color::red << "mkdir: missing directory name" << color::reset << "\n";
            return;
        }
        for (const auto& path : args) {
            std::error_code ec;
            if (std::filesystem::create_directories(path, ec)) {
                std::cout << "Created: " << path << "\n";
            } else if (ec) {
                std::cerr << color::red << "mkdir: " << path << ": " << ec.message() << color::reset << "\n";
            } else {
                std::cerr << color::red << "mkdir: " << path << ": already exists" << color::reset << "\n";
            }
        }
    });

    registry.add("rmdir", "Remove an empty directory", [](const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << color::red << "rmdir: missing directory name" << color::reset << "\n";
            return;
        }
        for (const auto& path : args) {
            std::error_code ec;
            if (std::filesystem::remove(path, ec)) {
                std::cout << "Removed: " << path << "\n";
            } else if (ec) {
                std::cerr << color::red << "rmdir: " << path << ": " << ec.message() << color::reset << "\n";
            } else {
                std::cerr << color::red << "rmdir: " << path << ": not found" << color::reset << "\n";
            }
        }
    });

    registry.add("mkfile", "Create an empty file", [](const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << color::red << "mkfile: missing file name" << color::reset << "\n";
            return;
        }
        for (const auto& path : args) {
            if (std::filesystem::exists(path)) {
                std::cerr << color::red << "mkfile: " << path << ": already exists" << color::reset << "\n";
                continue;
            }
            std::ofstream file(path);
            if (file) {
                std::cout << "Created: " << path << "\n";
            } else {
                std::cerr << color::red << "mkfile: " << path << ": could not create file" << color::reset << "\n";
            }
        }
    });

    registry.add("rmfile", "Remove a file", [](const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << color::red << "rmfile: missing file name" << color::reset << "\n";
            return;
        }
        for (const auto& path : args) {
            std::error_code ec;
            if (std::filesystem::remove(path, ec)) {
                std::cout << "Removed: " << path << "\n";
            } else if (ec) {
                std::cerr << color::red << "rmfile: " << path << ": " << ec.message() << color::reset << "\n";
            } else {
                std::cerr << color::red << "rmfile: " << path << ": not found" << color::reset << "\n";
            }
        }
    });
}

} // namespace aosh::commands
