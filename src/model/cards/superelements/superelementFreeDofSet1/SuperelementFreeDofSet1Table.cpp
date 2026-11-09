#include "oor/model/cards/superelements/superelementFreeDofSet1/SuperelementFreeDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<SuperelementFreeDofSet1Card> SuperelementFreeDofSet1Format::read(FieldReader& reader)
{
    SuperelementFreeDofSet1Card card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    if ((reader.text(2) == "All"))
    {
        card.all = true;
        if (reader.lastUsedField() > 2)
            reader.error("card.bad-value", "ALL is alone on its card after the " + std::string("superelementId"),
                         "components");
    }
    else
    {
        card.components = base::sortedDigits(reader.integer(2, "components", 0));
        card.points = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementFreeDofSet1Format::check(const SuperelementFreeDofSet1Card& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (card.all)
    {
        if (card.components != 0 || !card.points.empty())
            return CardProblem{"card.bad-value", "the card with ALL has no components and no points", "components"};
        return std::nullopt;
    }
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.points, "nodes");
}

void SuperelementFreeDofSet1Format::write(const SuperelementFreeDofSet1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    if (card.all)
    {
        writer.text(2, "All");
        return;
    }
    writer.integerUnlessDefault(2, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.points, writer.compressesRanges());
}

std::uint32_t SuperelementFreeDofSet1Table::append(const SuperelementFreeDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementFreeDofSet1Table::store(std::uint32_t row, const SuperelementFreeDofSet1Card& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementFreeDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementFreeDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementFreeDofSet1Card& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    visitRunList(scope, "nodes", card.points, ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
