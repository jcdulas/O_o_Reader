#include "oor/model/cards/nonlinear_contact/slidelineOutput/SlidelineOutputTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{true, true};

} // namespace

std::optional<SlidelineOutputCard> SlidelineOutputFormat::read(FieldReader& reader)
{
    SlidelineOutputCard card;
    card.id = reader.requiredInteger(1, "id");
    if ((reader.text(2) == "All"))
    {
        card.all = true;
        if (reader.lastUsedField() > 2)
            reader.error("card.bad-value", "no field may follow ALL on a " + std::string(reader.cardName()), "node");
    }
    else
    {
        card.nodes = readContactRuns(reader, 2, reader.lastUsedField(), "node", RULES);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SlidelineOutputFormat::check(const SlidelineOutputCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.all)
    {
        if (!card.nodes.empty())
            return CardProblem{"card.bad-value", "with ALL the card has no grid points", "node"};
        return std::nullopt;
    }
    if (card.nodes.empty())
        return CardProblem{"card.missing-field", "the card needs grid points, or ALL", "node"};
    return checkContactRuns(card.nodes, "node", RULES);
}

void SlidelineOutputFormat::write(const SlidelineOutputCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.all)
        writer.text(2, "All");
    else
        writeContactRuns(writer, 2, card.nodes);
}

std::uint32_t SlidelineOutputTable::append(const SlidelineOutputCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_all.push(card.all ? 1 : 0);
    m_runs.push(toContactTriples(card.nodes));
    return row;
}

SlidelineOutputCard SlidelineOutputTable::card(std::uint32_t row) const
{
    SlidelineOutputCard card;
    card.id = m_id.get(row);
    card.all = m_all.get(row) != 0;
    card.nodes = toContactRuns(m_runs.get(row));
    return card;
}

void SlidelineOutputTable::store(std::uint32_t row, const SlidelineOutputCard& card)
{
    m_id.set(row, card.id);
    m_all.set(row, card.all ? 1 : 0);
    m_runs.set(row, toContactTriples(card.nodes));
}

void SlidelineOutputTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_all.keepRows(rows);
    m_runs.keepRows(rows);
}

void SlidelineOutputTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactTriples(m_runs, row, visitor, "node", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
