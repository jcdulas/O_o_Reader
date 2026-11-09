#include "oor/model/cards/sets/generalizedDofSet1/GeneralizedDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<GeneralizedDofSet1Card> GeneralizedDofSet1Format::read(FieldReader& reader)
{
    GeneralizedDofSet1Card card;
    card.components = base::sortedDigits(reader.integer(1, "components", 0));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeneralizedDofSet1Format::check(const GeneralizedDofSet1Card& card)
{
    if (!base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be 0 or a list of the digits 1 to 6, each at most once",
                           "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void GeneralizedDofSet1Format::write(const GeneralizedDofSet1Card& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t GeneralizedDofSet1Table::append(const GeneralizedDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

GeneralizedDofSet1Card GeneralizedDofSet1Table::card(std::uint32_t row) const
{
    GeneralizedDofSet1Card card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void GeneralizedDofSet1Table::store(std::uint32_t row, const GeneralizedDofSet1Card& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void GeneralizedDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void GeneralizedDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs (a renumbering of a model that
    // has THRU ranges must renumber the points in order).
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
