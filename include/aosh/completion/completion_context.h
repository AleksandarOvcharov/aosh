#pragma once

#include <string>

namespace aosh::completion {

struct CompletionContext {
    std::string full_line;
    std::string word_prefix;
    size_t word_start = 0;
    bool completing_command = true;
};

CompletionContext build_context(const std::string& line, size_t cursor_pos);

} // namespace aosh::completion
