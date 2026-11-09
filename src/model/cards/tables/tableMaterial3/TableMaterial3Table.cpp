#include "oor/model/cards/tables/tableMaterial3/TableMaterial3Table.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TableMaterial3Card> TableMaterial3Format::read(FieldReader& reader)
{
    TableMaterial3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.abscissaShift = reader.requiredReal(2, "abscissaShift");
    card.abscissaScale = reader.requiredReal(3, "abscissaScale");
    for (int field = 4; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 4 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "abscissaScale");
    }
    readTablePoints(reader, FIRST_TABLE_DATA_FIELD, card.x, card.y);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableMaterial3Format::check(const TableMaterial3Card& card)
{
    if (card.id == 0)
        return CardProblem{"card.bad-id", "the table ID must not be zero", "id"};
    if (!std::isfinite(card.abscissaShift))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "abscissaShift"};
    if (!std::isfinite(card.abscissaScale) || card.abscissaScale == 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a finite number other than zero",
                           "abscissaScale"};
    return checkTablePoints(card.x, card.y, TableAxis::Linear, TableAxis::Linear);
}

void TableMaterial3Format::write(const TableMaterial3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.abscissaShift);
    writer.real(3, card.abscissaScale);
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableMaterial3Table::append(const TableMaterial3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_abscissaShift.push(card.abscissaShift);
    m_abscissaScale.push(card.abscissaScale);
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableMaterial3Card TableMaterial3Table::card(std::uint32_t row) const
{
    TableMaterial3Card card;
    card.id = m_id.get(row);
    card.abscissaShift = m_abscissaShift.get(row);
    card.abscissaScale = m_abscissaScale.get(row);
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableMaterial3Table::store(std::uint32_t row, const TableMaterial3Card& card)
{
    m_id.set(row, card.id);
    m_abscissaShift.set(row, card.abscissaShift);
    m_abscissaScale.set(row, card.abscissaScale);
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableMaterial3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_abscissaShift.keepRows(rows);
    m_abscissaScale.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

// y = yT((x - X1) / X2): the points are looked up at (x - X1) / X2.
double TableMaterial3Table::evaluate(const TableMaterial3Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, TableAxis::Linear, TableAxis::Linear,
                            (x - card.abscissaShift) / card.abscissaScale);
}

double TableMaterial3Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), TableAxis::Linear, TableAxis::Linear,
                            (x - m_abscissaShift.get(row)) / m_abscissaScale.get(row));
}

void TableMaterial3Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    const double abscissaShift = m_abscissaShift.get(row);
    const double abscissaScale = m_abscissaScale.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] =
            interpolateTable(xs, ys, TableAxis::Linear, TableAxis::Linear, (x[index] - abscissaShift) / abscissaScale);
}

} // namespace oor::model
