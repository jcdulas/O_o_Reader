#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropTable.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> given(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

HeatSurfacePropTable::HeatSurfacePropTable() : m_area(notGiven()), m_diameter1(notGiven()), m_diameter2(notGiven()) {}

std::optional<HeatSurfacePropCard> HeatSurfacePropFormat::read(FieldReader& reader)
{
    HeatSurfacePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.area = reader.optionalReal(2, "area");
    card.diameter1 = reader.optionalReal(3, "diameter1");
    card.diameter2 = reader.optionalReal(4, "diameter2");
    // format reference: the default of D2 is D1.
    if (card.diameter1 && !card.diameter2)
        card.diameter2 = card.diameter1;
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatSurfacePropFormat::check(const HeatSurfacePropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.area && !(*card.area > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "area"};
    if (card.diameter1 && !(*card.diameter1 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "diameter1"};
    if (card.diameter2 && !(*card.diameter2 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "diameter2"};
    // D2 is stored resolved: with a D1 it holds the value the card means (D1 for a blank D2), without one it is absent.
    if (card.diameter2 && !card.diameter1)
        return CardProblem{"card.missing-field",
                           CardProblem::fieldNamed("diameter2") + " needs " + std::string(CardProblem::FIELD) +
                               " (a blank " + CardProblem::fieldNamed("diameter2") + " is " +
                               std::string(CardProblem::FIELD) + ")",
                           "diameter1"};
    if (card.diameter1 && !card.diameter2)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " must be given with " +
                               CardProblem::fieldNamed("diameter1") + " (a blank " + std::string(CardProblem::FIELD) +
                               " on the card is " + CardProblem::fieldNamed("diameter1") + ")",
                           "diameter2"};
    return std::nullopt;
}

void HeatSurfacePropFormat::write(const HeatSurfacePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.area)
        writer.real(2, *card.area);
    if (card.diameter1)
        writer.real(3, *card.diameter1);
    // D2 defaults to D1: it is written whenever it is there.
    if (card.diameter2)
        writer.real(4, *card.diameter2);
}

std::uint32_t HeatSurfacePropTable::append(const HeatSurfacePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_area.push(card.area.value_or(notGiven()));
    m_diameter1.push(card.diameter1.value_or(notGiven()));
    m_diameter2.push(card.diameter2.value_or(notGiven()));
    return row;
}

HeatSurfacePropCard HeatSurfacePropTable::card(std::uint32_t row) const
{
    HeatSurfacePropCard card;
    card.id = m_id.get(row);
    card.area = given(m_area.get(row));
    card.diameter1 = given(m_diameter1.get(row));
    card.diameter2 = given(m_diameter2.get(row));
    return card;
}

void HeatSurfacePropTable::store(std::uint32_t row, const HeatSurfacePropCard& card)
{
    m_id.set(row, card.id);
    m_area.set(row, card.area.value_or(notGiven()));
    m_diameter1.set(row, card.diameter1.value_or(notGiven()));
    m_diameter2.set(row, card.diameter2.value_or(notGiven()));
}

void HeatSurfacePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_area.keepRows(rows);
    m_diameter1.keepRows(rows);
    m_diameter2.keepRows(rows);
}

} // namespace oor::model
