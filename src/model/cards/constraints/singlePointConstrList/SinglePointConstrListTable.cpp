#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

// The digits of a component list must be 0 alone, or 1 to 6 each at most once.
bool validComponents(std::int32_t components)
{
    if (components == 0)
        return true;
    if (components < 0)
        return false;
    unsigned seen = 0;
    for (std::int32_t rest = components; rest > 0; rest /= 10)
    {
        const unsigned digit = static_cast<unsigned>(rest % 10);
        if (digit < 1 || digit > 6 || (seen & (1U << digit)) != 0)
            return false;
        seen |= 1U << digit;
    }
    return true;
}

std::optional<CardProblem> checkHeader(const SinglePointConstrListCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (!validComponents(card.components))
        return CardProblem{"card.bad-value", "C must be 0 or a list of the digits 1 to 6, each at most once",
                           "components"};
    return std::nullopt;
}

std::vector<std::array<base::EntityId, 2>> toPairs(const std::vector<NodeRun>& runs)
{
    std::vector<std::array<base::EntityId, 2>> pairs;
    pairs.reserve(runs.size());
    for (const NodeRun& run : runs)
        pairs.push_back({run.first, run.last});
    return pairs;
}

} // namespace

std::optional<SinglePointConstrListCard> SinglePointConstrListFormat::read(FieldReader& reader)
{
    SinglePointConstrListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.components = base::sortedDigits(reader.requiredInteger(2, "components"));
    if (!reader.failed())
        reader.report(checkHeader(card));
    // Grid points from field 3 on. "Through" between two points makes a run. Blank fields (the padding of the
    // continuation lines) are skipped.
    const int last = reader.lastUsedField();
    for (int field = 3; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            if (card.nodes.empty() || card.nodes.back().first != card.nodes.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single grid point", "through");
                break;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last grid point", "through");
                break;
            }
            const base::EntityId end = reader.requiredInteger(next, "node2");
            if (!reader.failed() && end < card.nodes.back().first)
                reader.error("card.bad-value", "the end of a THRU range must not be below its start", "node2");
            card.nodes.back().last = end;
            field = next;
            continue;
        }
        const base::EntityId node = reader.requiredInteger(field, "node");
        card.nodes.push_back(NodeRun{node, node});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SinglePointConstrListFormat::check(const SinglePointConstrListCard& card)
{
    if (auto problem = checkHeader(card))
        return problem;
    if (card.nodes.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "node1"};
    for (const NodeRun& run : card.nodes)
    {
        if (run.last < run.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "node2"};
    }
    return std::nullopt;
}

void SinglePointConstrListFormat::write(const SinglePointConstrListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.components);
    // With compression, grid points that follow each other are one run: 1, 2, 3, 4 is "1 THRU 4". Two points stay two
    // points (a THRU would not be shorter).
    std::vector<NodeRun> runs;
    for (const NodeRun& run : card.nodes)
    {
        if (writer.compressesRanges() && !runs.empty() && run.first == runs.back().last + 1)
            runs.back().last = run.last;
        else
            runs.push_back(run);
    }
    int field = 3;
    for (const NodeRun& run : runs)
    {
        const bool asRange = run.last != run.first && (!writer.compressesRanges() || run.last - run.first >= 2);
        if (!asRange && run.last != run.first)
        {
            writer.integer(field++, run.first);
            writer.integer(field++, run.last);
            continue;
        }
        writer.integer(field++, run.first);
        if (asRange)
        {
            writer.text(field++, "Through");
            writer.integer(field++, run.last);
        }
    }
}

std::uint32_t SinglePointConstrListTable::append(const SinglePointConstrListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

SinglePointConstrListCard SinglePointConstrListTable::card(std::uint32_t row) const
{
    SinglePointConstrListCard card;
    card.id = m_id.get(row);
    card.components = m_components.get(row);
    for (const auto& pair : m_runs.get(row))
        card.nodes.push_back(NodeRun{pair[0], pair[1]});
    return card;
}

void SinglePointConstrListTable::store(std::uint32_t row, const SinglePointConstrListCard& card)
{
    m_id.set(row, card.id);
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void SinglePointConstrListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void SinglePointConstrListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are visited; the points between them keep their IDs (a renumbering of a model that
    // has THRU ranges must renumber the points in order).
    std::vector<std::array<base::EntityId, 2>> pairs(m_runs.get(row).begin(), m_runs.get(row).end());
    bool changed = false;
    for (auto& pair : pairs)
    {
        const bool isRange = pair[0] != pair[1];
        const ReferenceTarget target = isRange ? ReferenceTarget::NodeInRange : ReferenceTarget::Node;
        for (base::EntityId& id : pair)
        {
            const base::EntityId updated = visitor.visit("node", target, id);
            changed = changed || updated != id;
            id = updated;
        }
    }
    if (changed)
        m_runs.set(row, pairs);
}

} // namespace oor::model
