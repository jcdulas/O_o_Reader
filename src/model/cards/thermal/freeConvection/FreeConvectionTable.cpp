#include "oor/model/cards/thermal/freeConvection/FreeConvectionTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

std::optional<FreeConvectionCard> FreeConvectionFormat::read(FieldReader& reader)
{
    FreeConvectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.convectionPropId = reader.requiredInteger(2, "convectionPropId");
    card.filmNode = reader.integer(3, "filmNode", 0);
    card.controlNode = reader.integer(4, "controlNode", 0);
    card.ambientNodes[0] = reader.requiredInteger(5, "ambientNode1");
    // format reference: a blank ambient point from TA2 on is TA1; an explicit 0 is no point.
    for (std::size_t index = 1; index < FreeConvectionCard::AMBIENT_COUNT; ++index)
    {
        const std::string name = "TA" + std::to_string(index + 1);
        card.ambientNodes[index] = reader.integer(5 + static_cast<int>(index), name.c_str(), card.ambientNodes[0]);
    }
    requireUnusedBlank(reader, {}, 12);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeConvectionFormat::check(const FreeConvectionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the surface element ID must be a positive integer", "id"};
    if (card.convectionPropId <= 0)
        return CardProblem{"card.bad-id", "the free convection property ID must be a positive integer",
                           "convectionPropId"};
    if (card.filmNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "filmNode"};
    if (card.controlNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "controlNode"};
    if (card.ambientNodes[0] <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive point ID",
                           "ambientNode1"};
    for (std::size_t index = 1; index < FreeConvectionCard::AMBIENT_COUNT; ++index)
    {
        if (card.ambientNodes[index] < 0)
            return CardProblem{"card.bad-id", "TA" + std::to_string(index + 1) + " cannot be negative", "ambientNode1"};
    }
    return std::nullopt;
}

void FreeConvectionFormat::write(const FreeConvectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.convectionPropId);
    writer.integerUnlessDefault(3, card.filmNode, 0);
    writer.integerUnlessDefault(4, card.controlNode, 0);
    // TA2 to TA8 default to TA1: all are written.
    for (std::size_t index = 0; index < FreeConvectionCard::AMBIENT_COUNT; ++index)
        writer.integer(5 + static_cast<int>(index), card.ambientNodes[index]);
}

std::uint32_t FreeConvectionTable::append(const FreeConvectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_convectionPropId.push(card.convectionPropId);
    m_filmNode.push(card.filmNode);
    m_controlNode.push(card.controlNode);
    m_ambientNodes.push(card.ambientNodes);
    return row;
}

FreeConvectionCard FreeConvectionTable::card(std::uint32_t row) const
{
    FreeConvectionCard card;
    card.id = m_id.get(row);
    card.convectionPropId = m_convectionPropId.get(row);
    card.filmNode = m_filmNode.get(row);
    card.controlNode = m_controlNode.get(row);
    card.ambientNodes = m_ambientNodes.get(row);
    return card;
}

void FreeConvectionTable::store(std::uint32_t row, const FreeConvectionCard& card)
{
    m_id.set(row, card.id);
    m_convectionPropId.set(row, card.convectionPropId);
    m_filmNode.set(row, card.filmNode);
    m_controlNode.set(row, card.controlNode);
    m_ambientNodes.set(row, card.ambientNodes);
}

void FreeConvectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_convectionPropId.keepRows(rows);
    m_filmNode.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_ambientNodes.keepRows(rows);
}

void FreeConvectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the surface element the boundary condition applies to.
    m_id.set(row, visitor.visit("id", ReferenceTarget::SurfaceElement, m_id.get(row)));
    m_convectionPropId.set(
        row, visitor.visit("convectionPropId", ReferenceTarget::ConvectionProperty, m_convectionPropId.get(row)));
    if (const base::EntityId filmNode = m_filmNode.get(row); filmNode > 0)
        m_filmNode.set(row, visitor.visit("filmNode", ReferenceTarget::ScalarOrGridPoint, filmNode));
    if (const base::EntityId controlNode = m_controlNode.get(row); controlNode > 0)
        m_controlNode.set(row, visitor.visit("controlNode", ReferenceTarget::ScalarOrGridPoint, controlNode));
    std::array<base::EntityId, FreeConvectionCard::AMBIENT_COUNT> ambientNodes = m_ambientNodes.get(row);
    bool changed = false;
    for (base::EntityId& point : ambientNodes)
    {
        if (point <= 0)
            continue;
        const base::EntityId updated = visitor.visit("ambientNode1", ReferenceTarget::ScalarOrGridPoint, point);
        changed = changed || updated != point;
        point = updated;
    }
    if (changed)
        m_ambientNodes.set(row, ambientNodes);
}

} // namespace oor::model
