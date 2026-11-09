#include "oor/model/cards/nodes/axisymConeRing/AxisymConeRingTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7; // PS

} // namespace

std::optional<AxisymConeRingCard> AxisymConeRingFormat::read(FieldReader& reader)
{
    AxisymConeRingCard card;
    card.id = reader.requiredInteger(1, "id");
    // Fields 2, 5 and 6 are the blank cells of the format.
    requireBlankFields(reader, {2, 5, 6}, LAST_FIELD);
    card.radius = reader.requiredReal(3, "radius");
    card.zCoord = reader.requiredReal(4, "zCoord");
    card.permanentConstraints = base::sortedDigits(reader.integer(7, "permanentConstraints", 0));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConeRingFormat::check(const AxisymConeRingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.zCoord, "zCoord", RealRule::Finite))
        return problem;
    if (card.permanentConstraints < 0 || !base::isComponentSet(card.permanentConstraints))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "permanentConstraints"};
    return std::nullopt;
}

void AxisymConeRingFormat::write(const AxisymConeRingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(3, card.radius);
    writer.real(4, card.zCoord);
    if (card.permanentConstraints != 0)
        writer.integer(7, card.permanentConstraints);
}

std::uint32_t AxisymConeRingTable::append(const AxisymConeRingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_radius.push(card.radius);
    m_zCoord.push(card.zCoord);
    m_permanentConstraints.push(card.permanentConstraints);
    return row;
}

AxisymConeRingCard AxisymConeRingTable::card(std::uint32_t row) const
{
    return AxisymConeRingCard{m_id.get(row), m_radius.get(row), m_zCoord.get(row), m_permanentConstraints.get(row)};
}

void AxisymConeRingTable::store(std::uint32_t row, const AxisymConeRingCard& card)
{
    m_id.set(row, card.id);
    m_radius.set(row, card.radius);
    m_zCoord.set(row, card.zCoord);
    m_permanentConstraints.set(row, card.permanentConstraints);
}

void AxisymConeRingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_radius.keepRows(rows);
    m_zCoord.keepRows(rows);
    m_permanentConstraints.keepRows(rows);
}

} // namespace oor::model
