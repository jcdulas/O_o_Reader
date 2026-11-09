#include "oor/model/cards/tables/tableMaterial1/TableMaterial1Table.hpp"

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

std::optional<TableMaterial1Card> TableMaterial1Format::read(FieldReader& reader)
{
    TableMaterial1Card card;
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

std::optional<CardProblem> TableMaterial1Format::check(const TableMaterial1Card& card)
{
    if (card.id == 0)
        return CardProblem{"card.bad-id", "the table ID must not be zero", "id"};
    return checkTablePoints(card.x, card.y, card.xAxisKind, card.yAxisKind);
}

void TableMaterial1Format::write(const TableMaterial1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, tableAxisWord(card.xAxisKind), "Linear");
    writer.textUnlessDefault(3, tableAxisWord(card.yAxisKind), "Linear");
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableMaterial1Table::append(const TableMaterial1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_xAxisKind.push(static_cast<std::uint8_t>(card.xAxisKind));
    m_yAxisKind.push(static_cast<std::uint8_t>(card.yAxisKind));
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableMaterial1Card TableMaterial1Table::card(std::uint32_t row) const
{
    TableMaterial1Card card;
    card.id = m_id.get(row);
    card.xAxisKind = static_cast<TableAxis>(m_xAxisKind.get(row));
    card.yAxisKind = static_cast<TableAxis>(m_yAxisKind.get(row));
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableMaterial1Table::store(std::uint32_t row, const TableMaterial1Card& card)
{
    m_id.set(row, card.id);
    m_xAxisKind.set(row, static_cast<std::uint8_t>(card.xAxisKind));
    m_yAxisKind.set(row, static_cast<std::uint8_t>(card.yAxisKind));
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableMaterial1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_xAxisKind.keepRows(rows);
    m_yAxisKind.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

double TableMaterial1Table::evaluate(const TableMaterial1Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, card.xAxisKind, card.yAxisKind, x);
}

double TableMaterial1Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), static_cast<TableAxis>(m_xAxisKind.get(row)),
                            static_cast<TableAxis>(m_yAxisKind.get(row)), x);
}

void TableMaterial1Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    const auto xAxisKind = static_cast<TableAxis>(m_xAxisKind.get(row));
    const auto yAxisKind = static_cast<TableAxis>(m_yAxisKind.get(row));
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = interpolateTable(xs, ys, xAxisKind, yAxisKind, x[index]);
}

} // namespace oor::model
