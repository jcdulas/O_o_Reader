#include "oor/model/control/statement/StatementDefinition.hpp"
namespace oor::model
{
const StatementCatalogue& fileManagementStatements()
{
    static const StatementCatalogue CATALOGUE(
        []
        {
            StatementDefinition definition;
            definition.name = "FileAssignment";
            definition.section = SectionKind::FileManagement;
            definition.shape = ControlKind::Keyword;
            return std::vector<StatementDefinition>{std::move(definition)};
        }());
    return CATALOGUE;
}
} // namespace oor::model
