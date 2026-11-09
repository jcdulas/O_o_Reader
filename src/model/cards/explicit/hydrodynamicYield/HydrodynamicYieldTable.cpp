#include "oor/model/cards/explicit/hydrodynamicYield/HydrodynamicYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<HydrodynamicYieldCard> HydrodynamicYieldFormat::read(FieldReader& reader)
{
    HydrodynamicYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.lastUsedField() > 1)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 1 field", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HydrodynamicYieldFormat::check(const HydrodynamicYieldCard& card)
{
    return checkRequiredId(card.id, "id");
}

void HydrodynamicYieldFormat::write(const HydrodynamicYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
}

std::uint32_t HydrodynamicYieldTable::append(const HydrodynamicYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HydrodynamicYieldTable::store(std::uint32_t row, const HydrodynamicYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HydrodynamicYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HydrodynamicYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
