#include "oor/model/cards/explicit/coldGasInflator/ColdGasInflatorTable.hpp"

namespace oor::model
{

namespace
{

// An INFLGAS defines the gas by itself, so the gas constants of the card stay blank. Without one the card gives the
// constants; the format reference asks for two of the four and the solver derives the others, so the card does not
// count them (as the EOSGAM does not).
std::optional<CardProblem> checkGas(const ColdGasInflatorCard& card)
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

std::optional<ColdGasInflatorCard> ColdGasInflatorFormat::read(FieldReader& reader)
{
    ColdGasInflatorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tankVolume = reader.requiredReal(2, "tankVolume");
    card.initialPressure = reader.optionalReal(3, "initialPressure");
    card.initialTemperature = reader.requiredReal(4, "initialTemperature");
    card.initialMass = reader.optionalReal(5, "initialMass");
    card.gasName = readInflatorGasName(reader, 6, "gasName");
    card.specificHeatVolume = reader.optionalReal(7, "specificHeatVolume");
    card.gasConstant = reader.optionalReal(8, "gasConstant");
    card.specificHeatPressure = reader.optionalReal(9, "specificHeatPressure");
    if (reader.lastUsedField() > 9)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 9 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ColdGasInflatorFormat::check(const ColdGasInflatorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.tankVolume, "tankVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.initialPressure, "initialPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.initialTemperature, "initialTemperature", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.initialMass, "initialMass", RealRule::Positive))
        return problem;
    if (!card.initialPressure && !card.initialMass)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("initialMass") + " is required",
            "initialPressure"};
    if (const auto problem = checkInflatorGasName(card.gasName, "gasName"))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive))
        return problem;
    return checkGas(card);
}

void ColdGasInflatorFormat::write(const ColdGasInflatorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.tankVolume);
    if (card.initialPressure)
        writer.real(3, *card.initialPressure);
    writer.real(4, card.initialTemperature);
    if (card.initialMass)
        writer.real(5, *card.initialMass);
    writeInflatorGasName(writer, 6, card.gasName);
    if (card.specificHeatVolume)
        writer.real(7, *card.specificHeatVolume);
    if (card.gasConstant)
        writer.real(8, *card.gasConstant);
    if (card.specificHeatPressure)
        writer.real(9, *card.specificHeatPressure);
}

std::uint32_t ColdGasInflatorTable::append(const ColdGasInflatorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ColdGasInflatorTable::store(std::uint32_t row, const ColdGasInflatorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ColdGasInflatorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ColdGasInflatorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ColdGasInflatorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitInflatorGasName(visitor, "gasName", m_cards.edit(row).gasName);
}

} // namespace oor::model
