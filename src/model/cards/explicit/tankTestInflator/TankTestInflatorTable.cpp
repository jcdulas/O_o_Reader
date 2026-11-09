#include "oor/model/cards/explicit/tankTestInflator/TankTestInflatorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

CardProblem unusedField(const char* name)
{
    return CardProblem{"card.bad-value",
                       std::string(CardProblem::FIELD) + " is used only with " + CardProblem::fieldNamed("method") +
                           " = INFPRES and must be blank",
                       name};
}

CardProblem missingField(const char* name)
{
    return CardProblem{
        "card.missing-field",
        std::string(CardProblem::FIELD) + " is required with " + CardProblem::fieldNamed("method") + " = INFPRES",
        name};
}

// With METHOD = AVTEMP the fields of the inflator pressure method stay blank (SFTP and SFIP at their default).
std::optional<CardProblem> checkAverageTemperature(const TankTestInflatorCard& card)
{
    if (card.endPressure)
        return unusedField("endPressure");
    if (card.initialTemperature)
        return unusedField("initialTemperature");
    if (card.endTemperature)
        return unusedField("endTemperature");
    if (card.inflatorPressureTableId != 0)
        return unusedField("inflatorPressureTableId");
    if (card.inflatorPressure)
        return unusedField("inflatorPressure");
    if (card.inflatorTemperature)
        return unusedField("inflatorTemperature");
    if (card.inflatorArea)
        return unusedField("inflatorArea");
    if (card.tankPressureScale != 1.0)
        return unusedField("tankPressureScale");
    if (card.inflatorPressureScale != 1.0)
        return unusedField("inflatorPressureScale");
    return std::nullopt;
}

// With METHOD = INFPRES the format reference marks ENDPRES, INITTEMP, ENDTEMP, IPTABLE, INFLPRES and INFLAREA required.
std::optional<CardProblem> checkInflatorPressure(const TankTestInflatorCard& card)
{
    if (!card.endPressure)
        return missingField("endPressure");
    if (!card.initialTemperature)
        return missingField("initialTemperature");
    if (!card.endTemperature)
        return missingField("endTemperature");
    if (card.inflatorPressureTableId == 0)
        return missingField("inflatorPressureTableId");
    if (!card.inflatorPressure)
        return missingField("inflatorPressure");
    if (!card.inflatorArea)
        return missingField("inflatorArea");
    if (const auto problem = checkReal(*card.endPressure, "endPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(*card.initialTemperature, "initialTemperature", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(*card.endTemperature, "endTemperature", RealRule::Positive))
        return problem;
    if (const auto problem = checkRequiredId(card.inflatorPressureTableId, "inflatorPressureTableId"))
        return problem;
    if (const auto problem = checkReal(*card.inflatorPressure, "inflatorPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.inflatorTemperature, "inflatorTemperature", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(*card.inflatorArea, "inflatorArea", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.tankPressureScale, "tankPressureScale", RealRule::Positive))
        return problem;
    return checkReal(card.inflatorPressureScale, "inflatorPressureScale", RealRule::Positive);
}

} // namespace

std::optional<TankTestInflatorCard> TankTestInflatorFormat::read(FieldReader& reader)
{
    TankTestInflatorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.method = readRequiredKeyword<TankTestInflatorMethod>(reader, 2, "method", TANK_TEST_INFLATOR_METHOD_WORDS);
    card.tankPressureTableId = reader.requiredInteger(3, "tankPressureTableId");
    card.tankVolume = reader.requiredReal(4, "tankVolume");
    card.inflatorMass = reader.requiredReal(5, "inflatorMass");
    card.initialPressure = reader.requiredReal(6, "initialPressure");
    card.endPressure = reader.optionalReal(7, "endPressure");
    card.initialTemperature = reader.optionalReal(8, "initialTemperature");
    card.endTemperature = reader.optionalReal(9, "endTemperature");
    card.gammaCoefficient = reader.optionalReal(10, "gammaCoefficient");
    card.specificHeatVolume = reader.optionalReal(11, "specificHeatVolume");
    card.gasConstant = reader.requiredReal(12, "gasConstant");
    card.specificHeatPressure = reader.optionalReal(13, "specificHeatPressure");
    card.inflatorPressureTableId = reader.integer(14, "inflatorPressureTableId", 0);
    card.inflatorPressure = reader.optionalReal(15, "inflatorPressure");
    if (const std::string_view word = reader.text(16); !word.empty() && !(word == "AtmosphereUnit"))
        card.inflatorTemperature = reader.optionalReal(16, "inflatorTemperature");
    card.inflatorArea = reader.optionalReal(17, "inflatorArea");
    card.tankPressureScale = reader.real(18, "tankPressureScale", 1.0);
    card.inflatorPressureScale = reader.real(19, "inflatorPressureScale", 1.0);
    if (reader.lastUsedField() > 19)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 19 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TankTestInflatorFormat::check(const TankTestInflatorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.tankPressureTableId, "tankPressureTableId"))
        return problem;
    if (const auto problem = checkReal(card.tankVolume, "tankVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.inflatorMass, "inflatorMass", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.initialPressure, "initialPressure", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.gammaCoefficient, "gammaCoefficient", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive))
        return problem;
    if (card.method == TankTestInflatorMethod::AverageTemperature)
        return checkAverageTemperature(card);
    return checkInflatorPressure(card);
}

void TankTestInflatorFormat::write(const TankTestInflatorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, TANK_TEST_INFLATOR_METHOD_WORDS, card.method);
    writer.integer(3, card.tankPressureTableId);
    writer.real(4, card.tankVolume);
    writer.real(5, card.inflatorMass);
    writer.real(6, card.initialPressure);
    if (card.endPressure)
        writer.real(7, *card.endPressure);
    if (card.initialTemperature)
        writer.real(8, *card.initialTemperature);
    if (card.endTemperature)
        writer.real(9, *card.endTemperature);
    if (card.gammaCoefficient)
        writer.real(10, *card.gammaCoefficient);
    if (card.specificHeatVolume)
        writer.real(11, *card.specificHeatVolume);
    writer.real(12, card.gasConstant);
    if (card.specificHeatPressure)
        writer.real(13, *card.specificHeatPressure);
    writer.integerUnlessDefault(14, card.inflatorPressureTableId, 0);
    if (card.inflatorPressure)
        writer.real(15, *card.inflatorPressure);
    if (card.inflatorTemperature)
        writer.real(16, *card.inflatorTemperature); // a blank is ATM
    if (card.inflatorArea)
        writer.real(17, *card.inflatorArea);
    writer.realUnlessDefault(18, card.tankPressureScale, 1.0);
    writer.realUnlessDefault(19, card.inflatorPressureScale, 1.0);
}

std::uint32_t TankTestInflatorTable::append(const TankTestInflatorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TankTestInflatorTable::store(std::uint32_t row, const TankTestInflatorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TankTestInflatorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TankTestInflatorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TankTestInflatorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    TankTestInflatorCard& card = m_cards.edit(row);
    visitId(visitor, "tankPressureTableId", ReferenceTarget::DynamicTable, card.tankPressureTableId);
    visitId(visitor, "inflatorPressureTableId", ReferenceTarget::DynamicTable, card.inflatorPressureTableId);
}

} // namespace oor::model
