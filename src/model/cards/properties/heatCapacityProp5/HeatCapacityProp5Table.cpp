#include "oor/model/cards/properties/heatCapacityProp5/HeatCapacityProp5Table.hpp"

namespace oor::model
{

std::optional<HeatCapacityProp5Card> HeatCapacityProp5Format::read(FieldReader& reader)
{
    HeatCapacityProp5Card card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.heatCapacity = reader.requiredReal(3, "heatCapacity");
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has three fields", "heatCapacity");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatCapacityProp5Format::check(const HeatCapacityProp5Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (!(card.heatCapacity > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "heatCapacity"};
    return std::nullopt;
}

void HeatCapacityProp5Format::write(const HeatCapacityProp5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.heatCapacity);
}

std::uint32_t HeatCapacityProp5Table::append(const HeatCapacityProp5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_heatCapacity.push(card.heatCapacity);
    return row;
}

HeatCapacityProp5Card HeatCapacityProp5Table::card(std::uint32_t row) const
{
    return HeatCapacityProp5Card{m_id.get(row), m_matId.get(row), m_heatCapacity.get(row)};
}

void HeatCapacityProp5Table::store(std::uint32_t row, const HeatCapacityProp5Card& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_heatCapacity.set(row, card.heatCapacity);
}

void HeatCapacityProp5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_heatCapacity.keepRows(rows);
}

void HeatCapacityProp5Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
