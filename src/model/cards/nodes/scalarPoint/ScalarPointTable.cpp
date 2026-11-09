#include "oor/model/cards/nodes/scalarPoint/ScalarPointTable.hpp"

#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

std::vector<ScalarPointCard> ScalarPointFormat::readAll(FieldReader& reader)
{
    std::vector<ScalarPointCard> cards;
    const int last = reader.lastUsedField();
    bool lastIsSingle = false; // the last point read was written alone, so a THRU may follow it
    for (int field = 1; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            if (!lastIsSingle)
            {
                reader.error("card.bad-value", "THRU must follow a single point ID", "through");
                break;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last point ID", "lastId");
                break;
            }
            const base::EntityId end = reader.requiredInteger(next, "lastId");
            const base::EntityId first = cards.back().id;
            if (reader.failed())
                break;
            if (end <= first)
            {
                reader.error("card.bad-value", "the end of a THRU range must be above its start", "lastId");
                break;
            }
            // A hostile "1 THRU 2000000000" must not fill the memory (the bound of every THRU expansion).
            if (base::idRangeCount(first, end) > static_cast<std::uint64_t>(base::MAX_EXPANDED_IDS))
            {
                reader.error("card.bad-value", "a THRU range may not define more than 10 million points", "lastId");
                break;
            }
            // 64-bit: an end of 2147483647 must not wrap the counter
            for (std::int64_t id = std::int64_t{first} + 1; id <= end; ++id)
                cards.push_back(ScalarPointCard{static_cast<base::EntityId>(id)});
            lastIsSingle = false;
            field = next;
            continue;
        }
        cards.push_back(ScalarPointCard{reader.requiredInteger(field, "id")});
        lastIsSingle = true;
    }
    if (!reader.failed() && cards.empty())
        reader.error("card.missing-field", "an " + std::string(reader.cardName()) + " card needs at least one point ID",
                     "firstId");
    if (!reader.failed())
    {
        for (const ScalarPointCard& card : cards)
        {
            if (const auto problem = check(card))
            {
                reader.report(problem);
                break;
            }
        }
    }
    return cards;
}

std::optional<CardProblem> ScalarPointFormat::check(const ScalarPointCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the scalar point ID must be a positive integer", "id"};
    return std::nullopt;
}

void ScalarPointFormat::write(const ScalarPointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
}

bool ScalarPointTable::continuesRun(std::uint32_t previous, std::uint32_t row) const
{
    return static_cast<std::int64_t>(m_id.get(previous)) + 1 == static_cast<std::int64_t>(m_id.get(row));
}

void ScalarPointTable::writeRun(std::uint32_t row, std::uint32_t count, FieldWriter& writer) const
{
    writer.integer(1, m_id.get(row));
    writer.text(2, "Through");
    writer.integer(3, m_id.get(row + count - 1));
}

std::uint32_t ScalarPointTable::append(const ScalarPointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    return row;
}

ScalarPointCard ScalarPointTable::card(std::uint32_t row) const
{
    return ScalarPointCard{m_id.get(row)};
}

void ScalarPointTable::store(std::uint32_t row, const ScalarPointCard& card)
{
    m_id.set(row, card.id);
}

void ScalarPointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
}

} // namespace oor::model
