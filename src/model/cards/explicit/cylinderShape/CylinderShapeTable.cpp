#include "oor/model/cards/explicit/cylinderShape/CylinderShapeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CylinderShapeCard> CylinderShapeFormat::read(FieldReader& reader)
{
    CylinderShapeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.centerX1 = reader.requiredReal(2, "centerX1");
    card.centerY1 = reader.requiredReal(3, "centerY1");
    card.centerZ1 = reader.requiredReal(4, "centerZ1");
    card.centerX2 = reader.requiredReal(5, "centerX2");
    card.centerY2 = reader.requiredReal(6, "centerY2");
    card.centerZ2 = reader.requiredReal(7, "centerZ2");
    card.radius = reader.requiredReal(8, "radius");
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CylinderShapeFormat::check(const CylinderShapeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.centerX1, "centerX1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerY1, "centerY1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerZ1, "centerZ1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerX2, "centerX2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerY2, "centerY2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerZ2, "centerZ2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.radius, "radius", RealRule::Positive))
        return problem;
    // Two end points at the same place give no axis (the cylinder is defined by the axis and the radius).
    if (card.centerX1 == card.centerX2 && card.centerY1 == card.centerY2 && card.centerZ1 == card.centerZ2)
        return CardProblem{"card.bad-value", "the two end points of the axis must be different", "centerX2"};
    return std::nullopt;
}

void CylinderShapeFormat::write(const CylinderShapeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.centerX1);
    writer.real(3, card.centerY1);
    writer.real(4, card.centerZ1);
    writer.real(5, card.centerX2);
    writer.real(6, card.centerY2);
    writer.real(7, card.centerZ2);
    writer.real(8, card.radius);
}

std::uint32_t CylinderShapeTable::append(const CylinderShapeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CylinderShapeTable::store(std::uint32_t row, const CylinderShapeCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CylinderShapeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CylinderShapeTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
