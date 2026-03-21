#pragma once

#include <ostream>

namespace aosh::color {

// ANSI escape codes
constexpr const char* reset  = "\033[0m";
constexpr const char* red    = "\033[1;31m";
constexpr const char* green  = "\033[32m";
constexpr const char* yellow = "\033[33m";
constexpr const char* blue   = "\033[34m";
constexpr const char* cyan   = "\033[36m";
constexpr const char* bold   = "\033[1m";

#ifdef _WIN32
// Enable ANSI escape processing on Windows
void init();
#else
inline void init() {}
#endif

} // namespace aosh::color
