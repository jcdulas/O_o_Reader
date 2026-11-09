#include "oor/model/cards/superelements/freeBoundaryDofs1/FreeBoundaryDofs1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<FreeBoundaryDofs1Card> FreeBoundaryDofs1Format::read(FieldReader& reader)
{
    FreeBoundaryDofs1Card card;
    if ((reader.text(1) == "All"))
    {
        card.all = true;
        if (reader.lastUsedField() > 1)
            reader.error("card.bad-value", "ALL is alone on its card", "components");
    }
    else
    {
        card.components = base::sortedDigits(reader.integer(1, "components", 0));
        card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeBoundaryDofs1Format::check(const FreeBoundaryDofs1Card& card)
{
    if (card.all)
    {
        if (card.components != 0 || !card.nodes.empty())
            return CardProblem{"card.bad-value", "the card with ALL has no components and no points", "components"};
        return std::nullopt;
    }
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void FreeBoundaryDofs1Format::write(const FreeBoundaryDofs1Card& card, FieldWriter& writer)
{
    if (card.all)
    {
        writer.text(1, "All");
        return;
    }
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t FreeBoundaryDofs1Table::append(const FreeBoundaryDofs1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    m_all.push(card.all ? 1 : 0);
    return row;
}

FreeBoundaryDofs1Card FreeBoundaryDofs1Table::card(std::uint32_t row) const
{
    FreeBoundaryDofs1Card card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    card.all = m_all.get(row) != 0;
    return card;
}

void FreeBoundaryDofs1Table::store(std::uint32_t row, const FreeBoundaryDofs1Card& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
    m_all.set(row, card.all ? 1 : 0);
}

void FreeBoundaryDofs1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
    m_all.keepRows(rows);
}

void FreeBoundaryDofs1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs.
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
