#include "oor/model/cards/tables/tableDynamic5/TableDynamic5Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"
#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Table.hpp"
#include "oor/model/cards/tables/tableDynamic2/TableDynamic2Table.hpp"
#include "oor/model/cards/tables/tableDynamic3/TableDynamic3Table.hpp"
#include "oor/model/cards/tables/tableDynamic4/TableDynamic4Table.hpp"
#include "oor/model/core/Model.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 9;

bool isEndOfTable(std::string_view text)
{
    return (text == "EndTable");
}

// The value at `y` of the TABLED1 to TABLED4 with this ID in the model; NaN when there is none (or it is a TABLED5).
double evaluateReferenced(const Model& model, base::EntityId table, double y)
{
    const std::optional<CardLocation> location = model.locate(base::IdSpace::DynamicTable, table);
    if (!location)
        return std::numeric_limits<double>::quiet_NaN();
    if (location->table == typeIndexOf<TableDynamic1Table>())
        return model.findTable<TableDynamic1Table>()->evaluateRow(location->row, y);
    if (location->table == typeIndexOf<TableDynamic2Table>())
        return model.findTable<TableDynamic2Table>()->evaluateRow(location->row, y);
    if (location->table == typeIndexOf<TableDynamic3Table>())
        return model.findTable<TableDynamic3Table>()->evaluateRow(location->row, y);
    if (location->table == typeIndexOf<TableDynamic4Table>())
        return model.findTable<TableDynamic4Table>()->evaluateRow(location->row, y);
    return std::numeric_limits<double>::quiet_NaN();
}

double evaluatePairs(const Model& model, std::span<const double> x, std::span<const base::EntityId> tables, double atX,
                     double atY)
{
    if (std::isnan(atX) || std::isnan(atY) || x.empty() || x.size() != tables.size())
        return std::numeric_limits<double>::quiet_NaN();
    std::vector<double> values;
    values.reserve(tables.size());
    for (const base::EntityId table : tables)
        values.push_back(evaluateReferenced(model, table, atY));
    return interpolateTable(x, values, TableAxis::Linear, TableAxis::Linear, atX);
}

} // namespace

std::optional<TableDynamic5Card> TableDynamic5Format::read(FieldReader& reader)
{
    TableDynamic5Card card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_PAIR_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "id");
    }
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (isEndOfTable(reader.text(field)) || isEndOfTable(reader.text(field + 1)))
        {
            ended = true;
            if (last > field + 1)
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            break;
        }
        // A pair with both fields blank is padding: a line that stops short, before its continuation.
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
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

std::optional<CardProblem> TableDynamic5Format::check(const TableDynamic5Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (card.x.size() != card.tables.size())
        return CardProblem{"card.bad-value", "x and the table IDs must have the same number of values", "tables"};
    if (card.x.empty())
        return CardProblem{"card.missing-field",
                           "the table needs at least one pair ({field}, " + CardProblem::fieldNamed("tables") + ")",
                           "x"};
    for (std::size_t index = 0; index < card.x.size(); ++index)
    {
        if (!std::isfinite(card.x[index]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "x"};
        if (index > 0 && !(card.x[index] > card.x[index - 1]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be in ascending order", "x"};
        if (card.tables[index] <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "tables"};
    }
    return std::nullopt;
}

void TableDynamic5Format::write(const TableDynamic5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_PAIR_FIELD;
    for (std::size_t index = 0; index < card.x.size() && index < card.tables.size(); ++index)
    {
        writer.real(field, card.x[index]);
        writer.integer(field + 1, card.tables[index]);
        field += 2;
    }
    writer.text(field, "EndTable");
}

void TableDynamic5Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto tables = m_tables.get(row);
    std::vector<base::EntityId> updated(tables.begin(), tables.end());
    bool changed = false;
    for (base::EntityId& table : updated)
    {
        const base::EntityId renumbered = visitor.visit("tables", ReferenceTarget::DynamicTable, table);
        changed = changed || renumbered != table;
        table = renumbered;
    }
    if (changed)
        m_tables.set(row, updated);
}

std::uint32_t TableDynamic5Table::append(const TableDynamic5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_x.push(card.x);
    m_tables.push(card.tables);
    return row;
}

TableDynamic5Card TableDynamic5Table::card(std::uint32_t row) const
{
    TableDynamic5Card card;
    card.id = m_id.get(row);
    const auto x = m_x.get(row);
    const auto tables = m_tables.get(row);
    card.x.assign(x.begin(), x.end());
    card.tables.assign(tables.begin(), tables.end());
    return card;
}

void TableDynamic5Table::store(std::uint32_t row, const TableDynamic5Card& card)
{
    m_id.set(row, card.id);
    m_x.set(row, card.x);
    m_tables.set(row, card.tables);
}

void TableDynamic5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_x.keepRows(rows);
    m_tables.keepRows(rows);
}

double TableDynamic5Table::evaluate(const Model& model, const TableDynamic5Card& card, double x, double y)
{
    return evaluatePairs(model, card.x, card.tables, x, y);
}

double TableDynamic5Table::evaluateRow(const Model& model, std::uint32_t row, double x, double y) const
{
    return evaluatePairs(model, m_x.get(row), m_tables.get(row), x, y);
}

} // namespace oor::model
