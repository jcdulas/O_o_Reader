#include "oor/model/cards/tables/tableScattered3D/TableScattered3DTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The points start on the first continuation line, four fields each.
constexpr int FIRST_POINT_FIELD = 9;
constexpr int FIELDS_PER_POINT = 4;
// A table that has fewer points than two would have fewer than two continuation entries.
constexpr std::size_t MIN_POINTS = 2;

bool isTableEnd(std::string_view text)
{
    return (text == "EndTable");
}

bool isFinite(const TableScattered3DPoint& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z) && std::isfinite(point.value);
}

} // namespace

std::optional<TableScattered3DCard> TableScattered3DFormat::read(FieldReader& reader)
{
    TableScattered3DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.xOffset = reader.real(2, "xOffset", 0.0);
    card.yOffset = reader.real(3, "yOffset", 0.0);
    card.zOffset = reader.real(4, "zOffset", 0.0);
    card.valueOffset = reader.real(5, "valueOffset", 0.0);
    for (int field = 6; field < FIRST_POINT_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 6 to 8 of a " + std::string(reader.cardName()) + " are not used",
                         "valueOffset");
    }
    const int last = reader.lastUsedField();
    int endField = 0;
    for (int field = FIRST_POINT_FIELD; field <= last && endField == 0; ++field)
    {
        if (isTableEnd(reader.text(field)))
            endField = field;
    }
    if (endField == 0)
    {
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
        return card;
    }
    if (last > endField)
    {
        reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
        return card;
    }
    if ((endField - FIRST_POINT_FIELD) % FIELDS_PER_POINT != 0)
    {
        reader.error("card.bad-value",
                     std::string("tableEnd") + " must come right after the " + std::string("pointValue") +
                         " of the last point: a point has four fields",
                     "tableEnd");
        return card;
    }
    for (int field = FIRST_POINT_FIELD; field < endField && !reader.failed(); field += FIELDS_PER_POINT)
    {
        TableScattered3DPoint point;
        point.x = reader.real(field, "pointX", 0.0);
        point.y = reader.real(field + 1, "pointY", 0.0);
        point.z = reader.real(field + 2, "pointZ", 0.0);
        point.value = reader.real(field + 3, "pointValue", 0.0);
        card.points.push_back(point);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableScattered3DFormat::check(const TableScattered3DCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!std::isfinite(card.xOffset))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "xOffset"};
    if (!std::isfinite(card.yOffset))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "yOffset"};
    if (!std::isfinite(card.zOffset))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "zOffset"};
    if (!std::isfinite(card.valueOffset))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "valueOffset"};
    if (card.points.size() < MIN_POINTS)
        return CardProblem{"card.missing-field", "the table needs at least two points", "pointX"};
    for (const TableScattered3DPoint& point : card.points)
    {
        if (!isFinite(point))
            return CardProblem{"card.bad-value", "the coordinates and the values of a point must be finite numbers",
                               "pointX"};
    }
    return std::nullopt;
}

void TableScattered3DFormat::write(const TableScattered3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.xOffset, 0.0);
    writer.realUnlessDefault(3, card.yOffset, 0.0);
    writer.realUnlessDefault(4, card.zOffset, 0.0);
    writer.realUnlessDefault(5, card.valueOffset, 0.0);
    int field = FIRST_POINT_FIELD;
    for (const TableScattered3DPoint& point : card.points)
    {
        writer.real(field, point.x);
        writer.real(field + 1, point.y);
        writer.real(field + 2, point.z);
        writer.real(field + 3, point.value);
        field += FIELDS_PER_POINT;
    }
    writer.text(field, "EndTable");
}

std::uint32_t TableScattered3DTable::append(const TableScattered3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TableScattered3DTable::store(std::uint32_t row, const TableScattered3DCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TableScattered3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TableScattered3DTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
