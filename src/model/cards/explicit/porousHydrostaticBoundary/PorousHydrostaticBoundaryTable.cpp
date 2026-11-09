#include "oor/model/cards/explicit/porousHydrostaticBoundary/PorousHydrostaticBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<PorousHydrostaticBoundaryCard> PorousHydrostaticBoundaryFormat::read(FieldReader& reader)
{
    PorousHydrostaticBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.lastUsedField() > 1)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 1 field", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PorousHydrostaticBoundaryFormat::check(const PorousHydrostaticBoundaryCard& card)
{
    return checkRequiredId(card.id, "id");
}

void PorousHydrostaticBoundaryFormat::write(const PorousHydrostaticBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
}

std::uint32_t PorousHydrostaticBoundaryTable::append(const PorousHydrostaticBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PorousHydrostaticBoundaryTable::store(std::uint32_t row, const PorousHydrostaticBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PorousHydrostaticBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PorousHydrostaticBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
