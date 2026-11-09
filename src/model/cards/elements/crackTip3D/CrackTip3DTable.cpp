#include "oor/model/cards/elements/crackTip3D/CrackTip3DTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIRST_NODE_FIELD = 3; // G1 is field 3: the fields of the format reference are EID, PID, then G1 to G64
constexpr int LAST_FIELD = 66;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

constexpr const char* NODE_NAMES[CrackTip3DCard::NODE_COUNT] = {
    "node1",  "node2",  "node3",  "node4",  "node5",  "node6",  "node7",  "node8",  "node9",  "node10", "node11",
    "node12", "node13", "node14", "node15", "node16", "node17", "node18", "node19", "node20", "node21", "node22",
    "node23", "node24", "node25", "node26", "node27", "node28", "node29", "node30", "node31", "node32", "node33",
    "node34", "node35", "node36", "node37", "node38", "node39", "node40", "node41", "node42", "node43", "node44",
    "node45", "node46", "node47", "node48", "node49", "node50", "node51", "node52", "node53", "node54", "node55",
    "node56", "node57", "node58", "node59", "node60", "node61", "node62", "node63", "node64"};

// A run of slots of `nodes` as zero-based positions: from `first` up to, and not including, `end`.
using SlotRun = std::pair<std::size_t, std::size_t>;

// The full crack: the points that are required (G1 to G10 and G19 to G28).
constexpr SlotRun FULL_REQUIRED[] = {{0, 10}, {18, 28}};
// The symmetric half crack: the points that are required (G1 to G7 and G19 to G25).
constexpr SlotRun HALF_REQUIRED[] = {{0, 7}, {18, 25}};
// The points that must not be given in the half crack, besides G8 to G10 which are what makes it one.
constexpr SlotRun HALF_ABSENT[] = {{14, 18}, {25, 28}, {32, 36}, {42, 46}, {50, 55}, {59, 64}};
// G37 to G46 are all given or all blank in the full crack.
constexpr SlotRun MIDPLANE = {36, 46};

// The first blank slot of the run, or `end` when none is.
std::size_t firstBlank(const CrackTip3DCard& card, const SlotRun& run)
{
    for (std::size_t index = run.first; index < run.second; ++index)
    {
        if (card.nodes[index] == 0)
            return index;
    }
    return run.second;
}

// The first slot of the run that holds a point, or `end` when none does.
std::size_t firstUsed(const CrackTip3DCard& card, const SlotRun& run)
{
    for (std::size_t index = run.first; index < run.second; ++index)
    {
        if (card.nodes[index] != 0)
            return index;
    }
    return run.second;
}

} // namespace

std::optional<CrackTip3DCard> CrackTip3DFormat::read(FieldReader& reader)
{
    CrackTip3DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    // Every slot is read as optional: which of them are required depends on the form (full or half crack), which only
    // the whole card tells, so check() applies the rule.
    for (std::size_t index = 0; index < CrackTip3DCard::NODE_COUNT; ++index)
        card.nodes[index] = reader.integer(FIRST_NODE_FIELD + static_cast<int>(index), NODE_NAMES[index], 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrackTip3DFormat::check(const CrackTip3DCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t index = 0; index < CrackTip3DCard::NODE_COUNT; ++index)
    {
        if (card.nodes[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", NODE_NAMES[index]};
    }
    if (card.isHalfCrack())
    {
        for (const SlotRun& run : HALF_REQUIRED)
        {
            if (const std::size_t index = firstBlank(card, run); index < run.second)
                return CardProblem{"card.missing-field",
                                   "the grid points {field:node1} to {field:node7} and {field:node19} to "
                                   "{field:node25} are required (symmetric half-crack option)",
                                   NODE_NAMES[index]};
        }
        for (const SlotRun& run : HALF_ABSENT)
        {
            if (const std::size_t index = firstUsed(card, run); index < run.second)
                return CardProblem{"card.bad-value",
                                   "this grid point must be blank when {field:node8} to {field:node10} are blank "
                                   "(symmetric half-crack option)",
                                   NODE_NAMES[index]};
        }
        return std::nullopt;
    }
    for (const SlotRun& run : FULL_REQUIRED)
    {
        if (const std::size_t index = firstBlank(card, run); index < run.second)
            return CardProblem{"card.missing-field",
                               "the grid points {field:node1} to {field:node10} and {field:node19} to {field:node28} "
                               "are required ({field:node8} to {field:node10} may be all blank for "
                               "the symmetric half-crack option, which then needs only {field:node1} to {field:node7} "
                               "and {field:node19} to {field:node25})",
                               NODE_NAMES[index]};
    }
    // All or none of G37 to G46, else the connectivity is partial and the solver stops.
    if (firstUsed(card, MIDPLANE) < MIDPLANE.second)
    {
        if (const std::size_t index = firstBlank(card, MIDPLANE); index < MIDPLANE.second)
            return CardProblem{"card.missing-field",
                               "{field:node37} to {field:node46} must be all given or all blank (partial connectivity)",
                               NODE_NAMES[index]};
    }
    return std::nullopt;
}

void CrackTip3DFormat::write(const CrackTip3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < CrackTip3DCard::NODE_COUNT; ++index)
    {
        // A blank slot is a blank field: most of the points are optional, and the half crack leaves many out.
        if (card.nodes[index] != 0)
            writer.integer(FIRST_NODE_FIELD + static_cast<int>(index), card.nodes[index]);
    }
}

std::uint32_t CrackTip3DTable::append(const CrackTip3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

CrackTip3DCard CrackTip3DTable::card(std::uint32_t row) const
{
    CrackTip3DCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void CrackTip3DTable::store(std::uint32_t row, const CrackTip3DCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void CrackTip3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void CrackTip3DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId)); // a PRAC3D
    std::array<base::EntityId, CrackTip3DCard::NODE_COUNT> nodes = m_nodes.get(row);
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
