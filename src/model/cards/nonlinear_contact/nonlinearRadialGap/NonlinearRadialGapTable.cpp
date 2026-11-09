#include "oor/model/cards/nonlinear_contact/nonlinearRadialGap/NonlinearRadialGapTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int SMOOTHING_FIELD = 9;

// TABK is negative when it names a table of gap force against penetration: the table is the absolute value.
void visitSignedTable(base::EntityId& id, ReferenceVisitor& visitor, const char* name)
{
    if (id == 0)
        return;
    const base::EntityId sign = id < 0 ? -1 : 1;
    id = sign * visitor.visit(name, ReferenceTarget::DynamicTable, id < 0 ? -id : id);
}

} // namespace

std::optional<NonlinearRadialGapCard> NonlinearRadialGapFormat::read(FieldReader& reader)
{
    NonlinearRadialGapCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.plane = reader.isBlank(4) ? "PlaneXy" : std::string(reader.text(4));
    card.stiffnessTableId = reader.integer(5, "stiffnessTableId", 0);
    card.clearanceTableId = reader.integer(6, "clearanceTableId", 0);
    card.frictionTableId = reader.integer(7, "frictionTableId", 0);
    card.radius = reader.real(8, "radius", 0.0);
    if (!reader.isBlank(SMOOTHING_FIELD))
    {
        card.smoothing = (reader.text(SMOOTHING_FIELD) == "Gamma");
        if (!card.smoothing)
            reader.error("card.bad-value",
                         "the second line of an " + std::string(reader.cardName()) + " starts with GAMMA", "smoothing");
        card.smoothingValue = reader.real(SMOOTHING_FIELD + 1, "smoothingValue", 1.0E6);
    }
    else if (!reader.isBlank(SMOOTHING_FIELD + 1))
    {
        reader.error("card.bad-value", std::string("smoothingValue") + " needs the keyword GAMMA", "smoothingValue");
    }
    requireUnusedBlank(reader, {11, 12, 13, 14, 15, 16}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearRadialGapFormat::check(const NonlinearRadialGapCard& card)
{
    if (card.setId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "setId"};
    if (card.nodeA <= 0 || card.nodeB <= 0)
        return CardProblem{"card.bad-id",
                           CardProblem::fieldNamed("nodeA") + " and " + CardProblem::fieldNamed("nodeB") +
                               " must be positive grid point IDs",
                           card.nodeA <= 0 ? "nodeA" : "nodeB"};
    if (auto problem = checkKeyword(card.plane, "plane", {"PlaneXy", "PlaneYz", "PlaneZx"}))
        return problem;
    if (card.clearanceTableId < 0 || card.frictionTableId < 0)
        return CardProblem{"card.bad-id",
                           CardProblem::fieldNamed("clearanceTableId") + " and " +
                               CardProblem::fieldNamed("frictionTableId") + " must not be negative",
                           card.clearanceTableId < 0 ? "clearanceTableId" : "frictionTableId"};
    if (!(card.radius >= 0.0) || !std::isfinite(card.radius))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "radius"};
    if (!(card.smoothingValue > 0.0) || !std::isfinite(card.smoothingValue))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "smoothingValue"};
    // SMVAL belongs to the GAMMA line: a value without the keyword would not be written.
    if (!card.smoothing && card.smoothingValue != 1.0E6)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("smoothingValue") + " needs " + std::string(CardProblem::FIELD),
                           "smoothing"};
    return std::nullopt;
}

void NonlinearRadialGapFormat::write(const NonlinearRadialGapCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writer.textUnlessDefault(4, card.plane, "PlaneXy");
    if (card.stiffnessTableId != 0)
        writer.integer(5, card.stiffnessTableId);
    if (card.clearanceTableId != 0)
        writer.integer(6, card.clearanceTableId);
    if (card.frictionTableId != 0)
        writer.integer(7, card.frictionTableId);
    writer.realUnlessDefault(8, card.radius, 0.0);
    if (card.smoothing)
    {
        writer.text(SMOOTHING_FIELD, "Gamma");
        writer.realUnlessDefault(SMOOTHING_FIELD + 1, card.smoothingValue, 1.0E6);
    }
}

std::uint32_t NonlinearRadialGapTable::append(const NonlinearRadialGapCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_plane.push(card.plane);
    m_stiffnessTableId.push(card.stiffnessTableId);
    m_clearanceTableId.push(card.clearanceTableId);
    m_frictionTableId.push(card.frictionTableId);
    m_radius.push(card.radius);
    m_smoothing.push(card.smoothing ? 1 : 0);
    m_smoothingValue.push(card.smoothingValue);
    return row;
}

NonlinearRadialGapCard NonlinearRadialGapTable::card(std::uint32_t row) const
{
    NonlinearRadialGapCard card;
    card.setId = m_setId.get(row);
    card.nodeA = m_nodeA.get(row);
    card.nodeB = m_nodeB.get(row);
    card.plane = m_plane.get(row);
    card.stiffnessTableId = m_stiffnessTableId.get(row);
    card.clearanceTableId = m_clearanceTableId.get(row);
    card.frictionTableId = m_frictionTableId.get(row);
    card.radius = m_radius.get(row);
    card.smoothing = m_smoothing.get(row) != 0;
    card.smoothingValue = m_smoothingValue.get(row);
    return card;
}

void NonlinearRadialGapTable::store(std::uint32_t row, const NonlinearRadialGapCard& card)
{
    m_setId.set(row, card.setId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_plane.set(row, card.plane);
    m_stiffnessTableId.set(row, card.stiffnessTableId);
    m_clearanceTableId.set(row, card.clearanceTableId);
    m_frictionTableId.set(row, card.frictionTableId);
    m_radius.set(row, card.radius);
    m_smoothing.set(row, card.smoothing ? 1 : 0);
    m_smoothingValue.set(row, card.smoothingValue);
}

void NonlinearRadialGapTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_plane.keepRows(rows);
    m_stiffnessTableId.keepRows(rows);
    m_clearanceTableId.keepRows(rows);
    m_frictionTableId.keepRows(rows);
    m_radius.keepRows(rows);
    m_smoothing.keepRows(rows);
    m_smoothingValue.keepRows(rows);
}

void NonlinearRadialGapTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
    base::EntityId stiffnessTable = m_stiffnessTableId.get(row);
    visitSignedTable(stiffnessTable, visitor, "stiffnessTableId");
    m_stiffnessTableId.set(row, stiffnessTable);
    if (const base::EntityId clearanceTable = m_clearanceTableId.get(row); clearanceTable != 0)
        m_clearanceTableId.set(row, visitor.visit("clearanceTableId", ReferenceTarget::DynamicTable, clearanceTable));
    if (const base::EntityId frictionTable = m_frictionTableId.get(row); frictionTable != 0)
        m_frictionTableId.set(row, visitor.visit("frictionTableId", ReferenceTarget::DynamicTable, frictionTable));
}

} // namespace oor::model
