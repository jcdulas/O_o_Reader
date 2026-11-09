#include "oor/model/cards/tables/tableFatigueHistory/TableFatigueHistoryTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

namespace oor::model
{

namespace
{

// The values start on the first continuation line.
constexpr int FIRST_VALUE_FIELD = 9;

bool isTableEnd(std::string_view text)
{
    return (text == "EndTable");
}

} // namespace

std::optional<TableFatigueHistoryCard> TableFatigueHistoryFormat::read(FieldReader& reader)
{
    TableFatigueHistoryCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_VALUE_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "id");
    }
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = FIRST_VALUE_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (isTableEnd(reader.text(field)))
        {
            ended = true;
            if (last > field)
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            break;
        }
        card.ordinate.push_back(reader.requiredReal(field, "ordinate"));
    }
    if (!ended && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableFatigueHistoryFormat::check(const TableFatigueHistoryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.ordinate.empty())
        return CardProblem{"card.missing-field", "the table needs at least one value", "ordinate"};
    return checkFiniteTableValues(card.ordinate, "ordinate");
}

void TableFatigueHistoryFormat::write(const TableFatigueHistoryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_VALUE_FIELD;
    for (const double value : card.ordinate)
        writer.real(field++, value);
    writer.text(field, "EndTable");
}

std::uint32_t TableFatigueHistoryTable::append(const TableFatigueHistoryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ordinate.push(card.ordinate);
    return row;
}

TableFatigueHistoryCard TableFatigueHistoryTable::card(std::uint32_t row) const
{
    TableFatigueHistoryCard card;
    card.id = m_id.get(row);
    const auto y = m_ordinate.get(row);
    card.ordinate.assign(y.begin(), y.end());
    return card;
}

void TableFatigueHistoryTable::store(std::uint32_t row, const TableFatigueHistoryCard& card)
{
    m_id.set(row, card.id);
    m_ordinate.set(row, card.ordinate);
}

void TableFatigueHistoryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ordinate.keepRows(rows);
}

} // namespace oor::model
