#include "oor/model/cards/solution_control/residualVectorDofs1/ResidualVectorDofs1Table.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<ResidualVectorDofs1Card> ResidualVectorDofs1Format::read(FieldReader& reader)
{
    ResidualVectorDofs1Card card;
    card.components = base::sortedDigits(reader.integer(1, "components", 0));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ResidualVectorDofs1Format::check(const ResidualVectorDofs1Card& card)
{
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void ResidualVectorDofs1Format::write(const ResidualVectorDofs1Card& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t ResidualVectorDofs1Table::append(const ResidualVectorDofs1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

ResidualVectorDofs1Card ResidualVectorDofs1Table::card(std::uint32_t row) const
{
    ResidualVectorDofs1Card card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void ResidualVectorDofs1Table::store(std::uint32_t row, const ResidualVectorDofs1Card& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void ResidualVectorDofs1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void ResidualVectorDofs1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs (a renumbering of a model that
    // has THRU ranges must renumber the points in order).
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
