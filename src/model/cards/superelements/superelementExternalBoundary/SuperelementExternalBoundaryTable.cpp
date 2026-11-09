#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// The pairs (GID, C) fill the fields from the first one, four to a line of eight fields.
constexpr int PAIRS_PER_LINE = 4;

int gidField(int slot) noexcept
{
    return 2 * slot + 1;
}

int componentField(int slot) noexcept
{
    return 2 * slot + 2;
}

} // namespace

std::optional<SuperelementExternalBoundaryCard> SuperelementExternalBoundaryFormat::read(FieldReader& reader)
{
    SuperelementExternalBoundaryCard card;
    const int last = reader.lastUsedField();
    for (int slot = 0; gidField(slot) <= last && !reader.failed(); ++slot)
    {
        if (reader.isBlank(gidField(slot)) && reader.isBlank(componentField(slot)))
            continue; // a pair of blank fields, left for editing
        SuperelementExternalBoundaryItem item;
        item.points.first = item.points.last = reader.requiredInteger(gidField(slot), "nodeId");
        if ((reader.text(componentField(slot)) == "Through"))
        {
            ++slot;
            if (gidField(slot) > last)
            {
                reader.error("card.missing-field", "THRU needs a last point", "nodeId");
                break;
            }
            item.points.last = reader.requiredInteger(gidField(slot), "nodeId");
        }
        item.components = base::sortedDigits(reader.integer(componentField(slot), "components", 0));
        card.items.push_back(item);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementExternalBoundaryFormat::check(const SuperelementExternalBoundaryCard& card)
{
    if (card.items.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one connection point " + std::string(CardProblem::FIELD),
                           "nodeId1"};
    for (const SuperelementExternalBoundaryItem& item : card.items)
    {
        if (item.points.first <= 0 || item.points.last <= 0)
            return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer", "nodeId"};
        if (item.points.last < item.points.first)
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "nodeId"};
        if (!base::isComponentSet(item.components))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
                "components"};
    }
    return std::nullopt;
}

void SuperelementExternalBoundaryFormat::write(const SuperelementExternalBoundaryCard& card, FieldWriter& writer)
{
    int slot = 0;
    for (const SuperelementExternalBoundaryItem& item : card.items)
    {
        const bool range = item.points.last != item.points.first;
        // A THRU is not accepted in the last pair of a line: that pair stays blank and the range starts the next line.
        if (range && slot % PAIRS_PER_LINE == PAIRS_PER_LINE - 1)
            ++slot;
        writer.integer(gidField(slot), item.points.first);
        if (range)
        {
            writer.text(componentField(slot), "Through");
            ++slot;
            writer.integer(gidField(slot), item.points.last);
        }
        writer.integerUnlessDefault(componentField(slot), item.components, 0);
        ++slot;
    }
}

std::uint32_t SuperelementExternalBoundaryTable::append(const SuperelementExternalBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_count.push(static_cast<std::int32_t>(card.items.size()));
    m_cards.push(card);
    return row;
}

void SuperelementExternalBoundaryTable::store(std::uint32_t row, const SuperelementExternalBoundaryCard& card)
{
    m_count.set(row, static_cast<std::int32_t>(card.items.size()));
    m_cards.set(row, card);
}

void SuperelementExternalBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_count.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementExternalBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (SuperelementExternalBoundaryItem& item : m_cards.edit(row).items)
        visitRun(visitor, "nodeId", item.points, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

std::vector<IdRun> SuperelementExternalBoundaryTable::connectionPoints() const
{
    std::vector<IdRun> runs;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        for (const SuperelementExternalBoundaryItem& item : m_cards.get(row).items)
            runs.push_back(item.points);
    }
    std::sort(runs.begin(), runs.end(), [](const IdRun& left, const IdRun& right) { return left.first < right.first; });
    std::vector<IdRun> merged;
    for (const IdRun& run : runs)
    {
        if (!merged.empty() && run.first <= merged.back().last)
            merged.back().last = std::max(merged.back().last, run.last);
        else
            merged.push_back(run);
    }
    return merged;
}

} // namespace oor::model
