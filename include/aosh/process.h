#pragma once

#include <string>
#include <vector>

namespace aosh::process {

// Runs an external program and waits for it to finish.
// - "program"    → searches PATH only
// - "./program"  → current directory only
// Returns the process exit code, or -1 if the program could not be started.
int run(const std::string& program, const std::vector<std::string>& args);

} // namespace aosh::process
