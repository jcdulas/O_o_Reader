#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

// PS is a list of the digits 1 to 6 with no repeat ("123456", "3", "246").
bool isValidPermanentConstraint(std::int32_t digits)
{
    if (digits == 0)
        return true;
    bool seen[7] = {};
    for (std::int32_t rest = digits; rest > 0; rest /= 10)
    {
        const std::int32_t digit = rest % 10;
        if (digit < 1 || digit > 6 || seen[digit])
            return false;
        seen[digit] = true;
    }
    return digits > 0;
}

} // namespace

std::optional<NodeDefaultsCard> NodeDefaultsFormat::read(FieldReader& reader)
{
    NodeDefaultsCard card;
    // Fields 1, 3, 4 and 5 are blank on this card; a value there would be taken for a default that is not one.
    for (const int field : {1, 3, 4, 5})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "this field of a " + std::string(reader.cardName()) + " must be blank", "");
    }
    card.positionFrameId = reader.integer(2, "positionFrameId", 0);
    card.displacementFrameId = reader.integer(6, "displacementFrameId", 0);
    card.permanentConstraints = base::sortedDigits(reader.integer(7, "permanentConstraints", 0));
    card.superelementId = reader.integer(8, "superelementId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NodeDefaultsFormat::check(const NodeDefaultsCard& card)
{
    if (card.positionFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "positionFrameId"};
    if (card.displacementFrameId < -1)
        return CardProblem{"card.bad-value", "{field} must be -1, 0 or a coordinate system ID", "displacementFrameId"};
    if (!isValidPermanentConstraint(card.permanentConstraints))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "permanentConstraints"};
    if (card.superelementId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a superelement ID", "superelementId"};
    // at least one of the four fields must be given.
    if (card.positionFrameId == 0 && card.displacementFrameId == 0 && card.permanentConstraints == 0 &&
        card.superelementId == 0)
        return CardProblem{"card.missing-field",
                           "at least one of {field}, " + CardProblem::fieldNamed("displacementFrameId") + ", " +
                               CardProblem::fieldNamed("permanentConstraints") + " and " +
                               CardProblem::fieldNamed("superelementId") + " must be given",
                           "positionFrameId"};
    return std::nullopt;
}

void NodeDefaultsFormat::write(const NodeDefaultsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(2, card.positionFrameId, 0);
    writer.integerUnlessDefault(6, card.displacementFrameId, 0);
    if (card.permanentConstraints != 0)
        writer.integer(7, card.permanentConstraints);
    writer.integerUnlessDefault(8, card.superelementId, 0);
}

std::uint32_t NodeDefaultsTable::append(const NodeDefaultsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_positionFrameId.push(card.positionFrameId);
    m_displacementFrameId.push(card.displacementFrameId);
    m_permanentConstraints.push(card.permanentConstraints);
    m_superelementId.push(card.superelementId);
    return row;
}

NodeDefaultsCard NodeDefaultsTable::card(std::uint32_t row) const
{
    return NodeDefaultsCard{m_positionFrameId.get(row), m_displacementFrameId.get(row), m_permanentConstraints.get(row),
                            m_superelementId.get(row)};
}

void NodeDefaultsTable::store(std::uint32_t row, const NodeDefaultsCard& card)
{
    m_positionFrameId.set(row, card.positionFrameId);
    m_displacementFrameId.set(row, card.displacementFrameId);
    m_permanentConstraints.set(row, card.permanentConstraints);
    m_superelementId.set(row, card.superelementId);
}

void NodeDefaultsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_positionFrameId.keepRows(rows);
    m_displacementFrameId.keepRows(rows);
    m_permanentConstraints.keepRows(rows);
    m_superelementId.keepRows(rows);
}

void NodeDefaultsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId positionFrame = m_positionFrameId.get(row); positionFrame > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, positionFrame));
    if (const base::EntityId displacementFrame = m_displacementFrameId.get(row); displacementFrame > 0)
        m_displacementFrameId.set(row,
                                  visitor.visit("displacementFrameId", ReferenceTarget::Coordinate, displacementFrame));
    if (const base::EntityId superelement = m_superelementId.get(row); superelement > 0)
        m_superelementId.set(row, visitor.visit("superelementId", ReferenceTarget::Superelement, superelement));
}

void NodeDefaultsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-node-defaults",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
