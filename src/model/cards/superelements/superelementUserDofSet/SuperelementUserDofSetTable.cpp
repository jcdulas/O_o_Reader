#include "oor/model/cards/superelements/superelementUserDofSet/SuperelementUserDofSetTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// SEID and SNAME, then three pairs on the first line (fields 3 to 8), then four pairs to a continuation line.
constexpr DofPairLayout LAYOUT{3, 3};

} // namespace

std::optional<SuperelementUserDofSetCard> SuperelementUserDofSetFormat::read(FieldReader& reader)
{
    SuperelementUserDofSetCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.setName = std::string(reader.text(2));
    if (card.setName.empty())
        reader.error("card.missing-field", "an " + std::string(reader.cardName()) + " card needs a set name",
                     "setName");
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementUserDofSetFormat::check(const SuperelementUserDofSetCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (card.setName.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be blank", "setName"};
    if (card.setName.find_first_of(" ,$") != std::string::npos)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not hold blanks, commas or a '$'",
                           "setName"};
    return checkDofPairs(card.dofs);
}

void SuperelementUserDofSetFormat::write(const SuperelementUserDofSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.text(2, card.setName);
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t SuperelementUserDofSetTable::append(const SuperelementUserDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_setName.push(card.setName);
    m_cards.push(card);
    return row;
}

void SuperelementUserDofSetTable::store(std::uint32_t row, const SuperelementUserDofSetCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_setName.set(row, card.setName);
    m_cards.set(row, card);
}

void SuperelementUserDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_setName.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementUserDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementUserDofSetCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    for (DofPair& dof : card.dofs)
        visitId(scope, "node", ReferenceTarget::Node, dof.node);
}

} // namespace oor::model
