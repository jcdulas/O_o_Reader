#include "oor/analysis/superelements/ExternalSuperelementFiles.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/control/statement/StatementParameter.hpp"

namespace oor::analysis
{

std::optional<std::string> ExternalSuperelementFiles::assignedFile(const model::Model& model,
                                                                   const SuperelementEntry& entry)
{
    if (entry.type != SuperelementType::ExternalBinaryResults && entry.type != SuperelementType::ExternalMatrixFile &&
        entry.type != SuperelementType::ExternalReducedModel)
        return std::nullopt;
    // format reference ASSIGN: ASSIGN logical-key='filename' UNIT=u ...; the first parameter is the logical key with
    // the file.
    for (const model::ControlStatement* statement : model.root().control().fileManagement.findAll("FileAssignment"))
    {
        const model::StatementParameter* unit = model::findParameter(statement->parameters, "FileUnit");
        if (unit == nullptr || !unit->hasValue)
            continue;
        const auto number = base::NumberParser::parseInteger(base::trimAscii(unit->value));
        if (!number || *number != entry.unitNumber)
            continue;
        for (const model::StatementParameter& parameter : statement->parameters)
        {
            if (parameter.hasValue && &parameter != unit)
                return parameter.value;
        }
    }
    return std::nullopt;
}

} // namespace oor::analysis
