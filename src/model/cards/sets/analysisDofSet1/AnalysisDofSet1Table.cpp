#include "oor/model/cards/sets/analysisDofSet1/AnalysisDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<AnalysisDofSet1Card> AnalysisDofSet1Format::read(FieldReader& reader)
{
    AnalysisDofSet1Card card;
    card.components = base::sortedDigits(reader.integer(1, "components", 0));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnalysisDofSet1Format::check(const AnalysisDofSet1Card& card)
{
    if (!base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be 0 or a list of the digits 1 to 6, each at most once",
                           "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void AnalysisDofSet1Format::write(const AnalysisDofSet1Card& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t AnalysisDofSet1Table::append(const AnalysisDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

AnalysisDofSet1Card AnalysisDofSet1Table::card(std::uint32_t row) const
{
    AnalysisDofSet1Card card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void AnalysisDofSet1Table::store(std::uint32_t row, const AnalysisDofSet1Card& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void AnalysisDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void AnalysisDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs (a renumbering of a model that
    // has THRU ranges must renumber the points in order).
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
