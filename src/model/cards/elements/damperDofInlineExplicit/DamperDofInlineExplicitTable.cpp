#include "oor/model/cards/elements/damperDofInlineExplicit/DamperDofInlineExplicitTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_FRAME_ID = 9; // the first field of the continuation line
constexpr int FIELD_FOLLOW = 10;

} // namespace

std::optional<DamperDofInlineExplicitCard> DamperDofInlineExplicitFormat::read(FieldReader& reader)
{
    DamperDofInlineExplicitCard card;
    card.id = reader.requiredInteger(1, "id");
    card.damping = reader.requiredReal(2, "damping");
    card.node1 = reader.integer(3, "node1", 0); // blank: grounded
    card.components1 = base::sortedDigits(reader.integer(4, "components1", 0));
    card.node2 = reader.integer(5, "node2", 0);
    card.components2 = base::sortedDigits(reader.integer(6, "components2", 0));
    // The format reference leaves fields 7 and 8 empty: CORD and FOLLOW are the fields of the continuation line.
    requireBlankFields(reader, {7, 8}, FIELD_FOLLOW);
    card.frameId = reader.integer(FIELD_FRAME_ID, "frameId", 0);
    card.follow = readScalarFollow(reader, FIELD_FOLLOW);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DamperDofInlineExplicitFormat::check(const DamperDofInlineExplicitCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (const auto problem = checkReal(card.damping, "damping", RealRule::Finite))
        return problem;
    if (card.node1 < 0)
        return CardProblem{"card.bad-id", "the point ID {field} must not be negative", "node1"};
    if (!base::isComponentSet(card.components1))
        return CardProblem{"card.bad-value", "{field} must be 0 or a list of the digits 1 to 6, each at most once",
                           "components1"};
    if (card.node2 < 0)
        return CardProblem{"card.bad-id", "the point ID {field} must not be negative", "node2"};
    if (!base::isComponentSet(card.components2))
        return CardProblem{"card.bad-value", "{field} must be 0 or a list of the digits 1 to 6, each at most once",
                           "components2"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", "the coordinate system ID must not be negative", "frameId"};
    // An element cannot join a component of a point to itself.
    if (card.node1 == card.node2 && card.components1 == card.components2)
        return CardProblem{"card.duplicate-node",
                           "the two connection points ({field:node1}, {field:components1}) and ({field}, "
                           "{field:components2}) must be distinct",
                           "node2"};
    return std::nullopt;
}

void DamperDofInlineExplicitFormat::write(const DamperDofInlineExplicitCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.damping);
    writer.integerUnlessDefault(3, card.node1, 0);
    writer.integerUnlessDefault(4, card.components1, 0);
    writer.integerUnlessDefault(5, card.node2, 0);
    writer.integerUnlessDefault(6, card.components2, 0);
    writer.integerUnlessDefault(FIELD_FRAME_ID, card.frameId, 0);
    writeScalarFollow(writer, FIELD_FOLLOW, card.follow);
}

std::uint32_t DamperDofInlineExplicitTable::append(const DamperDofInlineExplicitCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_damping.push(card.damping);
    m_node1.push(card.node1);
    m_components1.push(card.components1);
    m_node2.push(card.node2);
    m_components2.push(card.components2);
    m_frameId.push(card.frameId);
    m_follow.push(static_cast<std::uint8_t>(card.follow));
    return row;
}

DamperDofInlineExplicitCard DamperDofInlineExplicitTable::card(std::uint32_t row) const
{
    return DamperDofInlineExplicitCard{
        m_id.get(row),    m_damping.get(row),     m_node1.get(row),   m_components1.get(row),
        m_node2.get(row), m_components2.get(row), m_frameId.get(row), static_cast<ScalarFollow>(m_follow.get(row))};
}

void DamperDofInlineExplicitTable::store(std::uint32_t row, const DamperDofInlineExplicitCard& card)
{
    m_id.set(row, card.id);
    m_damping.set(row, card.damping);
    m_node1.set(row, card.node1);
    m_components1.set(row, card.components1);
    m_node2.set(row, card.node2);
    m_components2.set(row, card.components2);
    m_frameId.set(row, card.frameId);
    m_follow.set(row, static_cast<std::uint8_t>(card.follow));
}

void DamperDofInlineExplicitTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_damping.keepRows(rows);
    m_node1.keepRows(rows);
    m_components1.keepRows(rows);
    m_node2.keepRows(rows);
    m_components2.keepRows(rows);
    m_frameId.keepRows(rows);
    m_follow.keepRows(rows);
}

void DamperDofInlineExplicitTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // G1 and G2 are grid points or scalar points (0: ground); a scalar point may be one no SPOINT defines.
    if (const base::EntityId node1 = m_node1.get(row); node1 > 0)
        m_node1.set(row, visitor.visit("node1", ReferenceTarget::ScalarOrGridPoint, node1));
    if (const base::EntityId node2 = m_node2.get(row); node2 > 0)
        m_node2.set(row, visitor.visit("node2", ReferenceTarget::ScalarOrGridPoint, node2));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
