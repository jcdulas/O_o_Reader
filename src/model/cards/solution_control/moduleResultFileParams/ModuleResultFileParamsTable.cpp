#include "oor/model/cards/solution_control/moduleResultFileParams/ModuleResultFileParamsTable.hpp"

namespace oor::model
{

namespace
{

// The pairs start in field 2 of the first continuation line: field 9 of the reader.
constexpr int FIRST_PARAMETER_FIELD = 9;

} // namespace

std::optional<ModuleResultFileParamsCard> ModuleResultFileParamsFormat::read(FieldReader& reader)
{
    ModuleResultFileParamsCard card;
    card.moduleId = reader.requiredInteger(1, "moduleId");
    // Fields 3 to 9 of the first line are blank in the drawing of the format.
    for (int field = 2; field < FIRST_PARAMETER_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error(
                "card.bad-value",
                "the parameters of an " + std::string(reader.cardName()) + " start on the second line, in field 2",
                "parameterName");
            return card;
        }
    }
    card.parameters = readHdf5Parameters(reader, FIRST_PARAMETER_FIELD);
    if (card.parameters.empty() && !reader.failed())
        reader.error("card.missing-field",
                     "an " + std::string(reader.cardName()) + " needs at least one parameter and its value",
                     "parameterNames");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleResultFileParamsFormat::check(const ModuleResultFileParamsCard& card)
{
    if (card.moduleId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "moduleId"};
    if (card.parameters.empty())
        return CardProblem{"card.missing-field", "the card needs at least one parameter and its value",
                           "parameterNames"};
    return checkHdf5Parameters(card.parameters);
}

void ModuleResultFileParamsFormat::write(const ModuleResultFileParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.moduleId);
    writeHdf5Parameters(writer, FIRST_PARAMETER_FIELD, card.parameters);
}

std::uint32_t ModuleResultFileParamsTable::append(const ModuleResultFileParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
