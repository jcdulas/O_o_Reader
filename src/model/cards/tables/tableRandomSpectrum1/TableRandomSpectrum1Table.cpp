#include "oor/model/cards/tables/tableRandomSpectrum1/TableRandomSpectrum1Table.hpp"

#include <string>

namespace oor::model
{

namespace
{

// XAXIS or YAXIS: blank is LINEAR.
TableAxis readAxis(FieldReader& reader, int field, const char* name)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return TableAxis::Linear;
    if (const auto axis = parseTableAxis(text))
        return *axis;
    reader.error("card.bad-value", std::string(name) + " must be LINEAR or LOG, found \"" + std::string(text) + "\"",
                 name);
    return TableAxis::Linear;
}

} // namespace

std::optional<TableRandomSpectrum1Card> TableRandomSpectrum1Format::read(FieldReader& reader)
{
    TableRandomSpectrum1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.xAxisKind = readAxis(reader, 2, "xAxisKind");
    card.yAxisKind = readAxis(reader, 3, "yAxisKind");
    for (int field = 4; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 4 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "yAxisKind");
    }
    readTablePoints(reader, FIRST_TABLE_DATA_FIELD, card.x, card.y);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableRandomSpectrum1Format::check(const TableRandomSpectrum1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (const auto problem = checkTablePoints(card.x, card.y, card.xAxisKind, card.yAxisKind))
        return problem;
    if (card.x.size() < 2)
        return CardProblem{"card.bad-value", "the table needs at least two points", "x"};
    for (const double frequency : card.x)
    {
        if (frequency < 0.0)
            return CardProblem{"card.bad-value", "a frequency must not be negative", "frequency"};
    }
    return std::nullopt;
}

void TableRandomSpectrum1Format::write(const TableRandomSpectrum1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, tableAxisWord(card.xAxisKind), "Linear");
    writer.textUnlessDefault(3, tableAxisWord(card.yAxisKind), "Linear");
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableRandomSpectrum1Table::append(const TableRandomSpectrum1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_xAxisKind.push(static_cast<std::uint8_t>(card.xAxisKind));
    m_yAxisKind.push(static_cast<std::uint8_t>(card.yAxisKind));
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableRandomSpectrum1Card TableRandomSpectrum1Table::card(std::uint32_t row) const
{
    TableRandomSpectrum1Card card;
    card.id = m_id.get(row);
    card.xAxisKind = static_cast<TableAxis>(m_xAxisKind.get(row));
    card.yAxisKind = static_cast<TableAxis>(m_yAxisKind.get(row));
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableRandomSpectrum1Table::store(std::uint32_t row, const TableRandomSpectrum1Card& card)
{
    m_id.set(row, card.id);
    m_xAxisKind.set(row, static_cast<std::uint8_t>(card.xAxisKind));
    m_yAxisKind.set(row, static_cast<std::uint8_t>(card.yAxisKind));
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableRandomSpectrum1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_xAxisKind.keepRows(rows);
    m_yAxisKind.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

double TableRandomSpectrum1Table::evaluate(const TableRandomSpectrum1Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, card.xAxisKind, card.yAxisKind, x);
}

double TableRandomSpectrum1Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), static_cast<TableAxis>(m_xAxisKind.get(row)),
                            static_cast<TableAxis>(m_yAxisKind.get(row)), x);
}

void TableRandomSpectrum1Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    const auto xAxisKind = static_cast<TableAxis>(m_xAxisKind.get(row));
    const auto yAxisKind = static_cast<TableAxis>(m_yAxisKind.get(row));
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = interpolateTable(xs, ys, xAxisKind, yAxisKind, x[index]);
}

} // namespace oor::model
