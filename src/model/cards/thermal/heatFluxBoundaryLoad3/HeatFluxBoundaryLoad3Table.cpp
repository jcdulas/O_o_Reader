#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Table.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatFluxBoundaryLoad3Card> HeatFluxBoundaryLoad3Format::read(FieldReader& reader)
{
    HeatFluxBoundaryLoad3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.heatFlux = reader.requiredReal(2, "heatFlux");
    card.controlNode = reader.integer(3, "controlNode", 0);
    card.elements = readThermalRuns(reader, 4, "elements", true);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatFluxBoundaryLoad3Format::check(const HeatFluxBoundaryLoad3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.controlNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "controlNode"};
    return checkThermalRuns(card.elements, "elements", true);
}

void HeatFluxBoundaryLoad3Format::write(const HeatFluxBoundaryLoad3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.heatFlux);
    writer.integerUnlessDefault(3, card.controlNode, 0);
    writeThermalRuns(writer, 4, card.elements);
}

std::uint32_t HeatFluxBoundaryLoad3Table::append(const HeatFluxBoundaryLoad3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_heatFlux.push(card.heatFlux);
    m_controlNode.push(card.controlNode);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

HeatFluxBoundaryLoad3Card HeatFluxBoundaryLoad3Table::card(std::uint32_t row) const
{
    HeatFluxBoundaryLoad3Card card;
    card.id = m_id.get(row);
    card.heatFlux = m_heatFlux.get(row);
    card.controlNode = m_controlNode.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void HeatFluxBoundaryLoad3Table::store(std::uint32_t row, const HeatFluxBoundaryLoad3Card& card)
{
    m_id.set(row, card.id);
    m_heatFlux.set(row, card.heatFlux);
    m_controlNode.set(row, card.controlNode);
    m_runs.set(row, toThermalTriples(card.elements));
}

void HeatFluxBoundaryLoad3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_heatFlux.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_runs.keepRows(rows);
}

void HeatFluxBoundaryLoad3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId controlNode = m_controlNode.get(row); controlNode > 0)
        m_controlNode.set(row, visitor.visit("controlNode", ReferenceTarget::ScalarOrGridPoint, controlNode));
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::SurfaceElement,
                     ReferenceTarget::SurfaceElementInRange);
}

} // namespace oor::model
