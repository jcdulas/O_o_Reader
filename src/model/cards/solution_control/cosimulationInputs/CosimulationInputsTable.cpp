#include "oor/model/cards/solution_control/cosimulationInputs/CosimulationInputsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_QUANTITY_FIELD = 9;   // PHYQUA1, the first field of the continuation line
constexpr int SECOND_QUANTITY_FIELD = 10; // PHYQUA2: the last field of the card
constexpr std::size_t MOST_QUANTITIES = 2;
constexpr CosimulationInputQuantity DEFAULT_QUANTITY = CosimulationInputQuantity::Force;

} // namespace

std::optional<CosimulationInputsCard> CosimulationInputsFormat::read(FieldReader& reader)
{
    CosimulationInputsCard card;
    card.id = reader.requiredInteger(1, "id");
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8}, SECOND_QUANTITY_FIELD);
    // A blank PHYQUA1 is FORCE, as the format says; a blank PHYQUA2 is no second quantity.
    card.quantities.clear();
    card.quantities.push_back(
        readKeyword(reader, FIRST_QUANTITY_FIELD, "quantity1", COSIMULATION_INPUT_WORDS, DEFAULT_QUANTITY));
    if (!reader.isBlank(SECOND_QUANTITY_FIELD))
        card.quantities.push_back(
            readKeyword(reader, SECOND_QUANTITY_FIELD, "quantity2", COSIMULATION_INPUT_WORDS, DEFAULT_QUANTITY));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CosimulationInputsFormat::check(const CosimulationInputsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.quantities.empty() || card.quantities.size() > MOST_QUANTITIES)
        return CardProblem{"card.bad-value",
                           "the card has one or two quantities (" + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("quantity2") + ")",
                           "quantity1"};
    if (card.quantities.size() == MOST_QUANTITIES && card.quantities.front() == card.quantities.back())
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " repeats " + CardProblem::fieldNamed("quantity1"),
                           "quantity2"};
    return std::nullopt;
}

void CosimulationInputsFormat::write(const CosimulationInputsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (!card.quantities.empty())
        writeKeywordUnlessDefault(writer, FIRST_QUANTITY_FIELD, COSIMULATION_INPUT_WORDS, card.quantities.front(),
                                  DEFAULT_QUANTITY);
    if (card.quantities.size() > 1)
        writeKeyword(writer, SECOND_QUANTITY_FIELD, COSIMULATION_INPUT_WORDS, card.quantities[1]);
}

std::uint32_t CosimulationInputsTable::append(const CosimulationInputsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CosimulationInputsTable::store(std::uint32_t row, const CosimulationInputsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CosimulationInputsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CosimulationInputsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
