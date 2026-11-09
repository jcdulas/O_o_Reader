#include "oor/model/cards/tables/tableDynamic4/TableDynamic4Table.hpp"

namespace oor::model
{

std::optional<TableDynamic4Card> TableDynamic4Format::read(FieldReader& reader)
{
    TableDynamic4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.abscissaShift = reader.requiredReal(2, "abscissaShift");
    card.abscissaScale = reader.requiredReal(3, "abscissaScale");
    card.abscissaMinimum = reader.requiredReal(4, "abscissaMinimum");
    card.abscissaMaximum = reader.requiredReal(5, "abscissaMaximum");
    for (int field = 6; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 6 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "abscissaMaximum");
    }
    readTableCoefficients(reader, FIRST_TABLE_DATA_FIELD, card.coefficients);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableDynamic4Format::check(const TableDynamic4Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    return checkTablePolynomial(card.abscissaShift, card.abscissaScale, card.abscissaMinimum, card.abscissaMaximum,
                                card.coefficients);
}

void TableDynamic4Format::write(const TableDynamic4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.abscissaShift);
    writer.real(3, card.abscissaScale);
    writer.real(4, card.abscissaMinimum);
    writer.real(5, card.abscissaMaximum);
    writeTableCoefficients(writer, FIRST_TABLE_DATA_FIELD, card.coefficients);
}

std::uint32_t TableDynamic4Table::append(const TableDynamic4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_abscissaShift.push(card.abscissaShift);
    m_abscissaScale.push(card.abscissaScale);
    m_abscissaMinimum.push(card.abscissaMinimum);
    m_abscissaMaximum.push(card.abscissaMaximum);
    m_coefficients.push(card.coefficients);
    return row;
}

TableDynamic4Card TableDynamic4Table::card(std::uint32_t row) const
{
    TableDynamic4Card card;
    card.id = m_id.get(row);
    card.abscissaShift = m_abscissaShift.get(row);
    card.abscissaScale = m_abscissaScale.get(row);
    card.abscissaMinimum = m_abscissaMinimum.get(row);
    card.abscissaMaximum = m_abscissaMaximum.get(row);
    const auto a = m_coefficients.get(row);
    card.coefficients.assign(a.begin(), a.end());
    return card;
}

void TableDynamic4Table::store(std::uint32_t row, const TableDynamic4Card& card)
{
    m_id.set(row, card.id);
    m_abscissaShift.set(row, card.abscissaShift);
    m_abscissaScale.set(row, card.abscissaScale);
    m_abscissaMinimum.set(row, card.abscissaMinimum);
    m_abscissaMaximum.set(row, card.abscissaMaximum);
    m_coefficients.set(row, card.coefficients);
}

void TableDynamic4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_abscissaShift.keepRows(rows);
    m_abscissaScale.keepRows(rows);
    m_abscissaMinimum.keepRows(rows);
    m_abscissaMaximum.keepRows(rows);
    m_coefficients.keepRows(rows);
}

double TableDynamic4Table::evaluate(const TableDynamic4Card& card, double x) noexcept
{
    return evaluateTablePolynomial(card.coefficients, card.abscissaShift, card.abscissaScale, card.abscissaMinimum,
                                   card.abscissaMaximum, x);
}

double TableDynamic4Table::evaluateRow(std::uint32_t row, double x) const
{
    return evaluateTablePolynomial(m_coefficients.get(row), m_abscissaShift.get(row), m_abscissaScale.get(row),
                                   m_abscissaMinimum.get(row), m_abscissaMaximum.get(row), x);
}

void TableDynamic4Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto a = m_coefficients.get(row);
    const double abscissaShift = m_abscissaShift.get(row);
    const double abscissaScale = m_abscissaScale.get(row);
    const double abscissaMinimum = m_abscissaMinimum.get(row);
    const double abscissaMaximum = m_abscissaMaximum.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = evaluateTablePolynomial(a, abscissaShift, abscissaScale, abscissaMinimum, abscissaMaximum, x[index]);
}

} // namespace oor::model
