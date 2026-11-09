#include "oor/model/cards/tables/tableHeatTwoVariables/TableHeatTwoVariablesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TableHeatTwoVariablesCard> TableHeatTwoVariablesFormat::read(FieldReader& reader)
{
    TableHeatTwoVariablesCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "id");
    }
    // The pairs (x, table ID) up to the field that has ENDT in either of its two fields.
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = FIRST_TABLE_DATA_FIELD; field <= last && !reader.failed(); field += 2)
    {
        const std::string_view first = reader.text(field);
        const std::string_view second = reader.text(field + 1);
        if ((first == "EndTable") || (second == "EndTable"))
        {
            ended = true;
            if (last > field + 1)
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            break;
        }
        if (first.empty() && second.empty())
            continue; // a pair with both fields blank is padding: a line that stops short
        const double x = reader.requiredReal(field, "x");
        const base::EntityId table = reader.requiredInteger(field + 1, "tables");
        if (reader.failed())
            break;
        card.x.push_back(x);
        card.tables.push_back(table);
    }
    if (!ended && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableHeatTwoVariablesFormat::check(const TableHeatTwoVariablesCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (card.x.size() != card.tables.size())
        return CardProblem{"card.bad-value", "there must be one table ID for each value of x", "tables"};
    if (card.x.empty())
        return CardProblem{"card.missing-field", "a table needs at least one pair", "x"};
    for (std::size_t index = 0; index < card.x.size(); ++index)
    {
        if (!std::isfinite(card.x[index]))
            return CardProblem{"card.bad-value", "x must be a finite number", "x"};
        if (index > 0 && card.x[index] < card.x[index - 1])
            return CardProblem{"card.bad-value", "the values of x must be listed in ascending order", "x"};
        if (card.tables[index] <= 0)
            return CardProblem{"card.bad-id", "a table ID must be a positive integer", "tables"};
    }
    return std::nullopt;
}

void TableHeatTwoVariablesFormat::write(const TableHeatTwoVariablesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_TABLE_DATA_FIELD;
    for (std::size_t index = 0; index < card.x.size() && index < card.tables.size(); ++index)
    {
        writer.real(field, card.x[index]);
        writer.integer(field + 1, card.tables[index]);
        field += 2;
    }
    writer.text(field, "EndTable");
}

std::uint32_t TableHeatTwoVariablesTable::append(const TableHeatTwoVariablesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_x.push(card.x);
    m_tables.push(card.tables);
    return row;
}

TableHeatTwoVariablesCard TableHeatTwoVariablesTable::card(std::uint32_t row) const
{
    TableHeatTwoVariablesCard card;
    card.id = m_id.get(row);
    const auto x = m_x.get(row);
    const auto tables = m_tables.get(row);
    card.x.assign(x.begin(), x.end());
    card.tables.assign(tables.begin(), tables.end());
    return card;
}

void TableHeatTwoVariablesTable::store(std::uint32_t row, const TableHeatTwoVariablesCard& card)
{
    m_id.set(row, card.id);
    m_x.set(row, card.x);
    m_tables.set(row, card.tables);
}

void TableHeatTwoVariablesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_x.keepRows(rows);
    m_tables.keepRows(rows);
}

void TableHeatTwoVariablesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The table IDs of the pairs are TABLEH1 cards (or other TABLEHT: the heat table space has both).
    const auto current = m_tables.get(row);
    std::vector<base::EntityId> tables(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& table : tables)
    {
        const base::EntityId updated = visitor.visit("tables", ReferenceTarget::HeatTable, table);
        changed = changed || updated != table;
        table = updated;
    }
    if (changed)
        m_tables.set(row, tables);
}

} // namespace oor::model
