#pragma once

#include "aosh/commands/registry.h"
#include "aosh/completion/completer.h"
#include <string>
#include <vector>

namespace aosh {

class Shell {
public:
    Shell();
    void run();

private:
    bool running_ = true;
    CommandRegistry registry_;
    completion::Completer completer_;
    std::vector<std::string> history_;

    std::string prompt_string() const;
    void execute(const std::string& input);
};

} // namespace aosh
