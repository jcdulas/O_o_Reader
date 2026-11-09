#include "oor/model/control/file_management/FileManagement.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/control/statement/StatementDefinition.hpp"

#include <algorithm>

namespace oor::model
{

ControlStatement& FileManagement::addKeyword(std::string_view name, std::vector<StatementParameter> parameters)
{
    const StatementDefinition* definition = fileManagementStatements().findByName(name);
    if (definition == nullptr)
        throw base::UsageError(std::string(name) + " is not a File Management statement");
    ControlStatement fresh;
    fresh.kind = ControlKind::Keyword;
    fresh.name = definition->name;
    fresh.parameters = std::move(parameters);
    return insertAt(endIndex(), std::move(fresh));
}

} // namespace oor::model
