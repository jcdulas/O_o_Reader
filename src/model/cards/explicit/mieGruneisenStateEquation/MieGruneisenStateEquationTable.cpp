#include "oor/model/cards/explicit/mieGruneisenStateEquation/MieGruneisenStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MieGruneisenStateEquationCard> MieGruneisenStateEquationFormat::read(FieldReader& reader)
{
    MieGruneisenStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.soundSpeed = reader.requiredReal(2, "soundSpeed");
    card.shockSlope = reader.requiredReal(3, "shockSlope");
    card.gruneisenParameter = reader.requiredReal(4, "gruneisenParameter");
    card.densityCutoff = reader.requiredReal(5, "densityCutoff");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MieGruneisenStateEquationFormat::check(const MieGruneisenStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.soundSpeed, "soundSpeed", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.shockSlope, "shockSlope", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.gruneisenParameter, "gruneisenParameter", RealRule::Finite))
        return problem;
    return checkReal(card.densityCutoff, "densityCutoff", RealRule::Finite);
}

void MieGruneisenStateEquationFormat::write(const MieGruneisenStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.soundSpeed);
    writer.real(3, card.shockSlope);
    writer.real(4, card.gruneisenParameter);
    writer.real(5, card.densityCutoff);
}

std::uint32_t MieGruneisenStateEquationTable::append(const MieGruneisenStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MieGruneisenStateEquationTable::store(std::uint32_t row, const MieGruneisenStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MieGruneisenStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MieGruneisenStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
