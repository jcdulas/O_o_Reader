#include "oor/model/cards/thermal/axisymFluidBoundaryList/AxisymFluidBoundaryListTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

AxisymFluidBoundaryListTable::AxisymFluidBoundaryListTable() : m_density(notGiven()) {}

std::optional<AxisymFluidBoundaryListCard> AxisymFluidBoundaryListFormat::read(FieldReader& reader)
{
    AxisymFluidBoundaryListCard card;
    card.density = reader.optionalReal(1, "density");
    const int last = reader.lastUsedField();
    for (int field = 2; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Axis"))
            card.points.push_back(AXIS_POINT);
        else if ((reader.text(field) == "LocalAxis"))
            card.points.push_back(LAST_AXIS_POINT);
        else
            card.points.push_back(reader.requiredInteger(field, "fluidPointId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluidBoundaryListFormat::check(const AxisymFluidBoundaryListCard& card)
{
    if (card.density && !(*card.density >= 0.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " cannot be negative (0.0 makes no boundary terms)",
                           "density"};
    if (card.points.empty())
        return CardProblem{"card.missing-field", "the card needs at least one fluid point", "fluidPointId"};
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const base::EntityId point = card.points[index];
        if (point == AXIS_POINT && index != 0)
            return CardProblem{"card.bad-value", "AXIS can only be the first point (use LAXIS for the last)",
                               "fluidPointId"};
        if (point == LAST_AXIS_POINT && index + 1 != card.points.size())
            return CardProblem{"card.bad-value", "LAXIS can only be the last point (use AXIS for the first)",
                               "fluidPointId"};
        if (point == 0 || point < LAST_AXIS_POINT)
            return CardProblem{"card.bad-id", "a fluid point ID must be a positive integer", "fluidPointId"};
    }
    return std::nullopt;
}

void AxisymFluidBoundaryListFormat::write(const AxisymFluidBoundaryListCard& card, FieldWriter& writer)
{
    if (card.density)
        writer.real(1, *card.density);
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const int field = 2 + static_cast<int>(index);
        if (card.points[index] == AXIS_POINT)
            writer.text(field, "Axis");
        else if (card.points[index] == LAST_AXIS_POINT)
            writer.text(field, "LocalAxis");
        else
            writer.integer(field, card.points[index]);
    }
}

std::uint32_t AxisymFluidBoundaryListTable::append(const AxisymFluidBoundaryListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_density.push(card.density.value_or(notGiven()));
    m_points.push(card.points);
    return row;
}

AxisymFluidBoundaryListCard AxisymFluidBoundaryListTable::card(std::uint32_t row) const
{
    AxisymFluidBoundaryListCard card;
    if (const double density = m_density.get(row); !std::isnan(density))
        card.density = density;
    const auto points = m_points.get(row);
    card.points.assign(points.begin(), points.end());
    return card;
}

void AxisymFluidBoundaryListTable::store(std::uint32_t row, const AxisymFluidBoundaryListCard& card)
{
    m_density.set(row, card.density.value_or(notGiven()));
    m_points.set(row, card.points);
}

void AxisymFluidBoundaryListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_density.keepRows(rows);
    m_points.keepRows(rows);
}

} // namespace oor::model
