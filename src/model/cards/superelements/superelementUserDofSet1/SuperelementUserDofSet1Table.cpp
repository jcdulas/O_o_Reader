#include "oor/model/cards/superelements/superelementUserDofSet1/SuperelementUserDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 4;

} // namespace

std::optional<SuperelementUserDofSet1Card> SuperelementUserDofSet1Format::read(FieldReader& reader)
{
    SuperelementUserDofSet1Card card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.setName = std::string(reader.text(2));
    if (card.setName.empty())
        reader.error("card.missing-field", "an " + std::string(reader.cardName()) + " card needs a set name",
                     "setName");
    card.components = base::sortedDigits(reader.integer(3, "components", 0));
    card.points = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementUserDofSet1Format::check(const SuperelementUserDofSet1Card& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (card.setName.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be blank", "setName"};
    if (card.setName.find_first_of(" ,$") != std::string::npos)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not hold blanks, commas or a '$'",
                           "setName"};
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.points, "nodes");
}

void SuperelementUserDofSet1Format::write(const SuperelementUserDofSet1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.text(2, card.setName);
    writer.integerUnlessDefault(3, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.points, writer.compressesRanges());
}

std::uint32_t SuperelementUserDofSet1Table::append(const SuperelementUserDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_setName.push(card.setName);
    m_cards.push(card);
    return row;
}

void SuperelementUserDofSet1Table::store(std::uint32_t row, const SuperelementUserDofSet1Card& card)
{
    m_superelementId.set(row, card.superelementId);
    m_setName.set(row, card.setName);
    m_cards.set(row, card);
}

void SuperelementUserDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_setName.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementUserDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementUserDofSet1Card& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    visitRunList(scope, "nodes", card.points, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
