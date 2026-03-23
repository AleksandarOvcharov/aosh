#include "aosh/completion/completer.h"
#include "aosh/completion/command_provider.h"
#include "aosh/completion/path_exe_provider.h"
#include "aosh/completion/filesystem_provider.h"
#include <algorithm>

namespace aosh::completion {

Completer::Completer(const CommandRegistry& registry) {
    providers_.push_back(std::make_unique<CommandProvider>(registry));
    providers_.push_back(std::make_unique<PathExeProvider>());
    providers_.push_back(std::make_unique<FilesystemProvider>());
}

CompletionResult Completer::on_tab(const std::string& line, size_t cursor_pos) {
    CompletionResult result;

    // Already have matches — just cycle
    if (!matches_.empty()) {
        cycle_index_ = (cycle_index_ + 1) % matches_.size();
        result.replacement = matches_[cycle_index_];
        result.candidates = matches_;
        result.show_candidates = true;
        result.active_index = cycle_index_;
        return result;
    }

    // First Tab — gather matches
    auto ctx = build_context(line, cursor_pos);

    for (const auto& provider : providers_) {
        auto m = provider->complete(ctx);
        for (auto& s : m) {
            matches_.push_back(std::move(s));
        }
    }

    std::sort(matches_.begin(), matches_.end());
    matches_.erase(std::unique(matches_.begin(), matches_.end()), matches_.end());

    if (matches_.empty()) {
        return result;
    }

    // Single match — complete directly, no box
    if (matches_.size() == 1) {
        result.replacement = matches_[0];
        if (!result.replacement.empty() && result.replacement.back() != '\\') {
            result.replacement += ' ';
        }
        matches_.clear();
        return result;
    }

    // Too many — don't show box
    if (matches_.size() > MAX_VISIBLE_CANDIDATES) {
        matches_.clear();
        return result;
    }

    // Multiple matches — show box with first highlighted
    cycle_index_ = 0;
    result.replacement = matches_[cycle_index_];
    result.candidates = matches_;
    result.show_candidates = true;
    result.active_index = cycle_index_;
    return result;
}

void Completer::reset() {
    matches_.clear();
    cycle_index_ = 0;
}

} // namespace aosh::completion
