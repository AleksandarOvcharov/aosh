#pragma once

#include "aosh/completion/completion_context.h"
#include <string>
#include <vector>

namespace aosh::completion {

class CompletionProvider {
public:
    virtual ~CompletionProvider() = default;
    virtual std::vector<std::string> complete(const CompletionContext& ctx) const = 0;
};

} // namespace aosh::completion
