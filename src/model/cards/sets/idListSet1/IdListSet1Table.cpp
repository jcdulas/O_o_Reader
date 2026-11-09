#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 2;

} // namespace

std::optional<IdListSet1Card> IdListSet1Format::read(FieldReader& reader)
{
    IdListSet1Card card;
    card.id = reader.requiredInteger(1, "id");
    int first = FIRST_ID_FIELD;
    if ((reader.text(FIRST_ID_FIELD) == IdListSet1Table::SKIN))
    {
        card.skin = true;
        ++first;
    }
    card.ids = readIdRuns(reader, first, "ids");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IdListSet1Format::check(const IdListSet1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.skin && !card.ids.empty())
        return CardProblem{"card.bad-value", "a set with " + std::string(CardProblem::FIELD) + " has no list of IDs",
                           "skin"};
    return checkIdRuns(card.ids, "ids", IdSign::NotZero, card.skin);
}

void IdListSet1Format::write(const IdListSet1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int first = FIRST_ID_FIELD;
    if (card.skin)
        writer.text(first++, IdListSet1Table::SKIN);
    writeIdRuns(writer, first, card.ids, writer.compressesRanges());
}

std::uint32_t IdListSet1Table::append(const IdListSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_skin.push(card.skin ? 1 : 0);
    m_runs.push(toPairs(card.ids));
    return row;
}

IdListSet1Card IdListSet1Table::card(std::uint32_t row) const
{
    IdListSet1Card card;
    card.id = m_id.get(row);
    card.skin = m_skin.get(row) != 0;
    card.ids = toIdRuns(m_runs.get(row));
    return card;
}

void IdListSet1Table::store(std::uint32_t row, const IdListSet1Card& card)
{
    m_id.set(row, card.id);
    m_skin.set(row, card.skin ? 1 : 0);
    m_runs.set(row, toPairs(card.ids));
}

void IdListSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_skin.keepRows(rows);
    m_runs.keepRows(rows);
}

void IdListSet1Table::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The IDs are grid points or elements, as the cards that use the set decide: none of them is a reference.
}

} // namespace oor::model
