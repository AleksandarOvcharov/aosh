#pragma once

#include "aosh/completion/provider.h"

namespace aosh::completion {

class FilesystemProvider : public CompletionProvider {
public:
    std::vector<std::string> complete(const CompletionContext& ctx) const override;
};

} // namespace aosh::completion
