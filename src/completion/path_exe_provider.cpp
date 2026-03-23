#include "aosh/completion/path_exe_provider.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <windows.h>

namespace aosh::completion {

static bool has_exe_extension(const std::string& name) {
    namespace fs = std::filesystem;
    auto ext = fs::path(name).extension().string();
    // Case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".exe" || ext == ".cmd" || ext == ".bat" || ext == ".com";
}

void PathExeProvider::refresh_cache() const {
    const char* path_env = std::getenv("PATH");
    if (!path_env) {
        cached_path_.clear();
        cached_exes_.clear();
        return;
    }

    std::string current_path = path_env;
    if (current_path == cached_path_) {
        return;
    }

    cached_path_ = current_path;
    cached_exes_.clear();

    namespace fs = std::filesystem;
    std::string dir;
    for (size_t i = 0; i <= current_path.size(); ++i) {
        if (i == current_path.size() || current_path[i] == ';') {
            if (!dir.empty()) {
                std::error_code ec;
                for (const auto& entry : fs::directory_iterator(dir, ec)) {
                    if (ec) break;
                    if (!entry.is_regular_file(ec)) continue;
                    auto filename = entry.path().filename().string();
                    if (has_exe_extension(filename)) {
                        // Store without extension for cleaner completion
                        auto stem = entry.path().stem().string();
                        // Convert to lowercase for matching
                        std::string lower_stem = stem;
                        std::transform(lower_stem.begin(), lower_stem.end(),
                                       lower_stem.begin(), ::tolower);
                        cached_exes_.push_back(lower_stem);
                    }
                }
            }
            dir.clear();
        } else {
            dir += current_path[i];
        }
    }

    // Remove duplicates
    std::sort(cached_exes_.begin(), cached_exes_.end());
    cached_exes_.erase(
        std::unique(cached_exes_.begin(), cached_exes_.end()),
        cached_exes_.end());
}

std::vector<std::string> PathExeProvider::complete(const CompletionContext& ctx) const {
    if (!ctx.completing_command) {
        return {};
    }

    refresh_cache();

    std::string prefix = ctx.word_prefix;
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

    std::vector<std::string> matches;
    for (const auto& exe : cached_exes_) {
        if (exe.size() >= prefix.size() &&
            exe.compare(0, prefix.size(), prefix) == 0) {
            matches.push_back(exe);
        }
    }
    return matches;
}

} // namespace aosh::completion
