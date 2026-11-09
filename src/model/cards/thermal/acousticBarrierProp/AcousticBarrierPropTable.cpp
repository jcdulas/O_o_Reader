#include "oor/model/cards/thermal/acousticBarrierProp/AcousticBarrierPropTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

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

AcousticBarrierPropTable::AcousticBarrierPropTable()
    : m_resonanceFrequency(notGiven()), m_resonanceStiffness(notGiven())
{
}

std::optional<AcousticBarrierPropCard> AcousticBarrierPropFormat::read(FieldReader& reader)
{
    AcousticBarrierPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.backingMass = reader.requiredReal(2, "backingMass");
    card.septumMass = reader.requiredReal(3, "septumMass");
    card.resonanceFrequency = reader.optionalReal(4, "resonanceFrequency");
    card.resonanceStiffness = reader.optionalReal(5, "resonanceStiffness");
    requireUnusedBlank(reader, {}, 5);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticBarrierPropFormat::check(const AcousticBarrierPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (!(card.backingMass > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "backingMass"};
    if (!(card.septumMass > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "septumMass"};
    // FRESON or KRESON, not both.
    if (card.resonanceFrequency && card.resonanceStiffness)
        return CardProblem{"card.bad-value",
                           "give " + CardProblem::fieldNamed("resonanceFrequency") + " or " +
                               std::string(CardProblem::FIELD) + ", not both",
                           "resonanceStiffness"};
    if (!card.resonanceFrequency && !card.resonanceStiffness)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("resonanceStiffness") + " is required",
            "resonanceFrequency"};
    if (card.resonanceFrequency && !(*card.resonanceFrequency > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "resonanceFrequency"};
    if (card.resonanceStiffness && !(*card.resonanceStiffness > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "resonanceStiffness"};
    return std::nullopt;
}

void AcousticBarrierPropFormat::write(const AcousticBarrierPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.backingMass);
    writer.real(3, card.septumMass);
    if (card.resonanceFrequency)
        writer.real(4, *card.resonanceFrequency);
    if (card.resonanceStiffness)
        writer.real(5, *card.resonanceStiffness);
}

std::uint32_t AcousticBarrierPropTable::append(const AcousticBarrierPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_backingMass.push(card.backingMass);
    m_septumMass.push(card.septumMass);
    m_resonanceFrequency.push(card.resonanceFrequency.value_or(notGiven()));
    m_resonanceStiffness.push(card.resonanceStiffness.value_or(notGiven()));
    return row;
}

AcousticBarrierPropCard AcousticBarrierPropTable::card(std::uint32_t row) const
{
    AcousticBarrierPropCard card;
    card.id = m_id.get(row);
    card.backingMass = m_backingMass.get(row);
    card.septumMass = m_septumMass.get(row);
    card.resonanceFrequency = given(m_resonanceFrequency.get(row));
    card.resonanceStiffness = given(m_resonanceStiffness.get(row));
    return card;
}

void AcousticBarrierPropTable::store(std::uint32_t row, const AcousticBarrierPropCard& card)
{
    m_id.set(row, card.id);
    m_backingMass.set(row, card.backingMass);
    m_septumMass.set(row, card.septumMass);
    m_resonanceFrequency.set(row, card.resonanceFrequency.value_or(notGiven()));
    m_resonanceStiffness.set(row, card.resonanceStiffness.value_or(notGiven()));
}

void AcousticBarrierPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_backingMass.keepRows(rows);
    m_septumMass.keepRows(rows);
    m_resonanceFrequency.keepRows(rows);
    m_resonanceStiffness.keepRows(rows);
}

} // namespace oor::model
