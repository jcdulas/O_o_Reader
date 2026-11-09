#include "oor/model/cards/superelements/axisymConeSector/AxisymConeSectorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<AxisymConeSectorCard> AxisymConeSectorFormat::read(FieldReader& reader)
{
    AxisymConeSectorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ringId = reader.requiredInteger(2, "ringId");
    card.radius = reader.requiredReal(3, "radius");
    card.phiAngle1 = reader.requiredReal(4, "phiAngle1");
    card.phiAngle2 = reader.requiredReal(5, "phiAngle2");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConeSectorFormat::check(const AxisymConeSectorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.ringId, "ringId"))
        return problem;
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.phiAngle1, "phiAngle1", RealRule::Finite))
        return problem;
    return checkReal(card.phiAngle2, "phiAngle2", RealRule::Finite);
}

void AxisymConeSectorFormat::write(const AxisymConeSectorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    writer.real(3, card.radius);
    writer.real(4, card.phiAngle1);
    writer.real(5, card.phiAngle2);
}

std::uint32_t AxisymConeSectorTable::append(const AxisymConeSectorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_radius.push(card.radius);
    m_phiAngle1.push(card.phiAngle1);
    m_phiAngle2.push(card.phiAngle2);
    return row;
}

AxisymConeSectorCard AxisymConeSectorTable::card(std::uint32_t row) const
{
    return AxisymConeSectorCard{m_id.get(row), m_ringId.get(row), m_radius.get(row), m_phiAngle1.get(row),
                                m_phiAngle2.get(row)};
}

void AxisymConeSectorTable::store(std::uint32_t row, const AxisymConeSectorCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_radius.set(row, card.radius);
    m_phiAngle1.set(row, card.phiAngle1);
    m_phiAngle2.set(row, card.phiAngle2);
}

void AxisymConeSectorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_radius.keepRows(rows);
    m_phiAngle1.keepRows(rows);
    m_phiAngle2.keepRows(rows);
}

void AxisymConeSectorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, m_ringId.get(row)));
}

} // namespace oor::model
