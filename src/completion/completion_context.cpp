#include "aosh/completion/completion_context.h"

namespace aosh::completion {

CompletionContext build_context(const std::string& line, size_t cursor_pos) {
    CompletionContext ctx;
    ctx.full_line = line.substr(0, cursor_pos);

    // Walk backwards from cursor to find current word start
    size_t pos = cursor_pos;
    while (pos > 0 && line[pos - 1] != ' ') {
        --pos;
    }
    ctx.word_start = pos;
    ctx.word_prefix = line.substr(pos, cursor_pos - pos);

    // Completing a command if we're on the first token
    // (no spaces before the current word, or only leading spaces)
    bool found_prior_token = false;
    for (size_t i = 0; i < pos; ++i) {
        if (line[i] != ' ') {
            found_prior_token = true;
            break;
        }
    }
    ctx.completing_command = !found_prior_token;

    return ctx;
}

} // namespace aosh::completion
