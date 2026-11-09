#include "oor/model/cards/explicit/explicitTextEnd/ExplicitTextEndTable.hpp"

namespace oor::model
{

std::optional<ExplicitTextEndCard> ExplicitTextEndFormat::read(FieldReader& reader)
{
    if (reader.lastUsedField() > 0)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has no field", "");
    if (reader.failed())
        return std::nullopt;
    return ExplicitTextEndCard{};
}

std::optional<CardProblem> ExplicitTextEndFormat::check(const ExplicitTextEndCard&)
{
    return std::nullopt;
}

void ExplicitTextEndFormat::write(const ExplicitTextEndCard&, FieldWriter&) {}

} // namespace oor::model
