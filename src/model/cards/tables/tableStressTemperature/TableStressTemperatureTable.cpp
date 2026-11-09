#include "oor/model/cards/tables/tableStressTemperature/TableStressTemperatureTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TableStressTemperatureCard> TableStressTemperatureFormat::read(FieldReader& reader)
{
    TableStressTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "id");
    }
    // The pairs (temperature, table ID) up to the field that has ENDT in either of its two fields.
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
        const double temperature = reader.requiredReal(field, "temperatures");
        const base::EntityId table = reader.requiredInteger(field + 1, "tables");
        if (reader.failed())
            break;
        card.temperatures.push_back(temperature);
        card.tables.push_back(table);
    }
    if (!ended && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableStressTemperatureFormat::check(const TableStressTemperatureCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (card.temperatures.size() != card.tables.size())
        return CardProblem{"card.bad-value", "there must be one table ID for each temperature", "tables"};
    if (card.temperatures.empty())
        return CardProblem{"card.missing-field", "a table needs at least one temperature", "temperatures"};
    for (std::size_t index = 0; index < card.temperatures.size(); ++index)
    {
        if (!std::isfinite(card.temperatures[index]))
            return CardProblem{"card.bad-value", "a temperature must be a finite number", "temperatures"};
        if (index > 0 && card.temperatures[index] < card.temperatures[index - 1])
            return CardProblem{"card.bad-value", "the temperatures must be listed in ascending order", "temperatures"};
        if (card.tables[index] <= 0)
            return CardProblem{"card.bad-id", "a table ID must be a positive integer", "tables"};
    }
    return std::nullopt;
}

void TableStressTemperatureFormat::write(const TableStressTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_TABLE_DATA_FIELD;
    for (std::size_t index = 0; index < card.temperatures.size() && index < card.tables.size(); ++index)
    {
        writer.real(field, card.temperatures[index]);
        writer.integer(field + 1, card.tables[index]);
        field += 2;
    }
    writer.text(field, "EndTable");
}

std::uint32_t TableStressTemperatureTable::append(const TableStressTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_temperatures.push(card.temperatures);
    m_tables.push(card.tables);
    return row;
}

TableStressTemperatureCard TableStressTemperatureTable::card(std::uint32_t row) const
{
    TableStressTemperatureCard card;
    card.id = m_id.get(row);
    const auto temperatures = m_temperatures.get(row);
    const auto tables = m_tables.get(row);
    card.temperatures.assign(temperatures.begin(), temperatures.end());
    card.tables.assign(tables.begin(), tables.end());
    return card;
}

void TableStressTemperatureTable::store(std::uint32_t row, const TableStressTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_temperatures.set(row, card.temperatures);
    m_tables.set(row, card.tables);
}

void TableStressTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_temperatures.keepRows(rows);
    m_tables.keepRows(rows);
}

void TableStressTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The table IDs of the pairs are TABLES1 cards (or other TABLEST: the stress table space has both).
    const auto current = m_tables.get(row);
    std::vector<base::EntityId> tables(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& table : tables)
    {
        const base::EntityId updated = visitor.visit("tables", ReferenceTarget::StressTable, table);
        changed = changed || updated != table;
        table = updated;
    }
    if (changed)
        m_tables.set(row, tables);
}

} // namespace oor::model
