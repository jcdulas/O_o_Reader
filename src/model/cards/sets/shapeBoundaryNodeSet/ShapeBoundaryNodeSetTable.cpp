#include "oor/model/cards/sets/shapeBoundaryNodeSet/ShapeBoundaryNodeSetTable.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<ShapeBoundaryNodeSetCard> ShapeBoundaryNodeSetFormat::read(FieldReader& reader)
{
    ShapeBoundaryNodeSetCard card;
    card.components = base::sortedDigits(reader.requiredInteger(1, "components"));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShapeBoundaryNodeSetFormat::check(const ShapeBoundaryNodeSetCard& card)
{
    if (card.components == 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be a list of the digits 1 to 6, each at most once", "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void ShapeBoundaryNodeSetFormat::write(const ShapeBoundaryNodeSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.components);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t ShapeBoundaryNodeSetTable::append(const ShapeBoundaryNodeSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

ShapeBoundaryNodeSetCard ShapeBoundaryNodeSetTable::card(std::uint32_t row) const
{
    ShapeBoundaryNodeSetCard card;
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void ShapeBoundaryNodeSetTable::store(std::uint32_t row, const ShapeBoundaryNodeSetCard& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void ShapeBoundaryNodeSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void ShapeBoundaryNodeSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs (a renumbering of a model that
    // has THRU ranges must renumber the points in order).
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
