#include "oor/model/cards/constraints/cyclicBoundaryJoin/CyclicBoundaryJoinTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 3;

} // namespace

std::optional<CyclicBoundaryJoinCard> CyclicBoundaryJoinFormat::read(FieldReader& reader)
{
    CyclicBoundaryJoinCard card;
    card.sideCode = reader.requiredInteger(1, "sideCode");
    card.system = readOptionalKeyword<CyclicBoundaryJoinSystem>(reader, 2, "system", CYCLIC_BOUNDARY_SYSTEM_WORDS);
    card.grids = readIdRuns(reader, FIRST_GRID_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicBoundaryJoinFormat::check(const CyclicBoundaryJoinCard& card)
{
    if (card.sideCode != 1 && card.sideCode != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "sideCode"};
    return checkIdRuns(card.grids, "nodes");
}

void CyclicBoundaryJoinFormat::write(const CyclicBoundaryJoinCard& card, FieldWriter& writer)
{
    writer.integer(1, card.sideCode);
    writeOptionalKeyword(writer, 2, CYCLIC_BOUNDARY_SYSTEM_WORDS, card.system);
    writeIdRuns(writer, FIRST_GRID_FIELD, card.grids, writer.compressesRanges());
}

std::uint32_t CyclicBoundaryJoinTable::append(const CyclicBoundaryJoinCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_sideCode.push(card.sideCode);
    m_cards.push(card);
    return row;
}

void CyclicBoundaryJoinTable::store(std::uint32_t row, const CyclicBoundaryJoinCard& card)
{
    m_sideCode.set(row, card.sideCode);
    m_cards.set(row, card);
}

void CyclicBoundaryJoinTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_sideCode.keepRows(rows);
    m_cards.keepRows(rows);
}

void CyclicBoundaryJoinTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // Every point the card names must be defined: a grid point or a scalar point is a node.
    visitRunList(visitor, "nodes", m_cards.edit(row).grids, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
