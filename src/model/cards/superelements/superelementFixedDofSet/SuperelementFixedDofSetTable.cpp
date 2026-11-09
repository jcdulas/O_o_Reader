#include "oor/model/cards/superelements/superelementFixedDofSet/SuperelementFixedDofSetTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// SEID, then three pairs on the first line (fields 2 to 7), then four pairs to a continuation line.
constexpr DofPairLayout LAYOUT{2, 3};

} // namespace

std::optional<SuperelementFixedDofSetCard> SuperelementFixedDofSetFormat::read(FieldReader& reader)
{
    SuperelementFixedDofSetCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementFixedDofSetFormat::check(const SuperelementFixedDofSetCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    return checkDofPairs(card.dofs);
}

void SuperelementFixedDofSetFormat::write(const SuperelementFixedDofSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t SuperelementFixedDofSetTable::append(const SuperelementFixedDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementFixedDofSetTable::store(std::uint32_t row, const SuperelementFixedDofSetCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementFixedDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementFixedDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementFixedDofSetCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    for (DofPair& dof : card.dofs)
        visitId(scope, "node", ReferenceTarget::Node, dof.node);
}

} // namespace oor::model
