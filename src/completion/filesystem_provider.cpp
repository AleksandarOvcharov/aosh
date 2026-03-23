#include "aosh/completion/filesystem_provider.h"
#include <algorithm>
#include <filesystem>

namespace aosh::completion {

std::vector<std::string> FilesystemProvider::complete(const CompletionContext& ctx) const {
    if (ctx.completing_command && ctx.word_prefix.empty()) {
        return {};
    }

    namespace fs = std::filesystem;

    std::string prefix = ctx.word_prefix;

    // Split into directory part and filename prefix
    fs::path prefix_path(prefix);
    fs::path search_dir;
    std::string file_prefix;

    if (!prefix.empty() && (prefix.back() == '/' || prefix.back() == '\\')) {
        // User typed a full directory path, complete its contents
        search_dir = prefix_path;
        file_prefix = "";
    } else if (prefix_path.has_parent_path()) {
        search_dir = prefix_path.parent_path();
        file_prefix = prefix_path.filename().string();
    } else {
        search_dir = ".";
        file_prefix = prefix;
    }

    // Lowercase the prefix for case-insensitive matching
    std::string lower_prefix = file_prefix;
    std::transform(lower_prefix.begin(), lower_prefix.end(),
                   lower_prefix.begin(), ::tolower);

    std::vector<std::string> matches;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(search_dir, ec)) {
        if (ec) break;

        auto filename = entry.path().filename().string();
        std::string lower_name = filename;
        std::transform(lower_name.begin(), lower_name.end(),
                       lower_name.begin(), ::tolower);

        if (lower_name.compare(0, lower_prefix.size(), lower_prefix) != 0) {
            continue;
        }

        // Build the completion string preserving the user's directory prefix
        std::string result;
        if (search_dir == ".") {
            result = filename;
        } else {
            result = (search_dir / filename).string();
        }

        // Append path separator for directories
        if (entry.is_directory(ec)) {
            result += "\\";
        }

        matches.push_back(result);
    }

    std::sort(matches.begin(), matches.end());
    return matches;
}

} // namespace aosh::completion
