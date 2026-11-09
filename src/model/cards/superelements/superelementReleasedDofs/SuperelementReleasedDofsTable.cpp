#include "oor/model/cards/superelements/superelementReleasedDofs/SuperelementReleasedDofsTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<SuperelementReleasedDofsCard> SuperelementReleasedDofsFormat::read(FieldReader& reader)
{
    SuperelementReleasedDofsCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.components = base::sortedDigits(reader.requiredInteger(2, "components"));
    if ((reader.text(FIRST_POINT_FIELD) == "All"))
    {
        card.all = true;
        if (reader.lastUsedField() > FIRST_POINT_FIELD)
            reader.error("card.bad-value", "ALL is alone on its card: no continuation is allowed", "node1");
    }
    else
    {
        card.points = readIdRuns(reader, FIRST_POINT_FIELD, "node1");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementReleasedDofsFormat::check(const SuperelementReleasedDofsCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "components"};
    if (card.all)
    {
        if (!card.points.empty())
            return CardProblem{"card.bad-value", "the card with ALL lists no points", "node1"};
        return std::nullopt;
    }
    return checkIdRuns(card.points, "node1");
}

void SuperelementReleasedDofsFormat::write(const SuperelementReleasedDofsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.integer(2, card.components);
    if (card.all)
        writer.text(FIRST_POINT_FIELD, "All");
    else
        writeIdRuns(writer, FIRST_POINT_FIELD, card.points, writer.compressesRanges());
}

std::uint32_t SuperelementReleasedDofsTable::append(const SuperelementReleasedDofsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementReleasedDofsTable::store(std::uint32_t row, const SuperelementReleasedDofsCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementReleasedDofsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementReleasedDofsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementReleasedDofsCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementId", card.superelementId);
    m_superelementId.set(row, card.superelementId);
    SuperelementScope scope(visitor, card.superelementId);
    visitRunList(scope, "node1", card.points, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
