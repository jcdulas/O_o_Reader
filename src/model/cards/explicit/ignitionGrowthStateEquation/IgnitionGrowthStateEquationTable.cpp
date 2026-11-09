#include "oor/model/cards/explicit/ignitionGrowthStateEquation/IgnitionGrowthStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// The problem with one of the constants of the equation: a value must be a finite number, and a blank one is missing
// when the card does not take its data from the database.
std::optional<CardProblem> checkConstant(const std::optional<double>& value, const char* field, bool required)
{
    if (value)
        return checkReal(*value, field, RealRule::Finite);
    if (required)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required unless " +
                               CardProblem::fieldNamed("explosiveDatabase") + " names an explosive of the database",
                           field};
    return std::nullopt;
}

} // namespace

std::optional<IgnitionGrowthStateEquationCard> IgnitionGrowthStateEquationFormat::read(FieldReader& reader)
{
    IgnitionGrowthStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.inputUnits =
        readOptionalKeyword<IgnitionGrowthStateEquationUnitSystem>(reader, 2, "inputUnits", IGNITION_GROWTH_UNIT_WORDS);
    card.explosiveDatabase = readKeyword(reader, 3, "explosiveDatabase", IGNITION_GROWTH_DATABASE_WORDS,
                                         IgnitionGrowthStateEquationDatabase::NotUsed);
    card.outputUnits = readOptionalKeyword<IgnitionGrowthStateEquationUnitSystem>(reader, 4, "outputUnits",
                                                                                  IGNITION_GROWTH_UNIT_WORDS);
    requireBlankFields(reader, {5, 6, 7, 8, 9}, 29);
    card.unreactedA = reader.optionalReal(10, "unreactedA");
    card.unreactedB = reader.optionalReal(11, "unreactedB");
    card.unreactedR1 = reader.optionalReal(12, "unreactedR1");
    card.unreactedR2 = reader.optionalReal(13, "unreactedR2");
    card.unreactedOmega = reader.optionalReal(14, "unreactedOmega");
    card.ignitionCoefficient = reader.optionalReal(15, "ignitionCoefficient");
    card.growthCoefficient = reader.optionalReal(16, "growthCoefficient");
    card.ignitionDensityCoefficient = reader.optionalReal(17, "ignitionDensityCoefficient");
    card.productA = reader.optionalReal(18, "productA");
    card.productB = reader.optionalReal(19, "productB");
    card.productR1 = reader.optionalReal(20, "productR1");
    card.productR2 = reader.optionalReal(21, "productR2");
    card.productOmega = reader.optionalReal(22, "productOmega");
    card.burnExponentX = reader.real(23, "burnExponentX", IGNITION_GROWTH_BURN_EXPONENT_X_DEFAULT);
    card.burnExponentY = reader.real(24, "burnExponentY", IGNITION_GROWTH_BURN_EXPONENT_Y_DEFAULT);
    card.pressureExponent = reader.optionalReal(25, "pressureExponent");
    card.densityExponent = reader.real(26, "densityExponent", IGNITION_GROWTH_DENSITY_EXPONENT_DEFAULT);
    card.chemicalEnergy = reader.optionalReal(27, "chemicalEnergy");
    card.pressureTolerance = reader.real(28, "pressureTolerance", IGNITION_GROWTH_PRESSURE_TOLERANCE_DEFAULT);
    card.maxIterations = reader.integer(29, "maxIterations", IGNITION_GROWTH_MAX_ITERATIONS_DEFAULT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IgnitionGrowthStateEquationFormat::check(const IgnitionGrowthStateEquationCard& card)
{
    const bool required = card.explosiveDatabase == IgnitionGrowthStateEquationDatabase::NotUsed;
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (required && card.outputUnits && !card.inputUnits)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required to convert the data to a " +
                               CardProblem::fieldNamed("outputUnits") + " system",
                           "inputUnits"};
    if (const auto problem = checkConstant(card.unreactedA, "unreactedA", required))
        return problem;
    if (const auto problem = checkConstant(card.unreactedB, "unreactedB", required))
        return problem;
    if (const auto problem = checkConstant(card.unreactedR1, "unreactedR1", required))
        return problem;
    if (const auto problem = checkConstant(card.unreactedR2, "unreactedR2", required))
        return problem;
    if (const auto problem = checkConstant(card.unreactedOmega, "unreactedOmega", required))
        return problem;
    if (const auto problem = checkConstant(card.ignitionCoefficient, "ignitionCoefficient", required))
        return problem;
    if (const auto problem = checkConstant(card.growthCoefficient, "growthCoefficient", required))
        return problem;
    if (const auto problem = checkConstant(card.ignitionDensityCoefficient, "ignitionDensityCoefficient", required))
        return problem;
    if (const auto problem = checkConstant(card.productA, "productA", required))
        return problem;
    if (const auto problem = checkConstant(card.productB, "productB", required))
        return problem;
    if (const auto problem = checkConstant(card.productR1, "productR1", required))
        return problem;
    if (const auto problem = checkConstant(card.productR2, "productR2", required))
        return problem;
    if (const auto problem = checkConstant(card.productOmega, "productOmega", required))
        return problem;
    if (const auto problem = checkReal(card.burnExponentX, "burnExponentX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.burnExponentY, "burnExponentY", RealRule::Finite))
        return problem;
    if (const auto problem = checkConstant(card.pressureExponent, "pressureExponent", required))
        return problem;
    if (const auto problem = checkReal(card.densityExponent, "densityExponent", RealRule::Finite))
        return problem;
    if (const auto problem = checkConstant(card.chemicalEnergy, "chemicalEnergy", required))
        return problem;
    if (const auto problem = checkReal(card.pressureTolerance, "pressureTolerance", RealRule::Positive))
        return problem;
    if (card.maxIterations <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "maxIterations"};
    return std::nullopt;
}

void IgnitionGrowthStateEquationFormat::write(const IgnitionGrowthStateEquationCard& card, FieldWriter& writer)
{
    const auto putReal = [&writer](int field, const std::optional<double>& value)
    {
        if (value)
            writer.real(field, *value);
    };
    writer.integer(1, card.id);
    writeOptionalKeyword(writer, 2, IGNITION_GROWTH_UNIT_WORDS, card.inputUnits);
    writeKeywordUnlessDefault(writer, 3, IGNITION_GROWTH_DATABASE_WORDS, card.explosiveDatabase,
                              IgnitionGrowthStateEquationDatabase::NotUsed);
    writeOptionalKeyword(writer, 4, IGNITION_GROWTH_UNIT_WORDS, card.outputUnits);
    putReal(10, card.unreactedA);
    putReal(11, card.unreactedB);
    putReal(12, card.unreactedR1);
    putReal(13, card.unreactedR2);
    putReal(14, card.unreactedOmega);
    putReal(15, card.ignitionCoefficient);
    putReal(16, card.growthCoefficient);
    putReal(17, card.ignitionDensityCoefficient);
    putReal(18, card.productA);
    putReal(19, card.productB);
    putReal(20, card.productR1);
    putReal(21, card.productR2);
    putReal(22, card.productOmega);
    writer.realUnlessDefault(23, card.burnExponentX, IGNITION_GROWTH_BURN_EXPONENT_X_DEFAULT);
    writer.realUnlessDefault(24, card.burnExponentY, IGNITION_GROWTH_BURN_EXPONENT_Y_DEFAULT);
    putReal(25, card.pressureExponent);
    writer.realUnlessDefault(26, card.densityExponent, IGNITION_GROWTH_DENSITY_EXPONENT_DEFAULT);
    putReal(27, card.chemicalEnergy);
    writer.realUnlessDefault(28, card.pressureTolerance, IGNITION_GROWTH_PRESSURE_TOLERANCE_DEFAULT);
    writer.integerUnlessDefault(29, card.maxIterations, IGNITION_GROWTH_MAX_ITERATIONS_DEFAULT);
}

std::uint32_t IgnitionGrowthStateEquationTable::append(const IgnitionGrowthStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void IgnitionGrowthStateEquationTable::store(std::uint32_t row, const IgnitionGrowthStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void IgnitionGrowthStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void IgnitionGrowthStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
