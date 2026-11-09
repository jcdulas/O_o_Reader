#include "oor/model/cards/explicit/explicitTextStart/ExplicitTextStartTable.hpp"

namespace oor::model
{

std::optional<ExplicitTextStartCard> ExplicitTextStartFormat::read(FieldReader& reader)
{
    if (reader.lastUsedField() > 0)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has no field", "");
    if (reader.failed())
        return std::nullopt;
    return ExplicitTextStartCard{};
}

std::optional<CardProblem> ExplicitTextStartFormat::check(const ExplicitTextStartCard&)
{
    return std::nullopt;
}

void ExplicitTextStartFormat::write(const ExplicitTextStartCard&, FieldWriter&) {}

} // namespace oor::model
