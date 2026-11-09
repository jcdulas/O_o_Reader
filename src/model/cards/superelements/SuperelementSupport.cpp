#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CardProblem> checkSuperelementId(base::EntityId id, const char* field, bool residualAllowed)
{
    if (id > 0 || (id == 0 && residualAllowed))
        return std::nullopt;
    return CardProblem{"card.bad-id",
                       std::string(CardProblem::FIELD) + " must be the number of a superelement" +
                           (residualAllowed ? " (0 is the residual structure)" : " (a positive integer)"),
                       field};
}

SuperelementOrAll readSuperelementOrAll(FieldReader& reader, int field, const char* name, SuperelementOrAll fallback)
{
    if (reader.isBlank(field))
        return fallback;
    if ((reader.text(field) == "All"))
        return std::nullopt;
    return reader.requiredInteger(field, name);
}

void writeSuperelementOrAll(FieldWriter& writer, int field, const SuperelementOrAll& value, bool blankIsAll)
{
    if (value)
        writer.integer(field, *value);
    else if (blankIsAll)
        writer.textUnlessDefault(field, "All", "All");
    else
        writer.text(field, "All");
}

void visitRun(ReferenceVisitor& visitor, std::string_view field, IdRun& run, ReferenceTarget single,
              ReferenceTarget range)
{
    if (run.first == run.last)
    {
        run.first = run.last = visitor.visit(field, single, run.first);
        return;
    }
    run.first = visitor.visit(field, range, run.first);
    run.last = visitor.visit(field, range, run.last);
}

void visitRunList(ReferenceVisitor& visitor, std::string_view field, std::vector<IdRun>& runs, ReferenceTarget single,
                  ReferenceTarget range)
{
    for (IdRun& run : runs)
        visitRun(visitor, field, run, single, range);
}

std::optional<CardProblem> superelementLabelProblem(std::string_view label, const char* field)
{
    return label.empty() ? std::nullopt : checkNameWord(label, field);
}

} // namespace oor::model
