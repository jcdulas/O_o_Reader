#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

std::optional<RotordynamicParamsCard> RotordynamicParamsFormat::read(FieldReader& reader)
{
    RotordynamicParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view syncText = reader.text(2);
    if (syncText.empty())
        card.synchronous = RotorSynchronisation::NotGiven;
    else if ((syncText == "SynchronousWhirl"))
        card.synchronous = RotorSynchronisation::Sync;
    else if ((syncText == "AsynchronousWhirl"))
        card.synchronous = RotorSynchronisation::Async;
    else
        reader.error(
            "card.bad-value",
            std::string("synchronous") + " must be SYNC, ASYNC or blank, found \"" + std::string(syncText) + "\"",
            "synchronous");
    card.referenceRotorId = reader.requiredInteger(3, "referenceRotorId");
    const std::string_view unitText = reader.text(4);
    if (unitText.empty())
        reader.error("card.missing-field", std::string("speedUnit") + " is required (RPM or FREQ)", "speedUnit");
    else if ((unitText == "RevolutionsPerMinute"))
        card.speedUnit = RotorSpeedUnit::Rpm;
    else if ((unitText == "Frequency"))
        card.speedUnit = RotorSpeedUnit::Freq;
    else
        reader.error("card.bad-value",
                     std::string("speedUnit") + " must be RPM or FREQ, found \"" + std::string(unitText) + "\"",
                     "speedUnit");
    card.speedLow = reader.real(5, "speedLow", 0.0);
    card.speedHigh = reader.real(6, "speedHigh", 99999.0);
    card.speed = reader.real(7, "speed", 0.0);
    card.rotorSuperelementId = reader.integer(8, "rotorSuperelementId", 0);
    card.viscousDampingFrequency = reader.real(9, "viscousDampingFrequency", 0.0);
    card.structuralDampingFrequency = reader.real(10, "structuralDampingFrequency", 0.0);
    card.hybridDampingFrequency = reader.real(11, "hybridDampingFrequency", 0.0);
    if (reader.lastUsedField() > 11)
        reader.error(
            "card.bad-value",
            "an " + std::string(reader.cardName()) + " has no field beyond " + std::string("hybridDampingFrequency"),
            "hybridDampingFrequency");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotordynamicParamsFormat::check(const RotordynamicParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.referenceRotorId <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "referenceRotorId"};
    if (!std::isfinite(card.speedLow) || !std::isfinite(card.speedHigh) || !std::isfinite(card.speed))
        return CardProblem{"card.bad-value", "the speeds must be finite numbers", "speedLow"};
    if (card.rotorSuperelementId < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "rotorSuperelementId"};
    if (!std::isfinite(card.viscousDampingFrequency) || !std::isfinite(card.structuralDampingFrequency) ||
        !std::isfinite(card.hybridDampingFrequency))
        return CardProblem{"card.bad-value", "the whirl frequencies must be finite numbers", "viscousDampingFrequency"};
    return std::nullopt;
}

void RotordynamicParamsFormat::write(const RotordynamicParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.synchronous != RotorSynchronisation::NotGiven)
        writer.text(2, card.synchronous == RotorSynchronisation::Sync ? "SynchronousWhirl" : "AsynchronousWhirl");
    writer.integer(3, card.referenceRotorId);
    writer.text(4, card.speedUnit == RotorSpeedUnit::Freq ? "Frequency" : "RevolutionsPerMinute");
    writer.realUnlessDefault(5, card.speedLow, 0.0);
    writer.realUnlessDefault(6, card.speedHigh, 99999.0);
    writer.realUnlessDefault(7, card.speed, 0.0);
    writer.integerUnlessDefault(8, card.rotorSuperelementId, 0);
    writer.realUnlessDefault(9, card.viscousDampingFrequency, 0.0);
    writer.realUnlessDefault(10, card.structuralDampingFrequency, 0.0);
    writer.realUnlessDefault(11, card.hybridDampingFrequency, 0.0);
}

void RotordynamicParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId referenceRotorId = m_referenceRotorId.get(row);
    visitRotor(visitor, "referenceRotorId", referenceRotorId, m_rotorSuperelementId.get(row));
    m_referenceRotorId.set(row, referenceRotorId);
}

std::uint32_t RotordynamicParamsTable::append(const RotordynamicParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_synchronous.push(static_cast<std::uint8_t>(card.synchronous));
    m_referenceRotorId.push(card.referenceRotorId);
    m_speedUnit.push(static_cast<std::uint8_t>(card.speedUnit));
    m_speedLow.push(card.speedLow);
    m_speedHigh.push(card.speedHigh);
    m_speed.push(card.speed);
    m_rotorSuperelementId.push(card.rotorSuperelementId);
    m_viscousDampingFrequency.push(card.viscousDampingFrequency);
    m_structuralDampingFrequency.push(card.structuralDampingFrequency);
    m_hybridDampingFrequency.push(card.hybridDampingFrequency);
    return row;
}

RotordynamicParamsCard RotordynamicParamsTable::card(std::uint32_t row) const
{
    RotordynamicParamsCard card;
    card.id = m_id.get(row);
    card.synchronous = static_cast<RotorSynchronisation>(m_synchronous.get(row));
    card.referenceRotorId = m_referenceRotorId.get(row);
    card.speedUnit = static_cast<RotorSpeedUnit>(m_speedUnit.get(row));
    card.speedLow = m_speedLow.get(row);
    card.speedHigh = m_speedHigh.get(row);
    card.speed = m_speed.get(row);
    card.rotorSuperelementId = m_rotorSuperelementId.get(row);
    card.viscousDampingFrequency = m_viscousDampingFrequency.get(row);
    card.structuralDampingFrequency = m_structuralDampingFrequency.get(row);
    card.hybridDampingFrequency = m_hybridDampingFrequency.get(row);
    return card;
}

void RotordynamicParamsTable::store(std::uint32_t row, const RotordynamicParamsCard& card)
{
    m_id.set(row, card.id);
    m_synchronous.set(row, static_cast<std::uint8_t>(card.synchronous));
    m_referenceRotorId.set(row, card.referenceRotorId);
    m_speedUnit.set(row, static_cast<std::uint8_t>(card.speedUnit));
    m_speedLow.set(row, card.speedLow);
    m_speedHigh.set(row, card.speedHigh);
    m_speed.set(row, card.speed);
    m_rotorSuperelementId.set(row, card.rotorSuperelementId);
    m_viscousDampingFrequency.set(row, card.viscousDampingFrequency);
    m_structuralDampingFrequency.set(row, card.structuralDampingFrequency);
    m_hybridDampingFrequency.set(row, card.hybridDampingFrequency);
}

void RotordynamicParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_synchronous.keepRows(rows);
    m_referenceRotorId.keepRows(rows);
    m_speedUnit.keepRows(rows);
    m_speedLow.keepRows(rows);
    m_speedHigh.keepRows(rows);
    m_speed.keepRows(rows);
    m_rotorSuperelementId.keepRows(rows);
    m_viscousDampingFrequency.keepRows(rows);
    m_structuralDampingFrequency.keepRows(rows);
    m_hybridDampingFrequency.keepRows(rows);
}

} // namespace oor::model
