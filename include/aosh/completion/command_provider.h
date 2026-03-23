#pragma once

#include "aosh/completion/provider.h"
#include "aosh/commands/registry.h"

namespace aosh::completion {

class CommandProvider : public CompletionProvider {
public:
    explicit CommandProvider(const CommandRegistry& registry);
    std::vector<std::string> complete(const CompletionContext& ctx) const override;

private:
    const CommandRegistry& registry_;
};

} // namespace aosh::completion
