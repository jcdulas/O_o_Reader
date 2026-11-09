#include "oor/model/cards/thermal/axisymFluid2/AxisymFluid2Table.hpp"

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

AxisymFluid2Table::AxisymFluid2Table() : m_density(notGiven()), m_bulkModulus(notGiven()) {}

std::optional<AxisymFluid2Card> AxisymFluid2Format::read(FieldReader& reader)
{
    AxisymFluid2Card card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < AxisymFluid2Card::RING_COUNT; ++index)
    {
        const std::string name = "fluidPointId" + std::to_string(index + 1);
        card.rings[index] = reader.requiredInteger(2 + static_cast<int>(index), name.c_str());
    }
    card.density = reader.optionalReal(4, "density");
    card.bulkModulus = reader.optionalReal(5, "bulkModulus");
    requireUnusedBlank(reader, {}, 5);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluid2Format::check(const AxisymFluid2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    for (const base::EntityId ring : card.rings)
    {
        if (ring <= 0 || ring > MAX_RING_ID)
            return CardProblem{"card.bad-id", "a fluid point ID must be between 1 and 499999", "fluidPointId"};
    }
    if (card.rings[0] == card.rings[1])
        return CardProblem{"card.bad-value", "the rings of an element are different", "fluidPointId"};
    if (card.density && !(*card.density > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "density"};
    return std::nullopt;
}

void AxisymFluid2Format::write(const AxisymFluid2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < AxisymFluid2Card::RING_COUNT; ++index)
        writer.integer(2 + static_cast<int>(index), card.rings[index]);
    if (card.density)
        writer.real(4, *card.density);
    if (card.bulkModulus)
        writer.real(5, *card.bulkModulus);
}

std::uint32_t AxisymFluid2Table::append(const AxisymFluid2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_rings.push(card.rings);
    m_density.push(card.density.value_or(notGiven()));
    m_bulkModulus.push(card.bulkModulus.value_or(notGiven()));
    return row;
}

AxisymFluid2Card AxisymFluid2Table::card(std::uint32_t row) const
{
    AxisymFluid2Card card;
    card.id = m_id.get(row);
    card.rings = m_rings.get(row);
    card.density = given(m_density.get(row));
    card.bulkModulus = given(m_bulkModulus.get(row));
    return card;
}

void AxisymFluid2Table::store(std::uint32_t row, const AxisymFluid2Card& card)
{
    m_id.set(row, card.id);
    m_rings.set(row, card.rings);
    m_density.set(row, card.density.value_or(notGiven()));
    m_bulkModulus.set(row, card.bulkModulus.value_or(notGiven()));
}

void AxisymFluid2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_rings.keepRows(rows);
    m_density.keepRows(rows);
    m_bulkModulus.keepRows(rows);
}

} // namespace oor::model
