#include "oor/model/cards/superelements/superelementFixedDofSet1/SuperelementFixedDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<SuperelementFixedDofSet1Card> SuperelementFixedDofSet1Format::read(FieldReader& reader)
{
    SuperelementFixedDofSet1Card card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.components = base::sortedDigits(reader.integer(2, "components", 0));
    card.points = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementFixedDofSet1Format::check(const SuperelementFixedDofSet1Card& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.points, "nodes");
}

void SuperelementFixedDofSet1Format::write(const SuperelementFixedDofSet1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.integerUnlessDefault(2, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.points, writer.compressesRanges());
}

std::uint32_t SuperelementFixedDofSet1Table::append(const SuperelementFixedDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementFixedDofSet1Table::store(std::uint32_t row, const SuperelementFixedDofSet1Card& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementFixedDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementFixedDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementFixedDofSet1Card& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    visitRunList(scope, "nodes", card.points, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
