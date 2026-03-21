#include "aosh/commands/help.h"
#include "aosh/color.h"
#include <algorithm>
#include <iostream>

namespace aosh::commands {

void register_help(CommandRegistry& registry) {
    registry.add("help", "Show available commands", [&registry](const std::vector<std::string>& args) {
        if (!args.empty()) {
            auto* cmd = registry.find(args[0]);
            if (cmd) {
                std::cout << color::cyan << cmd->name << color::reset
                          << " - " << cmd->description << "\n";
            } else {
                std::cerr << color::red << "help: unknown command: " << args[0] << color::reset << "\n";
            }
            return;
        }

        std::vector<const Command*> sorted;
        for (const auto& [name, cmd] : registry.all()) {
            sorted.push_back(&cmd);
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](const Command* a, const Command* b) { return a->name < b->name; });

        std::cout << color::bold << "Available commands:" << color::reset << "\n";
        for (const auto* cmd : sorted) {
            std::cout << "  " << color::cyan << cmd->name << color::reset;
            for (size_t i = cmd->name.size(); i < 12; ++i) std::cout << ' ';
            std::cout << cmd->description << "\n";
        }
    });
}

} // namespace aosh::commands
