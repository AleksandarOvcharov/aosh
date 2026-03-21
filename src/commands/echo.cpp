#include "aosh/commands/echo.h"
#include <iostream>

namespace aosh::commands {

void register_echo(CommandRegistry& registry) {
    registry.add("echo", "Print text to the console", [](const std::vector<std::string>& args) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << args[i];
        }
        std::cout << '\n';
    });
}

} // namespace aosh::commands
