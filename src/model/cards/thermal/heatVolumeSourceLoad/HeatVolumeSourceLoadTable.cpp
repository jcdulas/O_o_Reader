#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadTable.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatVolumeSourceLoadCard> HeatVolumeSourceLoadFormat::read(FieldReader& reader)
{
    HeatVolumeSourceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.heatGeneration = reader.requiredReal(2, "heatGeneration");
    card.controlNode = reader.integer(3, "controlNode", 0);
    card.elements = readThermalRuns(reader, 4, "elements", true);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatVolumeSourceLoadFormat::check(const HeatVolumeSourceLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.controlNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "controlNode"};
    return checkThermalRuns(card.elements, "elements", false);
}

void HeatVolumeSourceLoadFormat::write(const HeatVolumeSourceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.heatGeneration);
    writer.integerUnlessDefault(3, card.controlNode, 0);
    writeThermalRuns(writer, 4, card.elements);
}

std::uint32_t HeatVolumeSourceLoadTable::append(const HeatVolumeSourceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_heatGeneration.push(card.heatGeneration);
    m_controlNode.push(card.controlNode);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

HeatVolumeSourceLoadCard HeatVolumeSourceLoadTable::card(std::uint32_t row) const
{
    HeatVolumeSourceLoadCard card;
    card.id = m_id.get(row);
    card.heatGeneration = m_heatGeneration.get(row);
    card.controlNode = m_controlNode.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void HeatVolumeSourceLoadTable::store(std::uint32_t row, const HeatVolumeSourceLoadCard& card)
{
    m_id.set(row, card.id);
    m_heatGeneration.set(row, card.heatGeneration);
    m_controlNode.set(row, card.controlNode);
    m_runs.set(row, toThermalTriples(card.elements));
}

void HeatVolumeSourceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_heatGeneration.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_runs.keepRows(rows);
}

void HeatVolumeSourceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId controlNode = m_controlNode.get(row); controlNode > 0)
        m_controlNode.set(row, visitor.visit("controlNode", ReferenceTarget::ScalarOrGridPoint, controlNode));
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
}

} // namespace oor::model
