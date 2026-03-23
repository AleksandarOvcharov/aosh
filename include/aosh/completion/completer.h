#pragma once

#include "aosh/completion/provider.h"
#include "aosh/commands/registry.h"
#include <memory>
#include <string>
#include <vector>

namespace aosh::completion {

static constexpr size_t MAX_VISIBLE_CANDIDATES = 50;

struct CompletionResult {
    std::string replacement;
    bool show_candidates = false;
    std::vector<std::string> candidates;
    size_t active_index = 0;
};

class Completer {
public:
    explicit Completer(const CommandRegistry& registry);

    // Call on each Tab press.  First call gathers matches;
    // subsequent calls cycle through them.
    CompletionResult on_tab(const std::string& line, size_t cursor_pos);

    void reset();

private:
    std::vector<std::unique_ptr<CompletionProvider>> providers_;

    std::vector<std::string> matches_;
    size_t cycle_index_ = 0;
};

} // namespace aosh::completion
