#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"

namespace oor::model
{

std::optional<ShearPanelPropCard> ShearPanelPropFormat::read(FieldReader& reader)
{
    ShearPanelPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.thickness = reader.requiredReal(3, "thickness");
    card.nonStructMass = reader.real(4, "nonStructMass", 0.0);
    card.extensionalFactor1 = reader.real(5, "extensionalFactor1", 0.0);
    card.extensionalFactor2 = reader.real(6, "extensionalFactor2", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShearPanelPropFormat::check(const ShearPanelPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.thickness < 0.0)
        return CardProblem{"card.bad-value", "T must not be negative", "thickness"};
    if (card.extensionalFactor1 < 0.0)
        return CardProblem{"card.bad-value", "F1 must not be negative", "extensionalFactor1"};
    if (card.extensionalFactor2 < 0.0)
        return CardProblem{"card.bad-value", "F2 must not be negative", "extensionalFactor2"};
    return std::nullopt;
}

void ShearPanelPropFormat::write(const ShearPanelPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.thickness);
    writer.realUnlessDefault(4, card.nonStructMass, 0.0);
    writer.realUnlessDefault(5, card.extensionalFactor1, 0.0);
    writer.realUnlessDefault(6, card.extensionalFactor2, 0.0);
}

std::uint32_t ShearPanelPropTable::append(const ShearPanelPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_thickness.push(card.thickness);
    m_nonStructMass.push(card.nonStructMass);
    m_extensionalFactor1.push(card.extensionalFactor1);
    m_extensionalFactor2.push(card.extensionalFactor2);
    return row;
}

ShearPanelPropCard ShearPanelPropTable::card(std::uint32_t row) const
{
    return ShearPanelPropCard{m_id.get(row),
                              m_matId.get(row),
                              m_thickness.get(row),
                              m_nonStructMass.get(row),
                              m_extensionalFactor1.get(row),
                              m_extensionalFactor2.get(row)};
}

void ShearPanelPropTable::store(std::uint32_t row, const ShearPanelPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_thickness.set(row, card.thickness);
    m_nonStructMass.set(row, card.nonStructMass);
    m_extensionalFactor1.set(row, card.extensionalFactor1);
    m_extensionalFactor2.set(row, card.extensionalFactor2);
}

void ShearPanelPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_thickness.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_extensionalFactor1.keepRows(rows);
    m_extensionalFactor2.keepRows(rows);
}

void ShearPanelPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
