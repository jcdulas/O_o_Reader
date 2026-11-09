#include "oor/model/cards/tables/tableHeat1/TableHeat1Table.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

// The value of the table at `at`: the look-up of the format reference is linear inside the points and
// constant outside.
double lookUp(std::span<const double> x, std::span<const double> y, double at)
{
    if (std::isnan(at) || x.empty())
        return std::numeric_limits<double>::quiet_NaN();
    const double clamped = std::clamp(at, x.front(), x.back());
    return interpolateTable(x, y, TableAxis::Linear, TableAxis::Linear, clamped);
}

} // namespace

std::optional<TableHeat1Card> TableHeat1Format::read(FieldReader& reader)
{
    TableHeat1Card card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "id");
    }
    readTablePoints(reader, FIRST_TABLE_DATA_FIELD, card.x, card.y);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableHeat1Format::check(const TableHeat1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (const auto problem = checkTablePoints(card.x, card.y, TableAxis::Linear, TableAxis::Linear))
        return problem;
    if (card.x.size() > 1 && card.x.back() < card.x.front())
        return CardProblem{"card.bad-value", "the independent variable must be ascending", "x"};
    return std::nullopt;
}

void TableHeat1Format::write(const TableHeat1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableHeat1Table::append(const TableHeat1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableHeat1Card TableHeat1Table::card(std::uint32_t row) const
{
    TableHeat1Card card;
    card.id = m_id.get(row);
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableHeat1Table::store(std::uint32_t row, const TableHeat1Card& card)
{
    m_id.set(row, card.id);
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableHeat1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

double TableHeat1Table::evaluate(const TableHeat1Card& card, double x) noexcept
{
    return lookUp(card.x, card.y, x);
}

double TableHeat1Table::evaluateRow(std::uint32_t row, double x) const
{
    return lookUp(m_x.get(row), m_y.get(row), x);
}

void TableHeat1Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = lookUp(xs, ys, x[index]);
}

} // namespace oor::model
