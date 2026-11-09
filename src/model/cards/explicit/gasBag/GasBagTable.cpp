#include "oor/model/cards/explicit/gasBag/GasBagTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<GasBagCard> GasBagFormat::read(FieldReader& reader)
{
    GasBagCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceId = reader.requiredInteger(2, "surfaceId");
    card.trigger = readKeyword(reader, 3, "trigger", GAS_BAG_TRIGGER_WORDS, GasBagTrigger::Time);
    card.triggerOffset = reader.real(4, "triggerOffset", 0.0);
    card.porosityId = reader.integer(5, "porosityId", 0);
    card.inflatorId = reader.integer(6, "inflatorId", 0);
    card.heatTransferId = reader.integer(7, "heatTransferId", 0);
    card.initialGasId = reader.integer(8, "initialGasId", 0);
    card.specificHeat = readSourcedValue(reader, GAS_BAG_SPECIFIC_HEAT_LAYOUT);
    if (card.specificHeat.source == ValueSource::Constant && reader.isBlank(26))
        reader.error("card.missing-field", std::string("specificHeatPressure") + " is required",
                     "specificHeatPressure");
    card.gasConstant = reader.requiredReal(27, "gasConstant");
    card.environmentPressure = reader.requiredReal(28, "environmentPressure");
    card.reverse = readKeyword(reader, 30, "reverseNormals", SWITCH_WORDS, Switch::On);
    card.check = readKeyword(reader, 31, "checkNormals", SWITCH_WORDS, Switch::On);
    card.initialPressure = reader.real(32, "initialPressure", card.environmentPressure);
    card.initialTemperature = reader.requiredReal(33, "initialTemperature");
    card.environmentTemperature = reader.real(34, "environmentTemperature", card.initialTemperature);
    // Fields 9 to 24, 29 and everything after field 34 are blank cells of the layout.
    requireOnlyFields(reader, {1, 2, 3, 4, 5, 6, 7, 8, 25, 26, 27, 28, 30, 31, 32, 33, 34});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagFormat::check(const GasBagCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.surfaceId, "surfaceId"))
        return problem;
    if (const auto problem = checkReal(card.triggerOffset, "triggerOffset", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.porosityId, "porosityId"))
        return problem;
    if (const auto problem = checkOptionalId(card.inflatorId, "inflatorId"))
        return problem;
    if (const auto problem = checkOptionalId(card.heatTransferId, "heatTransferId"))
        return problem;
    if (const auto problem = checkOptionalId(card.initialGasId, "initialGasId"))
        return problem;
    if (const auto problem = checkSourcedValue(card.specificHeat, GAS_BAG_SPECIFIC_HEAT_LAYOUT, RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.environmentPressure, "environmentPressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.initialPressure, "initialPressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.initialTemperature, "initialTemperature", RealRule::Positive))
        return problem;
    return checkReal(card.environmentTemperature, "environmentTemperature", RealRule::Positive);
}

void GasBagFormat::write(const GasBagCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.surfaceId);
    writeKeywordUnlessDefault(writer, 3, GAS_BAG_TRIGGER_WORDS, card.trigger, GasBagTrigger::Time);
    writer.realUnlessDefault(4, card.triggerOffset, 0.0);
    writer.integerUnlessDefault(5, card.porosityId, 0);
    writer.integerUnlessDefault(6, card.inflatorId, 0);
    writer.integerUnlessDefault(7, card.heatTransferId, 0);
    writer.integerUnlessDefault(8, card.initialGasId, 0);
    writeSourcedValue(writer, GAS_BAG_SPECIFIC_HEAT_LAYOUT, card.specificHeat);
    writer.real(27, card.gasConstant);
    writer.real(28, card.environmentPressure);
    writeKeywordUnlessDefault(writer, 30, SWITCH_WORDS, card.reverse, Switch::On);
    writeKeywordUnlessDefault(writer, 31, SWITCH_WORDS, card.check, Switch::On);
    writer.real(32, card.initialPressure); // these two defaults depend on PENV and TINIT: always written
    writer.real(33, card.initialTemperature);
    writer.real(34, card.environmentTemperature);
}

std::uint32_t GasBagTable::append(const GasBagCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagTable::store(std::uint32_t row, const GasBagCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GasBagCard& card = m_cards.edit(row);
    visitId(visitor, "surfaceId", ReferenceTarget::ContactSurface, card.surfaceId);
    visitId(visitor, "porosityId", ReferenceTarget::Leakage, card.porosityId);
    visitId(visitor, "inflatorId", ReferenceTarget::InflatorSet, card.inflatorId);
    visitId(visitor, "heatTransferId", ReferenceTarget::HeatLossSet, card.heatTransferId);
    visitId(visitor, "initialGasId", ReferenceTarget::InitialGas, card.initialGasId);
}

} // namespace oor::model
