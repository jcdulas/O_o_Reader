#include "oor/model/cards/solution_control/cosimulationOutputs/CosimulationOutputsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int FIRST_QUANTITY_FIELD = 9; // PHYQUA1, the first field of the continuation line
constexpr int LAST_QUANTITY_FIELD = 11; // PHYQUA3: the last field of the card
constexpr std::size_t MOST_QUANTITIES = 3;

} // namespace

std::optional<CosimulationOutputsCard> CosimulationOutputsFormat::read(FieldReader& reader)
{
    CosimulationOutputsCard card;
    card.id = reader.requiredInteger(1, "id");
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8}, LAST_QUANTITY_FIELD);
    for (int field = FIRST_QUANTITY_FIELD; field <= LAST_QUANTITY_FIELD; ++field)
    {
        if (reader.isBlank(field))
            continue; // a quantity that is not given
        card.quantities.push_back(
            readRequiredKeyword<CosimulationOutputQuantity>(reader, field, "quantities", COSIMULATION_OUTPUT_WORDS));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CosimulationOutputsFormat::check(const CosimulationOutputsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.quantities.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one quantity (" + std::string(CardProblem::FIELD) + ")",
                           "quantity1"};
    if (card.quantities.size() > MOST_QUANTITIES)
        return CardProblem{"card.bad-value", "the card has at most three quantities", "quantity3"};
    std::vector<CosimulationOutputQuantity> sorted = card.quantities;
    std::ranges::sort(sorted);
    if (std::ranges::adjacent_find(sorted) != sorted.end())
        return CardProblem{"card.bad-value", "a quantity is given twice", "quantities"};
    return std::nullopt;
}

void CosimulationOutputsFormat::write(const CosimulationOutputsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_QUANTITY_FIELD;
    for (const CosimulationOutputQuantity quantity : card.quantities)
        writeKeyword(writer, field++, COSIMULATION_OUTPUT_WORDS, quantity);
}

std::uint32_t CosimulationOutputsTable::append(const CosimulationOutputsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CosimulationOutputsTable::store(std::uint32_t row, const CosimulationOutputsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CosimulationOutputsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CosimulationOutputsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
