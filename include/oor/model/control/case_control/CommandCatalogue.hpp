#pragma once

#include "oor/model/control/case_control/CommandDefinition.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::model
{

// Every Case Control command the library knows, by name. Built once, on first use, by the family functions of
// `commands/` (see CommandFamilies.hpp); read-only afterwards, so any thread may use it.
class CommandCatalogue
{
public:
    [[nodiscard]] static const CommandCatalogue& instance();

    // The command with exactly this neutral name ("DisplacementOutput").
    [[nodiscard]] const CommandDefinition* findByName(std::string_view name) const;
    [[nodiscard]] const std::vector<CommandDefinition>& definitions() const noexcept { return m_definitions; }
    // Names given to two commands: always empty; the tests check it.
    [[nodiscard]] const std::vector<std::string>& duplicateNames() const noexcept { return m_duplicateNames; }

    // Used by the family functions while the catalogue is built. Each returns the new definition to finish with the
    // setters of CommandDefinition.
    CommandDefinition& integer(std::string name);
    CommandDefinition& integerOrKeyword(std::string name);
    CommandDefinition& real(std::string name);
    CommandDefinition& keyword(std::string name);
    CommandDefinition& text(std::string name);
    CommandDefinition& integerList(std::string name);
    CommandDefinition& realList(std::string name);
    CommandDefinition& output(std::string name);
    CommandDefinition& flag(std::string name);
    CommandDefinition& subcaseHeader(std::string name, SubcaseKind kind);
    CommandDefinition& setDefinition(std::string name);
    CommandDefinition& parameter(std::string name);
    CommandDefinition& blockHeader(std::string name, std::string blockKind);

private:
    CommandCatalogue();
    CommandDefinition& add(std::string name, CommandShape shape);
    void buildIndexes();

    std::vector<CommandDefinition> m_definitions;
    std::unordered_map<std::string, std::size_t> m_byName; // neutral names
    std::vector<std::string> m_duplicateNames;
};

} // namespace oor::model
