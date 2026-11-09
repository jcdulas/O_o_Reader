#include "oor/model/cards/loads/axisymConePressureLoad/AxisymConePressureLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6; // PHI2

} // namespace

std::optional<AxisymConePressureLoadCard> AxisymConePressureLoadFormat::read(FieldReader& reader)
{
    AxisymConePressureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pressure = reader.requiredReal(2, "pressure");
    card.ringId1 = reader.requiredInteger(3, "ringId1");
    card.ringId2 = reader.requiredInteger(4, "ringId2");
    // A blank angle is 0 degrees for PHI1 and 360 degrees for PHI2 (the format gives no default).
    card.phiAngle1 = reader.real(5, "phiAngle1", AxisymConePressureLoadCard::DEFAULT_PHI_ANGLE1);
    card.phiAngle2 = reader.real(6, "phiAngle2", AxisymConePressureLoadCard::DEFAULT_PHI_ANGLE2);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConePressureLoadFormat::check(const AxisymConePressureLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.pressure, "pressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkRequiredId(card.ringId1, "ringId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.ringId2, "ringId2"))
        return problem;
    if (const auto problem = checkReal(card.phiAngle1, "phiAngle1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.phiAngle2, "phiAngle2", RealRule::Finite))
        return problem;
    if (card.phiAngle2 <= card.phiAngle1)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be above " + CardProblem::fieldNamed("phiAngle1"),
                           "phiAngle2"};
    return std::nullopt;
}

void AxisymConePressureLoadFormat::write(const AxisymConePressureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.pressure);
    writer.integer(3, card.ringId1);
    writer.integer(4, card.ringId2);
    writer.real(5, card.phiAngle1);
    writer.real(6, card.phiAngle2);
}

std::uint32_t AxisymConePressureLoadTable::append(const AxisymConePressureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_pressure.push(card.pressure);
    m_ringId1.push(card.ringId1);
    m_ringId2.push(card.ringId2);
    m_phiAngle1.push(card.phiAngle1);
    m_phiAngle2.push(card.phiAngle2);
    return row;
}

AxisymConePressureLoadCard AxisymConePressureLoadTable::card(std::uint32_t row) const
{
    AxisymConePressureLoadCard card;
    card.id = m_id.get(row);
    card.pressure = m_pressure.get(row);
    card.ringId1 = m_ringId1.get(row);
    card.ringId2 = m_ringId2.get(row);
    card.phiAngle1 = m_phiAngle1.get(row);
    card.phiAngle2 = m_phiAngle2.get(row);
    return card;
}

void AxisymConePressureLoadTable::store(std::uint32_t row, const AxisymConePressureLoadCard& card)
{
    m_id.set(row, card.id);
    m_pressure.set(row, card.pressure);
    m_ringId1.set(row, card.ringId1);
    m_ringId2.set(row, card.ringId2);
    m_phiAngle1.set(row, card.phiAngle1);
    m_phiAngle2.set(row, card.phiAngle2);
}

void AxisymConePressureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_pressure.keepRows(rows);
    m_ringId1.keepRows(rows);
    m_ringId2.keepRows(rows);
    m_phiAngle1.keepRows(rows);
    m_phiAngle2.keepRows(rows);
}

void AxisymConePressureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two rings are RINGAX cards, points of the node ID space.
    if (const base::EntityId ringId1 = m_ringId1.get(row); ringId1 > 0)
        m_ringId1.set(row, visitor.visit("ringId1", ReferenceTarget::Node, ringId1));
    if (const base::EntityId ringId2 = m_ringId2.get(row); ringId2 > 0)
        m_ringId2.set(row, visitor.visit("ringId2", ReferenceTarget::Node, ringId2));
}

} // namespace oor::model
