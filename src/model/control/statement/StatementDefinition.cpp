#include "oor/model/control/statement/StatementDefinition.hpp"

#include <algorithm>

namespace oor::model
{

StatementDefinition& StatementDefinition::describers(std::vector<std::string> list)
{
    keywords = std::move(list);
    return *this;
}

StatementCatalogue::StatementCatalogue(std::vector<StatementDefinition> definitions)
    : m_definitions(std::move(definitions))
{
}

const StatementDefinition* StatementCatalogue::findByName(std::string_view name) const noexcept
{
    for (const StatementDefinition& definition : m_definitions)
    {
        if (definition.name == name)
            return &definition;
    }
    return nullptr;
}

} // namespace oor::model
