#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"

namespace oor::model
{

std::optional<SpringScalarPropertyCard> SpringScalarPropertyFormat::read(FieldReader& reader)
{
    SpringScalarPropertyCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id);       // format reference: PID defaults to EID
    card.scalarPoint1 = reader.integer(3, "scalarPoint1", 0); // blank: grounded
    card.scalarPoint2 = reader.integer(4, "scalarPoint2", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SpringScalarPropertyFormat::check(const SpringScalarPropertyCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.scalarPoint1 < 0 || card.scalarPoint2 < 0)
        return CardProblem{"card.bad-id", "the scalar point IDs must not be negative", "scalarPoint1"};
    if (card.scalarPoint1 == card.scalarPoint2)
        return CardProblem{"card.duplicate-node", "the two scalar points must be different", "scalarPoint2"};
    return std::nullopt;
}

void SpringScalarPropertyFormat::write(const SpringScalarPropertyCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.scalarPoint1, 0);
    writer.integerUnlessDefault(4, card.scalarPoint2, 0);
}

std::uint32_t SpringScalarPropertyTable::append(const SpringScalarPropertyCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_scalarPoint1.push(card.scalarPoint1);
    m_scalarPoint2.push(card.scalarPoint2);
    return row;
}

SpringScalarPropertyCard SpringScalarPropertyTable::card(std::uint32_t row) const
{
    return SpringScalarPropertyCard{m_id.get(row), m_propId.get(row), m_scalarPoint1.get(row), m_scalarPoint2.get(row)};
}

void SpringScalarPropertyTable::store(std::uint32_t row, const SpringScalarPropertyCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_scalarPoint1.set(row, card.scalarPoint1);
    m_scalarPoint2.set(row, card.scalarPoint2);
}

void SpringScalarPropertyTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_scalarPoint1.keepRows(rows);
    m_scalarPoint2.keepRows(rows);
}

void SpringScalarPropertyTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    // S1 and S2 are scalar points (SPOINT cards, or implicit: the format reference does not require an SPOINT entry for
    // them).
    if (const base::EntityId scalarPoint1 = m_scalarPoint1.get(row); scalarPoint1 > 0)
        m_scalarPoint1.set(row, visitor.visit("scalarPoint1", ReferenceTarget::ScalarOrGridPoint, scalarPoint1));
    if (const base::EntityId scalarPoint2 = m_scalarPoint2.get(row); scalarPoint2 > 0)
        m_scalarPoint2.set(row, visitor.visit("scalarPoint2", ReferenceTarget::ScalarOrGridPoint, scalarPoint2));
}

} // namespace oor::model
