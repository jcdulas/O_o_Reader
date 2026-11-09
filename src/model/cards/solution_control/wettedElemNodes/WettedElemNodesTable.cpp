#include "oor/model/cards/solution_control/wettedElemNodes/WettedElemNodesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 9; // G1, the first field of the continuation line
constexpr int LAST_GRID_FIELD = 16; // G8: the last field of the card

// The names of the grid points in the diagnostics and for the references.
constexpr std::array<const char*, 8> GRID_NAMES{"nodes", "node2", "node3", "node4", "node5", "node6", "node7", "node8"};
constexpr std::array<const char*, 8> GRID_REFERENCE_NAMES{"g1", "g2", "g3", "g4", "g5", "g6", "g7", "g8"};

std::string gridCountMessage(WettedElementType type)
{
    return "a " + std::string(keywordText(WETTED_ELEMENT_WORDS, type)) + " has " + std::to_string(gridCountOf(type)) +
           " grid points";
}

} // namespace

std::optional<WettedElemNodesCard> WettedElemNodesFormat::read(FieldReader& reader)
{
    WettedElemNodesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readRequiredKeyword<WettedElementType>(reader, 2, "kind", WETTED_ELEMENT_WORDS);
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, LAST_GRID_FIELD);
    if (reader.failed())
        return card;
    // The grid points of the shape follow with no gap and nothing after them.
    const int count = static_cast<int>(gridCountOf(card.type));
    for (int index = 0; index < count && !reader.failed(); ++index)
        card.grids.push_back(
            reader.requiredInteger(FIRST_GRID_FIELD + index, GRID_NAMES[static_cast<std::size_t>(index)]));
    if (!reader.failed() && reader.lastUsedField() >= FIRST_GRID_FIELD + count)
        reader.error("card.bad-value", gridCountMessage(card.type), "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WettedElemNodesFormat::check(const WettedElemNodesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.grids.size() != gridCountOf(card.type))
        return CardProblem{"card.bad-value", gridCountMessage(card.type), "nodes"};
    for (std::size_t index = 0; index < card.grids.size(); ++index)
    {
        if (const auto problem = checkRequiredId(card.grids[index], GRID_NAMES[index]))
            return problem;
    }
    return std::nullopt;
}

void WettedElemNodesFormat::write(const WettedElemNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, WETTED_ELEMENT_WORDS, card.type);
    int field = FIRST_GRID_FIELD;
    for (const base::EntityId grid : card.grids)
        writer.integer(field++, grid);
}

std::uint32_t WettedElemNodesTable::append(const WettedElemNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void WettedElemNodesTable::store(std::uint32_t row, const WettedElemNodesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void WettedElemNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void WettedElemNodesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void WettedElemNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The WEID is the card's own number; only the grid points are references.
    std::vector<base::EntityId>& grids = m_cards.edit(row).grids;
    for (std::size_t index = 0; index < grids.size() && index < GRID_REFERENCE_NAMES.size(); ++index)
        visitId(visitor, GRID_REFERENCE_NAMES[index], ReferenceTarget::Node, grids[index]);
}

} // namespace oor::model
