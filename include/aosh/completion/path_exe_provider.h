#pragma once

#include "aosh/completion/provider.h"
#include <string>
#include <vector>

namespace aosh::completion {

class PathExeProvider : public CompletionProvider {
public:
    std::vector<std::string> complete(const CompletionContext& ctx) const override;

private:
    mutable std::string cached_path_;
    mutable std::vector<std::string> cached_exes_;

    void refresh_cache() const;
};

} // namespace aosh::completion
