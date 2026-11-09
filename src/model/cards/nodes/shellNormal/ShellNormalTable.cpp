#include "oor/model/cards/nodes/shellNormal/ShellNormalTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5; // N3

} // namespace

std::optional<ShellNormalCard> ShellNormalFormat::read(FieldReader& reader)
{
    ShellNormalCard card;
    card.nodeId = reader.requiredInteger(1, "nodeId");
    card.frameId = reader.integer(2, "frameId", 0);
    card.normal.x = reader.real(3, "directionX", 0.0);
    card.normal.y = reader.real(4, "directionY", 0.0);
    card.normal.z = reader.real(5, "directionZ", 0.0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellNormalFormat::check(const ShellNormalCard& card)
{
    if (const auto problem = checkRequiredId(card.nodeId, "nodeId"))
        return problem;
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", "{field} must be 0 or a coordinate system ID", "frameId"};
    if (const auto problem = checkReal(card.normal.x, "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.normal.y, "directionY", RealRule::Finite))
        return problem;
    return checkReal(card.normal.z, "directionZ", RealRule::Finite);
}

void ShellNormalFormat::write(const ShellNormalCard& card, FieldWriter& writer)
{
    writer.integer(1, card.nodeId);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.realUnlessDefault(3, card.normal.x, 0.0);
    writer.realUnlessDefault(4, card.normal.y, 0.0);
    writer.realUnlessDefault(5, card.normal.z, 0.0);
}

std::uint32_t ShellNormalTable::append(const ShellNormalCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_nodeId.push(card.nodeId);
    m_frameId.push(card.frameId);
    m_normal.push({card.normal.x, card.normal.y, card.normal.z});
    return row;
}

ShellNormalCard ShellNormalTable::card(std::uint32_t row) const
{
    const std::array<double, 3> normal = m_normal.get(row);
    return ShellNormalCard{m_nodeId.get(row), m_frameId.get(row), base::Vec3{normal[0], normal[1], normal[2]}};
}

void ShellNormalTable::store(std::uint32_t row, const ShellNormalCard& card)
{
    m_nodeId.set(row, card.nodeId);
    m_frameId.set(row, card.frameId);
    m_normal.set(row, {card.normal.x, card.normal.y, card.normal.z});
}

void ShellNormalTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_nodeId.keepRows(rows);
    m_frameId.keepRows(rows);
    m_normal.keepRows(rows);
}

void ShellNormalTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId node = m_nodeId.get(row); node > 0)
        m_nodeId.set(row, visitor.visit("nodeId", ReferenceTarget::Node, node));
    if (const base::EntityId frame = m_frameId.get(row); frame > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frame));
}

} // namespace oor::model
