#include "oor/model/cards/tables/tableMaterial2/TableMaterial2Table.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TableMaterial2Card> TableMaterial2Format::read(FieldReader& reader)
{
    TableMaterial2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.abscissaShift = reader.real(2, "abscissaShift", 0.0);
    for (int field = 3; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 3 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "abscissaShift");
    }
    readTablePoints(reader, FIRST_TABLE_DATA_FIELD, card.x, card.y);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableMaterial2Format::check(const TableMaterial2Card& card)
{
    if (card.id == 0)
        return CardProblem{"card.bad-id", "the table ID must not be zero", "id"};
    if (!std::isfinite(card.abscissaShift))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "abscissaShift"};
    return checkTablePoints(card.x, card.y, TableAxis::Linear, TableAxis::Linear);
}

void TableMaterial2Format::write(const TableMaterial2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.abscissaShift, 0.0);
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableMaterial2Table::append(const TableMaterial2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_abscissaShift.push(card.abscissaShift);
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableMaterial2Card TableMaterial2Table::card(std::uint32_t row) const
{
    TableMaterial2Card card;
    card.id = m_id.get(row);
    card.abscissaShift = m_abscissaShift.get(row);
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableMaterial2Table::store(std::uint32_t row, const TableMaterial2Card& card)
{
    m_id.set(row, card.id);
    m_abscissaShift.set(row, card.abscissaShift);
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableMaterial2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_abscissaShift.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

// y = yT(x - X1): the points are looked up at x - X1.
double TableMaterial2Table::evaluate(const TableMaterial2Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, TableAxis::Linear, TableAxis::Linear, x - card.abscissaShift);
}

double TableMaterial2Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), TableAxis::Linear, TableAxis::Linear,
                            x - m_abscissaShift.get(row));
}

void TableMaterial2Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    const double abscissaShift = m_abscissaShift.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = interpolateTable(xs, ys, TableAxis::Linear, TableAxis::Linear, x[index] - abscissaShift);
}

} // namespace oor::model
