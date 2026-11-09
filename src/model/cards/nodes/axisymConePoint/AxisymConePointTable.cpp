#include "oor/model/cards/nodes/axisymConePoint/AxisymConePointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3;

} // namespace

std::optional<AxisymConePointCard> AxisymConePointFormat::read(FieldReader& reader)
{
    AxisymConePointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ringId = reader.requiredInteger(2, "ringId");
    card.phiAngle = reader.requiredReal(3, "phiAngle");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConePointFormat::check(const AxisymConePointCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.ringId, "ringId"))
        return problem;
    return checkReal(card.phiAngle, "phiAngle", RealRule::Finite);
}

void AxisymConePointFormat::write(const AxisymConePointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    writer.real(3, card.phiAngle);
}

std::uint32_t AxisymConePointTable::append(const AxisymConePointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_phiAngle.push(card.phiAngle);
    return row;
}

AxisymConePointCard AxisymConePointTable::card(std::uint32_t row) const
{
    return AxisymConePointCard{m_id.get(row), m_ringId.get(row), m_phiAngle.get(row)};
}

void AxisymConePointTable::store(std::uint32_t row, const AxisymConePointCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_phiAngle.set(row, card.phiAngle);
}

void AxisymConePointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_phiAngle.keepRows(rows);
}

void AxisymConePointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The RID names a RINGAX, which is a point of the node ID space.
    if (const base::EntityId ring = m_ringId.get(row); ring > 0)
        m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, ring));
}

} // namespace oor::model
