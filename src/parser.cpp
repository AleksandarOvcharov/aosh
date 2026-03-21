#include "aosh/parser.h"

namespace aosh::parser {

std::pair<std::string, std::vector<std::string>> parse(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ' ' && !in_quotes) {
            if (!token.empty()) {
                tokens.push_back(std::move(token));
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(std::move(token));
    }

    if (tokens.empty()) {
        return {"", {}};
    }

    std::string name = tokens[0];
    tokens.erase(tokens.begin());
    return {name, tokens};
}

} // namespace aosh::parser
