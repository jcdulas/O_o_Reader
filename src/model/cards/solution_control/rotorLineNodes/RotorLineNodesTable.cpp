#include "oor/model/cards/solution_control/rotorLineNodes/RotorLineNodesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;

bool isWord(std::string_view text, std::string_view word)
{
    return (text == word);
}

// How many fields a run takes: one, or "a THRU b" (three), or "a THRU b BY c" (five).
int fieldsOf(const RotorLineNodesRun& run)
{
    if (run.first == run.last && run.increment == 1)
        return 1;
    return run.increment == 1 ? 3 : 5;
}

} // namespace

std::optional<RotorLineNodesCard> RotorLineNodesFormat::read(FieldReader& reader)
{
    RotorLineNodesCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    int field = 2;
    while (field <= last && !reader.failed())
    {
        if (reader.isBlank(field))
        {
            ++field; // the padding of a continuation line
            continue;
        }
        RotorLineNodesRun run;
        run.first = reader.requiredInteger(field, "nodeIds");
        run.last = run.first;
        if (isWord(reader.text(field + 1), "Through"))
        {
            run.last = reader.requiredInteger(field + 2, "nodeIds");
            field += 3;
            if (isWord(reader.text(field), "By"))
            {
                run.increment = reader.requiredInteger(field + 1, "increment");
                field += 2;
            }
        }
        else
        {
            ++field;
        }
        card.runs.push_back(run);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorLineNodesFormat::check(const RotorLineNodesCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the rotor ID must be a positive integer", "id"};
    if (card.runs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid", "nodeIds"};
    for (const RotorLineNodesRun& run : card.runs)
    {
        if (run.first <= 0 || run.last <= 0)
            return CardProblem{"card.bad-value", "the grids must be positive integers", "nodeIds"};
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "nodeIds"};
        if (run.increment < 1)
            return CardProblem{"card.bad-value", "the increment of a THRU range must be a positive integer",
                               "increment"};
        if (run.first == run.last && run.increment != 1)
            return CardProblem{"card.bad-value", "a single grid has no increment", "increment"};
    }
    return std::nullopt;
}

void RotorLineNodesFormat::write(const RotorLineNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = 2;
    for (const RotorLineNodesRun& run : card.runs)
    {
        // A run that does not fit in what is left of the line goes to the next one (its THRU is then in the second
        // field of the line, never in the first).
        const int width = fieldsOf(run);
        const int column = (field - 1) % FIELDS_PER_LINE; // 0 for the first field of a line
        if (width > 1 && column + width > FIELDS_PER_LINE)
            field += FIELDS_PER_LINE - column;
        writer.integer(field++, run.first);
        if (width > 1)
        {
            writer.text(field++, "Through");
            writer.integer(field++, run.last);
            if (width == 5)
            {
                writer.text(field++, "By");
                writer.integer(field++, run.increment);
            }
        }
    }
}

void RotorLineNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> first(m_first.get(row).begin(), m_first.get(row).end());
    std::vector<base::EntityId> last(m_last.get(row).begin(), m_last.get(row).end());
    bool changed = false;
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        const bool single = first[index] == last[index];
        const base::EntityId newFirst =
            visitor.visit("nodeIds", single ? ReferenceTarget::Node : ReferenceTarget::NodeInRange, first[index]);
        const base::EntityId newLast =
            single ? newFirst : visitor.visit("nodeIds", ReferenceTarget::NodeInRange, last[index]);
        changed = changed || newFirst != first[index] || newLast != last[index];
        first[index] = newFirst;
        last[index] = newLast;
    }
    if (changed)
    {
        m_first.set(row, first);
        m_last.set(row, last);
    }
}

std::uint32_t RotorLineNodesTable::append(const RotorLineNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> first, last, increment;
    for (const RotorLineNodesRun& run : card.runs)
    {
        first.push_back(run.first);
        last.push_back(run.last);
        increment.push_back(run.increment);
    }
    m_first.push(first);
    m_last.push(last);
    m_increment.push(increment);
    return row;
}

RotorLineNodesCard RotorLineNodesTable::card(std::uint32_t row) const
{
    RotorLineNodesCard card;
    card.id = m_id.get(row);
    const auto first = m_first.get(row);
    const auto last = m_last.get(row);
    const auto increment = m_increment.get(row);
    for (std::size_t index = 0; index < first.size(); ++index)
        card.runs.push_back(RotorLineNodesRun{first[index], last[index], increment[index]});
    return card;
}

void RotorLineNodesTable::store(std::uint32_t row, const RotorLineNodesCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> first, last, increment;
    for (const RotorLineNodesRun& run : card.runs)
    {
        first.push_back(run.first);
        last.push_back(run.last);
        increment.push_back(run.increment);
    }
    m_first.set(row, first);
    m_last.set(row, last);
    m_increment.set(row, increment);
}

void RotorLineNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_first.keepRows(rows);
    m_last.keepRows(rows);
    m_increment.keepRows(rows);
}

} // namespace oor::model
