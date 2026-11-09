#include "oor/model/cards/superelements/fixedBoundaryDofs1/FixedBoundaryDofs1Table.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<FixedBoundaryDofs1Card> FixedBoundaryDofs1Format::read(FieldReader& reader)
{
    FixedBoundaryDofs1Card card;
    card.components = base::sortedDigits(reader.integer(1, "components", 0));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FixedBoundaryDofs1Format::check(const FixedBoundaryDofs1Card& card)
{
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void FixedBoundaryDofs1Format::write(const FixedBoundaryDofs1Card& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t FixedBoundaryDofs1Table::append(const FixedBoundaryDofs1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

FixedBoundaryDofs1Card FixedBoundaryDofs1Table::card(std::uint32_t row) const
{
    FixedBoundaryDofs1Card card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void FixedBoundaryDofs1Table::store(std::uint32_t row, const FixedBoundaryDofs1Card& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void FixedBoundaryDofs1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void FixedBoundaryDofs1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs.
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
