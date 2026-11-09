#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

ForcedConvectionTable::ForcedConvectionTable() : m_massFlowFactor(1.0) {}

std::optional<ForcedConvectionCard> ForcedConvectionFormat::read(FieldReader& reader)
{
    ForcedConvectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.convectionPropId = reader.requiredInteger(2, "convectionPropId");
    card.filmNode = reader.integer(3, "filmNode", 0);
    card.massFlowNode = reader.integer(4, "massFlowNode", 0);
    card.ambientNode1 = reader.requiredInteger(5, "ambientNode1");
    card.ambientNode2 =
        reader.integer(6, "ambientNode2", card.ambientNode1); // format reference: the default of TA2 is TA1
    card.massFlowFactor = reader.real(7, "massFlowFactor", 1.0);
    requireUnusedBlank(reader, {}, 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ForcedConvectionFormat::check(const ForcedConvectionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the surface element ID must be a positive integer", "id"};
    if (card.convectionPropId <= 0)
        return CardProblem{"card.bad-id", "the forced convection property ID must be a positive integer",
                           "convectionPropId"};
    if (card.filmNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "filmNode"};
    if (card.massFlowNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "massFlowNode"};
    if (card.ambientNode1 <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive point ID",
                           "ambientNode1"};
    if (card.ambientNode2 < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "ambientNode2"};
    if (!(card.massFlowFactor >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "massFlowFactor"};
    // with no control point the mass flow rate comes from MDOT.
    if (card.massFlowNode == 0 && !(card.massFlowFactor > 0.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 when " +
                               CardProblem::fieldNamed("massFlowNode") + " is blank",
                           "massFlowFactor"};
    return std::nullopt;
}

void ForcedConvectionFormat::write(const ForcedConvectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.convectionPropId);
    writer.integerUnlessDefault(3, card.filmNode, 0);
    writer.integerUnlessDefault(4, card.massFlowNode, 0);
    writer.integer(5, card.ambientNode1);
    // TA2 defaults to TA1: always written.
    writer.integer(6, card.ambientNode2);
    writer.realUnlessDefault(7, card.massFlowFactor, 1.0);
}

std::uint32_t ForcedConvectionTable::append(const ForcedConvectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_convectionPropId.push(card.convectionPropId);
    m_filmNode.push(card.filmNode);
    m_massFlowNode.push(card.massFlowNode);
    m_ambientNode1.push(card.ambientNode1);
    m_ambientNode2.push(card.ambientNode2);
    m_massFlowFactor.push(card.massFlowFactor);
    return row;
}

ForcedConvectionCard ForcedConvectionTable::card(std::uint32_t row) const
{
    ForcedConvectionCard card;
    card.id = m_id.get(row);
    card.convectionPropId = m_convectionPropId.get(row);
    card.filmNode = m_filmNode.get(row);
    card.massFlowNode = m_massFlowNode.get(row);
    card.ambientNode1 = m_ambientNode1.get(row);
    card.ambientNode2 = m_ambientNode2.get(row);
    card.massFlowFactor = m_massFlowFactor.get(row);
    return card;
}

void ForcedConvectionTable::store(std::uint32_t row, const ForcedConvectionCard& card)
{
    m_id.set(row, card.id);
    m_convectionPropId.set(row, card.convectionPropId);
    m_filmNode.set(row, card.filmNode);
    m_massFlowNode.set(row, card.massFlowNode);
    m_ambientNode1.set(row, card.ambientNode1);
    m_ambientNode2.set(row, card.ambientNode2);
    m_massFlowFactor.set(row, card.massFlowFactor);
}

void ForcedConvectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_convectionPropId.keepRows(rows);
    m_filmNode.keepRows(rows);
    m_massFlowNode.keepRows(rows);
    m_ambientNode1.keepRows(rows);
    m_ambientNode2.keepRows(rows);
    m_massFlowFactor.keepRows(rows);
}

void ForcedConvectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_id.set(row, visitor.visit("id", ReferenceTarget::SurfaceElement, m_id.get(row)));
    m_convectionPropId.set(
        row, visitor.visit("convectionPropId", ReferenceTarget::ForcedConvectionProperty, m_convectionPropId.get(row)));
    if (const base::EntityId filmNode = m_filmNode.get(row); filmNode > 0)
        m_filmNode.set(row, visitor.visit("filmNode", ReferenceTarget::ScalarOrGridPoint, filmNode));
    if (const base::EntityId massFlowNode = m_massFlowNode.get(row); massFlowNode > 0)
        m_massFlowNode.set(row, visitor.visit("massFlowNode", ReferenceTarget::ScalarOrGridPoint, massFlowNode));
    m_ambientNode1.set(row, visitor.visit("ambientNode1", ReferenceTarget::ScalarOrGridPoint, m_ambientNode1.get(row)));
    if (const base::EntityId ambientNode2 = m_ambientNode2.get(row); ambientNode2 > 0)
        m_ambientNode2.set(row, visitor.visit("ambientNode2", ReferenceTarget::ScalarOrGridPoint, ambientNode2));
}

} // namespace oor::model
