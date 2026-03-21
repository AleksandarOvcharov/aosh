#pragma once

#include "aosh/commands/registry.h"
#include <string>

namespace aosh {

class Shell {
public:
    Shell();
    void run();

private:
    bool running_ = true;
    CommandRegistry registry_;

    void execute(const std::string& input);
};

} // namespace aosh
