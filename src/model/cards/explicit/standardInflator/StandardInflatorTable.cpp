#include "oor/model/cards/explicit/standardInflator/StandardInflatorTable.hpp"

namespace oor::model
{

namespace
{

// An INFLGAS defines the gas by itself, so the gas constants of the card stay blank. Without one the card gives the
// constants; the format reference asks for two of the four and the solver derives the others, so the card does not
// count them (as the EOSGAM does not).
std::optional<CardProblem> checkGas(const StandardInflatorCard& card)
{
    if (card.gasName.gas == 0)
        return std::nullopt;
    if (card.specificHeatVolume)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank when " +
                               CardProblem::fieldNamed("gasName") + " names an " +
                               CardProblem::cardNamed("InflatorGas"),
                           "specificHeatVolume"};
    if (card.gasConstant)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank when " +
                               CardProblem::fieldNamed("gasName") + " names an " +
                               CardProblem::cardNamed("InflatorGas"),
                           "gasConstant"};
    if (card.specificHeatPressure)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank when " +
                               CardProblem::fieldNamed("gasName") + " names an " +
                               CardProblem::cardNamed("InflatorGas"),
                           "specificHeatPressure"};
    return std::nullopt;
}

} // namespace

std::optional<StandardInflatorCard> StandardInflatorFormat::read(FieldReader& reader)
{
    StandardInflatorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.massFlowTableId = reader.requiredInteger(2, "massFlowTableId");
    card.temperatureTableId = reader.integer(3, "temperatureTableId", 0);
    card.temperature = reader.optionalReal(4, "temperature");
    card.gasName = readInflatorGasName(reader, 5, "gasName");
    card.specificHeatVolume = reader.optionalReal(6, "specificHeatVolume");
    card.gasConstant = reader.optionalReal(7, "gasConstant");
    card.specificHeatPressure = reader.optionalReal(8, "specificHeatPressure");
    card.matId = reader.integer(9, "matId", 0);
    if (reader.lastUsedField() > 9)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 9 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> StandardInflatorFormat::check(const StandardInflatorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.massFlowTableId, "massFlowTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.temperatureTableId, "temperatureTableId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.temperature, "temperature", RealRule::Positive))
        return problem;
    if (card.temperatureTableId == 0 && !card.temperature)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("temperature") + " is required",
            "temperatureTableId"};
    if (card.temperatureTableId != 0 && card.temperature)
        return CardProblem{"card.bad-value",
                           "give " + CardProblem::fieldNamed("temperatureTableId") + " or " +
                               std::string(CardProblem::FIELD) + ", not both",
                           "temperature"};
    if (const auto problem = checkInflatorGasName(card.gasName, "gasName"))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkGas(card))
        return problem;
    return checkOptionalId(card.matId, "matId");
}

void StandardInflatorFormat::write(const StandardInflatorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.massFlowTableId);
    writer.integerUnlessDefault(3, card.temperatureTableId, 0);
    if (card.temperature)
        writer.real(4, *card.temperature);
    writeInflatorGasName(writer, 5, card.gasName);
    if (card.specificHeatVolume)
        writer.real(6, *card.specificHeatVolume);
    if (card.gasConstant)
        writer.real(7, *card.gasConstant);
    if (card.specificHeatPressure)
        writer.real(8, *card.specificHeatPressure);
    writer.integerUnlessDefault(9, card.matId, 0);
}

std::uint32_t StandardInflatorTable::append(const StandardInflatorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void StandardInflatorTable::store(std::uint32_t row, const StandardInflatorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void StandardInflatorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void StandardInflatorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void StandardInflatorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    StandardInflatorCard& card = m_cards.edit(row);
    visitId(visitor, "massFlowTableId", ReferenceTarget::DynamicTable, card.massFlowTableId);
    visitId(visitor, "temperatureTableId", ReferenceTarget::DynamicTable, card.temperatureTableId);
    visitInflatorGasName(visitor, "gasName", card.gasName);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
