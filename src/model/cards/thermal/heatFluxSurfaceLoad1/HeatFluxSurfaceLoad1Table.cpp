#include "oor/model/cards/thermal/heatFluxSurfaceLoad1/HeatFluxSurfaceLoad1Table.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatFluxSurfaceLoad1Card> HeatFluxSurfaceLoad1Format::read(FieldReader& reader)
{
    HeatFluxSurfaceLoad1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.heatFlux = reader.requiredReal(2, "heatFlux");
    card.elements = readThermalRuns(reader, 3, "elements", false);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatFluxSurfaceLoad1Format::check(const HeatFluxSurfaceLoad1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    for (const ThermalRun& run : card.elements)
    {
        if (run.increment != 1)
            return CardProblem{"card.bad-value",
                               std::string("a range of this card has no ") + std::string(CardProblem::FIELD),
                               "increment"};
    }
    return checkThermalRuns(card.elements, "elements", true);
}

void HeatFluxSurfaceLoad1Format::write(const HeatFluxSurfaceLoad1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.heatFlux);
    writeThermalRuns(writer, 3, card.elements);
}

std::uint32_t HeatFluxSurfaceLoad1Table::append(const HeatFluxSurfaceLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_heatFlux.push(card.heatFlux);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

HeatFluxSurfaceLoad1Card HeatFluxSurfaceLoad1Table::card(std::uint32_t row) const
{
    HeatFluxSurfaceLoad1Card card;
    card.id = m_id.get(row);
    card.heatFlux = m_heatFlux.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void HeatFluxSurfaceLoad1Table::store(std::uint32_t row, const HeatFluxSurfaceLoad1Card& card)
{
    m_id.set(row, card.id);
    m_heatFlux.set(row, card.heatFlux);
    m_runs.set(row, toThermalTriples(card.elements));
}

void HeatFluxSurfaceLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_heatFlux.keepRows(rows);
    m_runs.keepRows(rows);
}

void HeatFluxSurfaceLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::SurfaceElement,
                     ReferenceTarget::SurfaceElementInRange);
}

} // namespace oor::model
