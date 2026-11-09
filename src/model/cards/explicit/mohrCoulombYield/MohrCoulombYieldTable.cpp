#include "oor/model/cards/explicit/mohrCoulombYield/MohrCoulombYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MohrCoulombYieldCard> MohrCoulombYieldFormat::read(FieldReader& reader)
{
    MohrCoulombYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.maxYieldStress = reader.requiredReal(2, "maxYieldStress");
    card.cohesion = reader.requiredReal(3, "cohesion");
    card.internalFriction = reader.requiredReal(4, "internalFriction");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MohrCoulombYieldFormat::check(const MohrCoulombYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.maxYieldStress, "maxYieldStress", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.cohesion, "cohesion", RealRule::Finite))
        return problem;
    return checkReal(card.internalFriction, "internalFriction", RealRule::Finite);
}

void MohrCoulombYieldFormat::write(const MohrCoulombYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.maxYieldStress);
    writer.real(3, card.cohesion);
    writer.real(4, card.internalFriction);
}

std::uint32_t MohrCoulombYieldTable::append(const MohrCoulombYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MohrCoulombYieldTable::store(std::uint32_t row, const MohrCoulombYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MohrCoulombYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MohrCoulombYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
