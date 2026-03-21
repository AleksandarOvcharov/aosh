#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace aosh {

struct Command {
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>&)> handler;
};

class CommandRegistry {
public:
    void add(const std::string& name, const std::string& description,
             std::function<void(const std::vector<std::string>&)> handler);

    const Command* find(const std::string& name) const;
    const std::unordered_map<std::string, Command>& all() const;

private:
    std::unordered_map<std::string, Command> commands_;
};

} // namespace aosh
