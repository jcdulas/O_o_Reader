#include "oor/model/cards/thermal/axisymFluid3/AxisymFluid3Table.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

constexpr base::EntityId MAX_RING_ID = 499999;

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

AxisymFluid3Table::AxisymFluid3Table() : m_density(notGiven()), m_bulkModulus(notGiven()) {}

std::optional<AxisymFluid3Card> AxisymFluid3Format::read(FieldReader& reader)
{
    AxisymFluid3Card card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < AxisymFluid3Card::RING_COUNT; ++index)
    {
        const std::string name = "fluidPointId" + std::to_string(index + 1);
        card.rings[index] = reader.requiredInteger(2 + static_cast<int>(index), name.c_str());
    }
    card.density = reader.optionalReal(5, "density");
    card.bulkModulus = reader.optionalReal(6, "bulkModulus");
    requireUnusedBlank(reader, {}, 6);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluid3Format::check(const AxisymFluid3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    for (const base::EntityId ring : card.rings)
    {
        if (ring <= 0 || ring > MAX_RING_ID)
            return CardProblem{"card.bad-id", "a fluid point ID must be between 1 and 499999", "fluidPointId"};
    }
    for (std::size_t first = 0; first < AxisymFluid3Card::RING_COUNT; ++first)
    {
        for (std::size_t second = first + 1; second < AxisymFluid3Card::RING_COUNT; ++second)
        {
            if (card.rings[first] == card.rings[second])
                return CardProblem{"card.bad-value", "the rings of an element are different", "fluidPointId"};
        }
    }
    if (card.density && !(*card.density > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "density"};
    return std::nullopt;
}

void AxisymFluid3Format::write(const AxisymFluid3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < AxisymFluid3Card::RING_COUNT; ++index)
        writer.integer(2 + static_cast<int>(index), card.rings[index]);
    if (card.density)
        writer.real(5, *card.density);
    if (card.bulkModulus)
        writer.real(6, *card.bulkModulus);
}

std::uint32_t AxisymFluid3Table::append(const AxisymFluid3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_rings.push(card.rings);
    m_density.push(card.density.value_or(notGiven()));
    m_bulkModulus.push(card.bulkModulus.value_or(notGiven()));
    return row;
}

AxisymFluid3Card AxisymFluid3Table::card(std::uint32_t row) const
{
    AxisymFluid3Card card;
    card.id = m_id.get(row);
    card.rings = m_rings.get(row);
    card.density = given(m_density.get(row));
    card.bulkModulus = given(m_bulkModulus.get(row));
    return card;
}

void AxisymFluid3Table::store(std::uint32_t row, const AxisymFluid3Card& card)
{
    m_id.set(row, card.id);
    m_rings.set(row, card.rings);
    m_density.set(row, card.density.value_or(notGiven()));
    m_bulkModulus.set(row, card.bulkModulus.value_or(notGiven()));
}

void AxisymFluid3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_rings.keepRows(rows);
    m_density.keepRows(rows);
    m_bulkModulus.keepRows(rows);
}

} // namespace oor::model
