#include "aosh/process.h"
#include <cstring>
#include <filesystem>
#include <process.h>
#include <windows.h>

namespace aosh::process {

static bool has_prefix(const std::string& s, const char* prefix) {
    return s.compare(0, std::strlen(prefix), prefix) == 0;
}

static std::vector<const char*> build_argv(const std::string& program,
                                           const std::vector<std::string>& args) {
    std::vector<const char*> argv;
    argv.push_back(program.c_str());
    for (const auto& a : args) {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);
    return argv;
}

static std::string resolve_exe(const std::string& name) {
    namespace fs = std::filesystem;
    if (fs::path(name).has_extension()) {
        return name;
    }
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
        if (fs::exists(name + ".exe")) {
            return name + ".exe";
        }
        if (fs::exists(name)) {
            return name;
        }
    }
    return name + ".exe";
}

int run(const std::string& program, const std::vector<std::string>& args) {
    bool is_relative = has_prefix(program, "./") || has_prefix(program, ".\\");

    std::string resolved = resolve_exe(program);
    auto argv = build_argv(resolved, args);

    intptr_t rc;
    if (is_relative) {
        auto full = std::filesystem::current_path() / resolved;
        auto full_str = full.string();
        auto argv_abs = build_argv(full_str, args);
        rc = _spawnv(_P_WAIT, full_str.c_str(), argv_abs.data());
    } else {
        rc = _spawnvp(_P_WAIT, resolved.c_str(), argv.data());
    }

    return static_cast<int>(rc);
}

} // namespace aosh::process
