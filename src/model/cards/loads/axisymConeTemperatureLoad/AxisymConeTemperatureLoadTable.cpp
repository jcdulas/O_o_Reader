#include "oor/model/cards/loads/axisymConeTemperatureLoad/AxisymConeTemperatureLoadTable.hpp"

namespace oor::model
{

namespace
{

// The group whose four fields start at `first` (1 for the first, 5 for the second).
AxisymConeTemperatureLoadCard readGroup(FieldReader& reader, int first, const char* const (&names)[4])
{
    AxisymConeTemperatureLoadCard card;
    card.id = reader.requiredInteger(first, names[0]);
    card.ringId = reader.requiredInteger(first + 1, names[1]);
    card.phi = reader.requiredReal(first + 2, names[2]);
    card.temperature = reader.requiredReal(first + 3, names[3]);
    if (!reader.failed())
        reader.report(AxisymConeTemperatureLoadFormat::check(card));
    return card;
}

} // namespace

std::vector<AxisymConeTemperatureLoadCard> AxisymConeTemperatureLoadFormat::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[4] = {"id1", "ringId1", "phiAngle1", "temperature1"};
    static constexpr const char* NAMES_SECOND[4] = {"id2", "ringId2", "phiAngle2", "temperature2"};
    std::vector<AxisymConeTemperatureLoadCard> cards;
    cards.push_back(readGroup(reader, 1, NAMES_FIRST));
    // The second group is there as soon as one of its fields is; all of them are then required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7) || !reader.isBlank(8))
        cards.push_back(readGroup(reader, 5, NAMES_SECOND));
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "the card defines at most two temperatures", "temperature2");
    return cards;
}

std::optional<CardProblem> AxisymConeTemperatureLoadFormat::check(const AxisymConeTemperatureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    if (card.ringId <= 0)
        return CardProblem{"card.bad-id", "the ring ID must be a positive integer", "ringId"};
    return std::nullopt;
}

void AxisymConeTemperatureLoadFormat::write(const AxisymConeTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    writer.real(3, card.phi);
    writer.real(4, card.temperature);
}

std::uint32_t AxisymConeTemperatureLoadTable::append(const AxisymConeTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_phi.push(card.phi);
    m_temperature.push(card.temperature);
    return row;
}

AxisymConeTemperatureLoadCard AxisymConeTemperatureLoadTable::card(std::uint32_t row) const
{
    return AxisymConeTemperatureLoadCard{m_id.get(row), m_ringId.get(row), m_phi.get(row), m_temperature.get(row)};
}

void AxisymConeTemperatureLoadTable::store(std::uint32_t row, const AxisymConeTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_phi.set(row, card.phi);
    m_temperature.set(row, card.temperature);
}

void AxisymConeTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_phi.keepRows(rows);
    m_temperature.keepRows(rows);
}

void AxisymConeTemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ring is a RINGAX, a point of the node ID space.
    m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, m_ringId.get(row)));
}

} // namespace oor::model
