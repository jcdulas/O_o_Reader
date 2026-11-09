#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"

namespace oor::model
{

std::optional<RodPropCard> RodPropFormat::read(FieldReader& reader)
{
    RodPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.area = reader.requiredReal(3, "area");
    card.torsion = reader.real(4, "torsionConstant", 0.0);
    card.stressCoefficient = reader.real(5, "torsionStressCoefficient", 0.0);
    card.nonStructMass = reader.real(6, "nonStructMass", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RodPropFormat::check(const RodPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    return std::nullopt;
}

void RodPropFormat::write(const RodPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.area);
    writer.realUnlessDefault(4, card.torsion, 0.0);
    writer.realUnlessDefault(5, card.stressCoefficient, 0.0);
    writer.realUnlessDefault(6, card.nonStructMass, 0.0);
}

std::uint32_t RodPropTable::append(const RodPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_area.push(card.area);
    m_torsion.push(card.torsion);
    m_stressCoefficient.push(card.stressCoefficient);
    m_nonStructMass.push(card.nonStructMass);
    return row;
}

RodPropCard RodPropTable::card(std::uint32_t row) const
{
    return RodPropCard{
        m_id.get(row),           m_matId.get(row), m_area.get(row), m_torsion.get(row), m_stressCoefficient.get(row),
        m_nonStructMass.get(row)};
}

void RodPropTable::store(std::uint32_t row, const RodPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_area.set(row, card.area);
    m_torsion.set(row, card.torsion);
    m_stressCoefficient.set(row, card.stressCoefficient);
    m_nonStructMass.set(row, card.nonStructMass);
}

void RodPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_area.keepRows(rows);
    m_torsion.keepRows(rows);
    m_stressCoefficient.keepRows(rows);
    m_nonStructMass.keepRows(rows);
}

void RodPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
