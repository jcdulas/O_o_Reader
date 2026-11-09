#include "oor/model/cards/optimisation/designNodeMove/DesignNodeMoveTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 7;

} // namespace

std::optional<DesignNodeMoveCard> DesignNodeMoveFormat::read(FieldReader& reader)
{
    DesignNodeMoveCard card;
    card.designVarId = reader.requiredInteger(1, "designVarId");
    card.nodeId = reader.requiredInteger(2, "nodeId");
    card.frameId = reader.integer(3, "frameId", 0);
    card.coefficient = reader.real(4, "coefficient", 0.0);
    card.direction[0] = reader.real(5, "directionX", 0.0);
    card.direction[1] = reader.real(6, "directionY", 0.0);
    card.direction[2] = reader.real(7, "directionZ", 0.0);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignNodeMoveFormat::check(const DesignNodeMoveCard& card)
{
    if (const auto problem = checkRequiredId(card.designVarId, "designVarId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeId, "nodeId"))
        return problem;
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
    if (const auto problem = checkReal(card.coefficient, "coefficient", RealRule::Finite))
        return problem;
    static constexpr const char* NAMES[3] = {"directionX", "directionY", "directionZ"};
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (const auto problem = checkReal(card.direction[index], NAMES[index], RealRule::Finite))
            return problem;
    }
    if (card.direction[0] == 0.0 && card.direction[1] == 0.0 && card.direction[2] == 0.0)
        return CardProblem{"card.bad-value",
                           "at least one of " + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("directionY") + " and " + CardProblem::fieldNamed("directionZ") +
                               " must not be zero",
                           "directionX"};
    return std::nullopt;
}

void DesignNodeMoveFormat::write(const DesignNodeMoveCard& card, FieldWriter& writer)
{
    writer.integer(1, card.designVarId);
    writer.integer(2, card.nodeId);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.realUnlessDefault(4, card.coefficient, 0.0);
    writer.realUnlessDefault(5, card.direction[0], 0.0);
    writer.realUnlessDefault(6, card.direction[1], 0.0);
    writer.realUnlessDefault(7, card.direction[2], 0.0);
}

std::uint32_t DesignNodeMoveTable::append(const DesignNodeMoveCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_designVarId.push(card.designVarId);
    m_nodeId.push(card.nodeId);
    m_frameId.push(card.frameId);
    m_coefficient.push(card.coefficient);
    m_direction.push(card.direction);
    return row;
}

DesignNodeMoveCard DesignNodeMoveTable::card(std::uint32_t row) const
{
    return DesignNodeMoveCard{m_designVarId.get(row), m_nodeId.get(row), m_frameId.get(row), m_coefficient.get(row),
                              m_direction.get(row)};
}

void DesignNodeMoveTable::store(std::uint32_t row, const DesignNodeMoveCard& card)
{
    m_designVarId.set(row, card.designVarId);
    m_nodeId.set(row, card.nodeId);
    m_frameId.set(row, card.frameId);
    m_coefficient.set(row, card.coefficient);
    m_direction.set(row, card.direction);
}

void DesignNodeMoveTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_designVarId.keepRows(rows);
    m_nodeId.keepRows(rows);
    m_frameId.keepRows(rows);
    m_coefficient.keepRows(rows);
    m_direction.keepRows(rows);
}

void DesignNodeMoveTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId designVarId = m_designVarId.get(row);
    visitId(visitor, "designVarId", ReferenceTarget::DesignVariable, designVarId);
    m_designVarId.set(row, designVarId);
    base::EntityId nodeId = m_nodeId.get(row);
    visitId(visitor, "nodeId", ReferenceTarget::Node, nodeId);
    m_nodeId.set(row, nodeId);
    base::EntityId frameId = m_frameId.get(row);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, frameId);
    m_frameId.set(row, frameId);
}

} // namespace oor::model
