#include "oor/model/cards/tables/tableStress1/TableStress1Table.hpp"

namespace oor::model
{

std::optional<TableStress1Card> TableStress1Format::read(FieldReader& reader)
{
    TableStress1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = reader.integer(2, "kind", 1);
    for (int field = 3; field < FIRST_TABLE_DATA_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 3 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "kind");
    }
    readTablePoints(reader, FIRST_TABLE_DATA_FIELD, card.x, card.y);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableStress1Format::check(const TableStress1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (card.kind != 1 && card.kind != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "kind"};
    return checkTablePoints(card.x, card.y, TableAxis::Linear, TableAxis::Linear);
}

void TableStress1Format::write(const TableStress1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.kind, 1);
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableStress1Table::append(const TableStress1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(card.kind);
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableStress1Card TableStress1Table::card(std::uint32_t row) const
{
    TableStress1Card card;
    card.id = m_id.get(row);
    card.kind = m_kind.get(row);
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableStress1Table::store(std::uint32_t row, const TableStress1Card& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, card.kind);
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableStress1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

double TableStress1Table::evaluate(const TableStress1Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, TableAxis::Linear, TableAxis::Linear, x);
}

double TableStress1Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), TableAxis::Linear, TableAxis::Linear, x);
}

void TableStress1Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = interpolateTable(xs, ys, TableAxis::Linear, TableAxis::Linear, x[index]);
}

} // namespace oor::model
