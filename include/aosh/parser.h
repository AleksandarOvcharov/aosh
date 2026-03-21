#pragma once

#include <string>
#include <vector>
#include <utility>

namespace aosh::parser {

// Splits input into command name + arguments.
// Supports double-quoted strings with spaces.
std::pair<std::string, std::vector<std::string>> parse(const std::string& input);

} // namespace aosh::parser
