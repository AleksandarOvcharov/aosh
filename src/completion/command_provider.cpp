#include "aosh/completion/command_provider.h"
#include <algorithm>

namespace aosh::completion {

CommandProvider::CommandProvider(const CommandRegistry& registry)
    : registry_(registry) {}

std::vector<std::string> CommandProvider::complete(const CompletionContext& ctx) const {
    if (!ctx.completing_command) {
        return {};
    }

    std::vector<std::string> matches;
    for (const auto& [name, cmd] : registry_.all()) {
        if (name.size() >= ctx.word_prefix.size() &&
            name.compare(0, ctx.word_prefix.size(), ctx.word_prefix) == 0) {
            matches.push_back(name);
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

} // namespace aosh::completion
