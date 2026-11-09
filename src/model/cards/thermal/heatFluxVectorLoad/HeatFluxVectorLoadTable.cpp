#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadTable.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <variant>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> given(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

HeatFluxVectorLoadTable::HeatFluxVectorLoadTable()
    : m_heatFlux(notGiven()), m_sourceTemperature(notGiven()), m_directionFrameId(NO_DIRECTION_FRAME)
{
}

std::optional<HeatFluxVectorLoadCard> HeatFluxVectorLoadFormat::read(FieldReader& reader)
{
    HeatFluxVectorLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.heatFlux = reader.optionalReal(2, "heatFlux");
    card.sourceTemperature = reader.optionalReal(3, "sourceTemperature");
    card.directionFrameId = reader.optionalInteger(4, "directionFrameId");
    // Each component of the direction is a real, or an integer that is the TABLEDi of its variation with time.
    for (std::size_t index = 0; index < 3; ++index)
    {
        const std::string name = "direction" + std::to_string(index + 1);
        const auto value = reader.integerOrReal(5 + static_cast<int>(index), name.c_str());
        if (!value)
            continue;
        if (std::holds_alternative<std::int32_t>(*value))
            card.tableId[index] = std::get<std::int32_t>(*value);
        else
            card.direction[index] = std::get<double>(*value);
    }
    card.controlNode = reader.integer(8, "controlNode", 0);
    card.elements = readThermalRuns(reader, 9, "elements", false);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatFluxVectorLoadFormat::check(const HeatFluxVectorLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.directionFrameId && *card.directionFrameId < -1)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be -1, 0 or a coordinate system ID (or blank)",
                           "directionFrameId"};
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (card.tableId[index] < 0)
            return CardProblem{"card.bad-id",
                               std::string("a ") + std::string(CardProblem::FIELD) + " must be a positive table ID",
                               "tableId"};
        if (card.tableId[index] != 0 && card.direction[index] != 0.0)
            return CardProblem{"card.bad-value", "a component is a real or a table, not both", "direction"};
    }
    if (card.controlNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "controlNode"};
    for (const ThermalRun& run : card.elements)
    {
        if (run.increment != 1)
            return CardProblem{"card.bad-value",
                               std::string("a range of this card has no ") + std::string(CardProblem::FIELD),
                               "increment"};
    }
    // the continuation entry, the elements, is required.
    return checkThermalRuns(card.elements, "elements", false);
}

void HeatFluxVectorLoadFormat::write(const HeatFluxVectorLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.heatFlux)
        writer.real(2, *card.heatFlux);
    if (card.sourceTemperature)
        writer.real(3, *card.sourceTemperature);
    if (card.directionFrameId)
        writer.integer(4, *card.directionFrameId);
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (card.tableId[index] != 0)
            writer.integer(5 + static_cast<int>(index), card.tableId[index]);
        else
            writer.realUnlessDefault(5 + static_cast<int>(index), card.direction[index], 0.0);
    }
    writer.integerUnlessDefault(8, card.controlNode, 0);
    writeThermalRuns(writer, 9, card.elements);
}

std::uint32_t HeatFluxVectorLoadTable::append(const HeatFluxVectorLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_heatFlux.push(card.heatFlux.value_or(notGiven()));
    m_sourceTemperature.push(card.sourceTemperature.value_or(notGiven()));
    m_directionFrameId.push(card.directionFrameId.value_or(NO_DIRECTION_FRAME));
    m_direction.push(card.direction);
    m_tableId.push(card.tableId);
    m_controlNode.push(card.controlNode);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

HeatFluxVectorLoadCard HeatFluxVectorLoadTable::card(std::uint32_t row) const
{
    HeatFluxVectorLoadCard card;
    card.id = m_id.get(row);
    card.heatFlux = given(m_heatFlux.get(row));
    card.sourceTemperature = given(m_sourceTemperature.get(row));
    if (const std::int32_t directionFrameId = m_directionFrameId.get(row); directionFrameId != NO_DIRECTION_FRAME)
        card.directionFrameId = directionFrameId;
    card.direction = m_direction.get(row);
    card.tableId = m_tableId.get(row);
    card.controlNode = m_controlNode.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void HeatFluxVectorLoadTable::store(std::uint32_t row, const HeatFluxVectorLoadCard& card)
{
    m_id.set(row, card.id);
    m_heatFlux.set(row, card.heatFlux.value_or(notGiven()));
    m_sourceTemperature.set(row, card.sourceTemperature.value_or(notGiven()));
    m_directionFrameId.set(row, card.directionFrameId.value_or(NO_DIRECTION_FRAME));
    m_direction.set(row, card.direction);
    m_tableId.set(row, card.tableId);
    m_controlNode.set(row, card.controlNode);
    m_runs.set(row, toThermalTriples(card.elements));
}

void HeatFluxVectorLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_heatFlux.keepRows(rows);
    m_sourceTemperature.keepRows(rows);
    m_directionFrameId.keepRows(rows);
    m_direction.keepRows(rows);
    m_tableId.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_runs.keepRows(rows);
}

void HeatFluxVectorLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const std::int32_t directionFrameId = m_directionFrameId.get(row); directionFrameId > 0)
        m_directionFrameId.set(row, visitor.visit("directionFrameId", ReferenceTarget::Coordinate, directionFrameId));
    std::array<base::EntityId, 3> tableIds = m_tableId.get(row);
    bool changed = false;
    for (base::EntityId& table : tableIds)
    {
        if (table <= 0)
            continue;
        const base::EntityId updated = visitor.visit("tableId", ReferenceTarget::DynamicTable, table);
        changed = changed || updated != table;
        table = updated;
    }
    if (changed)
        m_tableId.set(row, tableIds);
    if (const base::EntityId controlNode = m_controlNode.get(row); controlNode > 0)
        m_controlNode.set(row, visitor.visit("controlNode", ReferenceTarget::ScalarOrGridPoint, controlNode));
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::SurfaceElement,
                     ReferenceTarget::SurfaceElementInRange);
}

} // namespace oor::model
