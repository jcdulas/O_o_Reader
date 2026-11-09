#include "oor/model/cards/nonlinear_contact/slidelineSegments/SlidelineSegmentsTable.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{true, true};

} // namespace

std::optional<SlidelineSegmentsCard> SlidelineSegmentsFormat::read(FieldReader& reader)
{
    SlidelineSegmentsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodes = readContactRuns(reader, 2, reader.lastUsedField(), "node", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SlidelineSegmentsFormat::check(const SlidelineSegmentsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.nodes.empty())
        return CardProblem{"card.missing-field", "the line needs at least one grid point", "node"};
    return checkContactRuns(card.nodes, "node", RULES);
}

void SlidelineSegmentsFormat::write(const SlidelineSegmentsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactRuns(writer, 2, card.nodes);
}

std::uint32_t SlidelineSegmentsTable::append(const SlidelineSegmentsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toContactTriples(card.nodes));
    return row;
}

SlidelineSegmentsCard SlidelineSegmentsTable::card(std::uint32_t row) const
{
    SlidelineSegmentsCard card;
    card.id = m_id.get(row);
    card.nodes = toContactRuns(m_runs.get(row));
    return card;
}

void SlidelineSegmentsTable::store(std::uint32_t row, const SlidelineSegmentsCard& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toContactTriples(card.nodes));
}

void SlidelineSegmentsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void SlidelineSegmentsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactTriples(m_runs, row, visitor, "node", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
