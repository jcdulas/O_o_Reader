#include "oor/model/cards/elements/crackTip2D/CrackTip2DTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_NODE_FIELD = 3; // G1 is field 3: the fields of the format reference are EID, PID, then G1 to G18
constexpr int LAST_FIELD = 20;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

// G1 to G7 are required in both forms; G8 to G10 in the full crack only.
constexpr std::size_t ALWAYS_REQUIRED = 7;
constexpr std::size_t FIRST_FULL_ONLY = 7;
constexpr std::size_t END_FULL_ONLY = 10;
// G15 to G18 do not exist in the symmetric half crack.
constexpr std::size_t FIRST_ABSENT_IN_HALF = 14;

constexpr const char* NODE_NAMES[CrackTip2DCard::NODE_COUNT] = {
    "node1",  "node2",  "node3",  "node4",  "node5",  "node6",  "node7",  "node8",  "node9",
    "node10", "node11", "node12", "node13", "node14", "node15", "node16", "node17", "node18"};

} // namespace

std::optional<CrackTip2DCard> CrackTip2DFormat::read(FieldReader& reader)
{
    CrackTip2DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    // Every slot is read as optional: which of them are required depends on the form (full or half crack), which only
    // the whole card tells, so check() applies the rule.
    for (std::size_t index = 0; index < CrackTip2DCard::NODE_COUNT; ++index)
        card.nodes[index] = reader.integer(FIRST_NODE_FIELD + static_cast<int>(index), NODE_NAMES[index], 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrackTip2DFormat::check(const CrackTip2DCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t index = 0; index < CrackTip2DCard::NODE_COUNT; ++index)
    {
        if (card.nodes[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", NODE_NAMES[index]};
    }
    for (std::size_t index = 0; index < ALWAYS_REQUIRED; ++index)
    {
        if (card.nodes[index] == 0)
            return CardProblem{"card.missing-field", "the grid points {field:node1} to {field:node7} are required",
                               NODE_NAMES[index]};
    }
    if (card.isHalfCrack())
    {
        // The symmetric half crack has no G8 to G10 and no G15 to G18.
        for (std::size_t index = FIRST_ABSENT_IN_HALF; index < CrackTip2DCard::NODE_COUNT; ++index)
        {
            if (card.nodes[index] != 0)
                return CardProblem{"card.bad-value",
                                   "{field:node15} to {field:node18} must be blank when {field:node8} to "
                                   "{field:node10} are blank (the symmetric half-crack option)",
                                   NODE_NAMES[index]};
        }
        return std::nullopt;
    }
    // The full crack has G1 to G10; a card that gives some of G8 to G10 and not the others is neither form.
    for (std::size_t index = FIRST_FULL_ONLY; index < END_FULL_ONLY; ++index)
    {
        if (card.nodes[index] == 0)
            return CardProblem{
                "card.missing-field",
                "the grid points {field:node8} to {field:node10} are required, or all three blank for the symmetric "
                "half-crack option",
                NODE_NAMES[index]};
    }
    return std::nullopt;
}

void CrackTip2DFormat::write(const CrackTip2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < CrackTip2DCard::NODE_COUNT; ++index)
    {
        // A blank slot is a blank field: the half crack has no G8 to G10, and G11 to G18 are optional.
        if (card.nodes[index] != 0)
            writer.integer(FIRST_NODE_FIELD + static_cast<int>(index), card.nodes[index]);
    }
}

std::uint32_t CrackTip2DTable::append(const CrackTip2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

CrackTip2DCard CrackTip2DTable::card(std::uint32_t row) const
{
    CrackTip2DCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void CrackTip2DTable::store(std::uint32_t row, const CrackTip2DCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void CrackTip2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void CrackTip2DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId)); // a PRAC2D
    std::array<base::EntityId, CrackTip2DCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue; // a blank slot
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
