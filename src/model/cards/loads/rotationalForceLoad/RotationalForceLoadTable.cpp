#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadTable.hpp"

namespace oor::model
{

std::optional<RotationalForceLoadCard> RotationalForceLoadFormat::read(FieldReader& reader)
{
    RotationalForceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.integer(2, "node", 0);
    card.frameId = reader.integer(3, "frameId", 0);
    // format reference: A has no default; a blank real is 0.0.
    card.magnitude = reader.real(4, "magnitude", 0.0);
    card.axis[0] = reader.real(5, "radius1", 0.0);
    card.axis[1] = reader.real(6, "radius2", 0.0);
    card.axis[2] = reader.real(7, "radius3", 0.0);
    card.methodName = reader.integer(8, "methodName", 1);
    card.angularAcceleration = reader.real(9, "angularAcceleration", 0.0);
    card.mainBulkFlag = reader.integer(10, "mainBulkFlag", 0);
    card.regionId = reader.integer(11, "regionId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotationalForceLoadFormat::check(const RotationalForceLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.node < 0)
        return CardProblem{"card.bad-id", "the grid point ID cannot be negative", "node"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-value", "the coordinate system ID cannot be negative", "frameId"};
    if (card.methodName != 1 && card.methodName != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "methodName"};
    if (card.regionId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "regionId"};
    // format reference: the rotation vector is not zero unless A and RACC both are.
    if (card.axis[0] == 0.0 && card.axis[1] == 0.0 && card.axis[2] == 0.0 &&
        (card.magnitude != 0.0 || card.angularAcceleration != 0.0))
        return CardProblem{"card.bad-value",
                           "the rotation vector (" + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("radius2") + ", " + CardProblem::fieldNamed("radius3") +
                               ") must not be zero",
                           "radius1"};
    return std::nullopt;
}

void RotationalForceLoadFormat::write(const RotationalForceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.node, 0);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.realUnlessDefault(4, card.magnitude, 0.0);
    writer.realUnlessDefault(5, card.axis[0], 0.0);
    writer.realUnlessDefault(6, card.axis[1], 0.0);
    writer.realUnlessDefault(7, card.axis[2], 0.0);
    writer.integerUnlessDefault(8, card.methodName, 1);
    writer.realUnlessDefault(9, card.angularAcceleration, 0.0);
    writer.integerUnlessDefault(10, card.mainBulkFlag, 0);
    writer.integerUnlessDefault(11, card.regionId, 0);
}

std::uint32_t RotationalForceLoadTable::append(const RotationalForceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_frameId.push(card.frameId);
    m_magnitude.push(card.magnitude);
    m_axis.push(card.axis);
    m_methodName.push(card.methodName);
    m_angularAcceleration.push(card.angularAcceleration);
    m_mainBulkFlag.push(card.mainBulkFlag);
    m_regionId.push(card.regionId);
    return row;
}

RotationalForceLoadCard RotationalForceLoadTable::card(std::uint32_t row) const
{
    RotationalForceLoadCard card;
    card.id = m_id.get(row);
    card.node = m_node.get(row);
    card.frameId = m_frameId.get(row);
    card.magnitude = m_magnitude.get(row);
    card.axis = m_axis.get(row);
    card.methodName = m_methodName.get(row);
    card.angularAcceleration = m_angularAcceleration.get(row);
    card.mainBulkFlag = m_mainBulkFlag.get(row);
    card.regionId = m_regionId.get(row);
    return card;
}

void RotationalForceLoadTable::store(std::uint32_t row, const RotationalForceLoadCard& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_frameId.set(row, card.frameId);
    m_magnitude.set(row, card.magnitude);
    m_axis.set(row, card.axis);
    m_methodName.set(row, card.methodName);
    m_angularAcceleration.set(row, card.angularAcceleration);
    m_mainBulkFlag.set(row, card.mainBulkFlag);
    m_regionId.set(row, card.regionId);
}

void RotationalForceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_frameId.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_axis.keepRows(rows);
    m_methodName.keepRows(rows);
    m_angularAcceleration.keepRows(rows);
    m_mainBulkFlag.keepRows(rows);
    m_regionId.keepRows(rows);
}

void RotationalForceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId node = m_node.get(row); node > 0)
        m_node.set(row, visitor.visit("node", ReferenceTarget::Node, node));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
