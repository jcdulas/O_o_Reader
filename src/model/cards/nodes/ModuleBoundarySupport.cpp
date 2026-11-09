#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

ModuleOrAll readModuleOrAll(FieldReader& reader, int field, const char* name, ModuleOrAll fallback)
{
    if (reader.isBlank(field))
        return fallback;
    if ((reader.text(field) == "All"))
        return ModuleOrAll::everything();
    return ModuleOrAll::module(reader.requiredInteger(field, name));
}

ModuleOrAll readRequiredModuleOrAll(FieldReader& reader, int field, const char* name)
{
    if (reader.isBlank(field))
    {
        reader.error("card.missing-field", std::string(name) + " is required: a module number or ALL", name);
        return ModuleOrAll::module(0);
    }
    return readModuleOrAll(reader, field, name, ModuleOrAll::module(0));
}

void writeModuleOrAll(FieldWriter& writer, int field, const ModuleOrAll& value, bool blankIsAll)
{
    if (!value.all)
        writer.integer(field, value.number);
    else if (blankIsAll)
        writer.textUnlessDefault(field, "All", "All");
    else
        writer.text(field, "All");
}

std::optional<CardProblem> checkModuleOrAll(const ModuleOrAll& value, const char* field, bool zeroAllowed)
{
    if (value.all)
        return std::nullopt;
    if (value.number > 0 || (value.number == 0 && zeroAllowed))
        return std::nullopt;
    return CardProblem{"card.bad-id",
                       std::string(CardProblem::FIELD) + " must be " +
                           (zeroAllowed ? "zero or a positive" : "a positive") + " module number or ALL",
                       field};
}

std::optional<CardProblem> checkBoundaryModule(base::EntityId module, const char* field)
{
    if (module >= 0)
        return std::nullopt;
    return CardProblem{"card.bad-id",
                       std::string(CardProblem::FIELD) + " must be a module number (zero or a positive integer)",
                       field};
}

std::optional<CardProblem> boundaryLabelProblem(std::string_view label, const char* field)
{
    return checkNameWord(label, field);
}

void visitBoundaryRuns(ReferenceVisitor& visitor, std::string_view field, base::EntityId module,
                       std::vector<IdRun>& runs, ReferenceTarget single, ReferenceTarget range)
{
    const auto visitOne = [&](ReferenceTarget target, base::EntityId id)
    {
        return module == 0 ? visitor.visit(field, target, id)
                           : visitor.visitInPart(field, target, id, "ModuleSection", module);
    };
    for (IdRun& run : runs)
    {
        if (run.first == run.last)
        {
            run.first = run.last = visitOne(single, run.first);
            continue;
        }
        run.first = visitOne(range, run.first);
        run.last = visitOne(range, run.last);
    }
}

} // namespace oor::model
