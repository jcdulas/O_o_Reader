#include "oor/model/cards/tables/tableModalDamping1/TableModalDamping1Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

const char* typeWord(DampingType type)
{
    switch (type)
    {
    case DampingType::G:
        return "StructuralDamping";
    case DampingType::Crit:
        return "CriticalDamping";
    case DampingType::Q:
        return "QualityFactor";
    }
    return "StructuralDamping";
}

} // namespace

std::optional<TableModalDamping1Card> TableModalDamping1Format::read(FieldReader& reader)
{
    TableModalDamping1Card card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view typeText = reader.text(2);
    if (typeText.empty() || (typeText == "StructuralDamping"))
        card.kind = DampingType::G;
    else if ((typeText == "CriticalDamping"))
        card.kind = DampingType::Crit;
    else if ((typeText == "QualityFactor"))
        card.kind = DampingType::Q;
    else
        reader.error("card.bad-value",
                     std::string("kind") + " must be G, CRIT or Q, found \"" + std::string(typeText) + "\"", "kind");
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

std::optional<CardProblem> TableModalDamping1Format::check(const TableModalDamping1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (const auto problem = checkTablePoints(card.x, card.y, TableAxis::Linear, TableAxis::Linear))
        return problem;
    for (const double frequency : card.x)
    {
        // The format reference asks for fi > 0.0; a table that starts at 0.0 is accepted.
        if (frequency < 0.0)
            return CardProblem{"card.bad-value", "a natural frequency must not be negative", "frequency"};
    }
    return std::nullopt;
}

void TableModalDamping1Format::write(const TableModalDamping1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, typeWord(card.kind), "StructuralDamping");
    writeTablePoints(writer, FIRST_TABLE_DATA_FIELD, card.x, card.y);
}

std::uint32_t TableModalDamping1Table::append(const TableModalDamping1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(static_cast<std::uint8_t>(card.kind));
    m_x.push(card.x);
    m_y.push(card.y);
    return row;
}

TableModalDamping1Card TableModalDamping1Table::card(std::uint32_t row) const
{
    TableModalDamping1Card card;
    card.id = m_id.get(row);
    card.kind = static_cast<DampingType>(m_type.get(row));
    const auto x = m_x.get(row);
    const auto y = m_y.get(row);
    card.x.assign(x.begin(), x.end());
    card.y.assign(y.begin(), y.end());
    return card;
}

void TableModalDamping1Table::store(std::uint32_t row, const TableModalDamping1Card& card)
{
    m_id.set(row, card.id);
    m_type.set(row, static_cast<std::uint8_t>(card.kind));
    m_x.set(row, card.x);
    m_y.set(row, card.y);
}

void TableModalDamping1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_x.keepRows(rows);
    m_y.keepRows(rows);
}

double TableModalDamping1Table::evaluate(const TableModalDamping1Card& card, double x) noexcept
{
    return interpolateTable(card.x, card.y, TableAxis::Linear, TableAxis::Linear, x);
}

double TableModalDamping1Table::evaluateRow(std::uint32_t row, double x) const
{
    return interpolateTable(m_x.get(row), m_y.get(row), TableAxis::Linear, TableAxis::Linear, x);
}

void TableModalDamping1Table::evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const
{
    const auto xs = m_x.get(row);
    const auto ys = m_y.get(row);
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
        y[index] = interpolateTable(xs, ys, TableAxis::Linear, TableAxis::Linear, x[index]);
}

} // namespace oor::model
