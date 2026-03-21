#include "aosh/commands/registry.h"

namespace aosh {

void CommandRegistry::add(const std::string& name, const std::string& description,
                          std::function<void(const std::vector<std::string>&)> handler) {
    commands_[name] = {name, description, std::move(handler)};
}

const Command* CommandRegistry::find(const std::string& name) const {
    auto it = commands_.find(name);
    return it != commands_.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string, Command>& CommandRegistry::all() const {
    return commands_;
}

} // namespace aosh
