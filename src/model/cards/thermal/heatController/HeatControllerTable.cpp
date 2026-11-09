#include "oor/model/cards/thermal/heatController/HeatControllerTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatControllerCard> HeatControllerFormat::read(FieldReader& reader)
{
    HeatControllerCard card;
    card.id = reader.requiredInteger(1, "id");
    card.sensorNode = reader.requiredInteger(2, "sensorNode");
    card.sensorForm = readKeyword(reader, 3, "sensorForm", HEAT_CONTROLLER_SENSOR_FORM_WORDS, SensorForm::Temperature);
    card.controllerKind = readKeyword(reader, 4, "controllerKind", HEAT_CONTROLLER_TYPE_WORDS, ControlType::Thermostat);
    card.lowerLimit = reader.requiredReal(5, "lowerLimit");
    card.upperLimit = reader.requiredReal(6, "upperLimit");
    card.controlLaw = reader.requiredInteger(7, "controlLaw");
    card.initialState = reader.real(8, "initialState", 0.0);
    card.timeStep = reader.real(9, "timeStep", 0.0);
    card.delayTime = reader.real(10, "delayTime", 0.0);
    card.timeConstant = reader.real(11, "timeConstant", 0.0);
    requireUnusedBlank(reader, {}, 11);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatControllerFormat::check(const HeatControllerCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.sensorNode, "sensorNode"))
        return problem;
    if (const auto problem = checkReal(card.lowerLimit, "lowerLimit", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.upperLimit, "upperLimit", RealRule::Finite))
        return problem;
    if (card.controlLaw < HEAT_CONTROLLER_MIN_CONTROL_LAW || card.controlLaw > HEAT_CONTROLLER_MAX_CONTROL_LAW)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 11",
                           "controlLaw"};
    if (card.initialState != 0.0 && card.initialState != 1.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0.0 or 1.0", "initialState"};
    if (const auto problem = checkReal(card.timeStep, "timeStep", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.delayTime, "delayTime", RealRule::NonNegative))
        return problem;
    return checkReal(card.timeConstant, "timeConstant", RealRule::NonNegative);
}

void HeatControllerFormat::write(const HeatControllerCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.sensorNode);
    writeKeywordUnlessDefault(writer, 3, HEAT_CONTROLLER_SENSOR_FORM_WORDS, card.sensorForm, SensorForm::Temperature);
    writeKeywordUnlessDefault(writer, 4, HEAT_CONTROLLER_TYPE_WORDS, card.controllerKind, ControlType::Thermostat);
    writer.real(5, card.lowerLimit);
    writer.real(6, card.upperLimit);
    writer.integer(7, card.controlLaw);
    writer.realUnlessDefault(8, card.initialState, 0.0);
    writer.realUnlessDefault(9, card.timeStep, 0.0);
    writer.realUnlessDefault(10, card.delayTime, 0.0);
    writer.realUnlessDefault(11, card.timeConstant, 0.0);
}

std::uint32_t HeatControllerTable::append(const HeatControllerCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HeatControllerTable::store(std::uint32_t row, const HeatControllerCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HeatControllerTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HeatControllerTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HeatControllerTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The sensor may be a scalar point that no SPOINT defines (a scalar element can define its points by naming them).
    visitId(visitor, "sensorNode", ReferenceTarget::ScalarOrGridPoint, m_cards.edit(row).sensorNode);
}

} // namespace oor::model
