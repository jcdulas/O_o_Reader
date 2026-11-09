#include "oor/model/cards/materials/hyperelasticMat/HyperelasticMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

// Where the constants sit on the continuation lines, in the order the card keeps them.
constexpr int COEFFICIENT_C_FIELDS[20] = {9,  10, 17, 18, 19, 25, 26, 27, 28, 33,
                                          34, 35, 36, 37, 41, 42, 43, 44, 45, 46};
constexpr const char* COEFFICIENT_C_NAMES[20] = {
    "coefficientC10", "coefficientC01", "coefficientC20", "coefficientC11", "coefficientC02",
    "coefficientC30", "coefficientC21", "coefficientC12", "coefficientC03", "coefficientC40",
    "coefficientC31", "coefficientC22", "coefficientC13", "coefficientC04", "coefficientC50",
    "coefficientC41", "coefficientC32", "coefficientC23", "coefficientC14", "coefficientC05"};
constexpr int MOONEY_COEFFICIENT_D_FIELDS[5] = {11, 20, 29, 38, 47};
constexpr int OGDEN_COEFFICIENT_D_FIELDS[5] = {33, 34, 35, 36, 37};
constexpr int BOYCE_COEFFICIENT_D_FIELDS[5] = {17, 18, 19, 20, 21};
constexpr const char* COEFFICIENT_D_NAMES[5] = {"coefficientD1", "coefficientD2", "coefficientD3", "coefficientD4",
                                                "coefficientD5"};
constexpr int OGDEN_MODULUS_FIELDS[5] = {9, 17, 20, 25, 28};
constexpr int ALPHA_FIELDS[5] = {10, 18, 21, 26, 29};
constexpr int BETA_FIELDS[5] = {11, 19, 22, 27, 30};
constexpr const char* OGDEN_MODULUS_NAMES[5] = {"ogdenModulus1", "ogdenModulus2", "ogdenModulus3", "ogdenModulus4",
                                                "ogdenModulus5"};
constexpr const char* ALPHA_NAMES[5] = {"alphaCoefficient1", "alphaCoefficient2", "alphaCoefficient3",
                                        "alphaCoefficient4", "alphaCoefficient5"};
constexpr const char* BETA_NAMES[5] = {"betaCoefficient1", "betaCoefficient2", "betaCoefficient3", "betaCoefficient4",
                                       "betaCoefficient5"};
constexpr int TENSION_TABLE_FIELD = 12;
constexpr int VOLUMETRIC_TABLE_FIELD = 16;
constexpr int DISTORTIONAL_ORDER_FIELD = 21;
constexpr int VOLUMETRIC_ORDER_FIELD = 22;
constexpr int RUBBER_MODULUS_FIELD = 9;
constexpr int EXPONENT_OR_MODULUS_FIELD = 10;
constexpr int INVARIANT_LIMIT_FIELD = 11;

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

template <std::size_t N>
bool allZero(const std::array<double, N>& values)
{
    for (const double value : values)
    {
        if (value != 0.0)
            return false;
    }
    return true;
}

bool isMooney(HyperelasticMatModel model)
{
    return model == HyperelasticMatModel::Mooney;
}

bool isOgdenOrFoam(HyperelasticMatModel model)
{
    return model == HyperelasticMatModel::Ogden || model == HyperelasticMatModel::Foam;
}

bool isBoyceOrGent(HyperelasticMatModel model)
{
    return model == HyperelasticMatModel::Aboyce || model == HyperelasticMatModel::Gent;
}

bool inRange(int order)
{
    return order >= 1 && order <= 5;
}

} // namespace

const char* HyperelasticMatFormat::modelWord(HyperelasticMatModel model) noexcept
{
    switch (model)
    {
    case HyperelasticMatModel::Mooney:
        return "MooneyRivlin";
    case HyperelasticMatModel::Ogden:
        return "Ogden";
    case HyperelasticMatModel::Foam:
        return "Foam";
    case HyperelasticMatModel::Aboyce:
        return "ArrudaBoyce";
    case HyperelasticMatModel::Gent:
        return "Gent";
    case HyperelasticMatModel::Ghem1:
        return "GeneralHyperelasticModel1";
    case HyperelasticMatModel::Ghem2:
        return "GeneralHyperelasticModel2";
    case HyperelasticMatModel::Ghem3:
        return "GeneralHyperelasticModel3";
    case HyperelasticMatModel::Ghem4:
        return "GeneralHyperelasticModel4";
    case HyperelasticMatModel::Ghem5:
        return "GeneralHyperelasticModel5";
    case HyperelasticMatModel::Ghem6:
        return "GeneralHyperelasticModel6";
    }
    return "MooneyRivlin";
}

std::optional<HyperelasticMatModel> HyperelasticMatFormat::parseModel(std::string_view text) noexcept
{
    static constexpr HyperelasticMatModel ALL[] = {
        HyperelasticMatModel::Mooney, HyperelasticMatModel::Ogden, HyperelasticMatModel::Foam,
        HyperelasticMatModel::Aboyce, HyperelasticMatModel::Gent,  HyperelasticMatModel::Ghem1,
        HyperelasticMatModel::Ghem2,  HyperelasticMatModel::Ghem3, HyperelasticMatModel::Ghem4,
        HyperelasticMatModel::Ghem5,  HyperelasticMatModel::Ghem6};
    for (const HyperelasticMatModel model : ALL)
    {
        if ((text == modelWord(model)))
            return model;
    }
    return std::nullopt;
}

HyperelasticMatTable::HyperelasticMatTable()
    : m_bulkModulus(notGiven()),
      m_distortionalOrder(HyperelasticMatCard::DEFAULT_DISTORTIONAL_ORDER),
      m_volumetricOrder(1),
      m_termCount(1),
      m_rubberModulus(1.0),
      m_exponentOrModulus(1.0)
{
}

std::optional<HyperelasticMatCard> HyperelasticMatFormat::read(FieldReader& reader)
{
    HyperelasticMatCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view word = reader.text(2);
    if (!word.empty())
    {
        if (const auto model = parseModel(word))
            card.model = *model;
        else
            reader.error("card.bad-value",
                         std::string("modelName") + " is not one the card knows: \"" + std::string(word) + "\"",
                         "modelName");
    }
    card.bulkModulus = reader.optionalReal(4, "bulkModulus");
    card.density = reader.real(5, "density", 0.0);
    card.thermalExpansion = reader.real(6, "thermalExpansion", 0.0);
    card.referenceTemperature = reader.real(7, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(8, "structuralDamping", 0.0);

    if (isMooney(card.model))
    {
        for (std::size_t index = 0; index < 20; ++index)
            card.coefficientsC[index] = reader.real(COEFFICIENT_C_FIELDS[index], COEFFICIENT_C_NAMES[index], 0.0);
        for (std::size_t index = 0; index < 5; ++index)
            card.coefficientsD[index] =
                reader.real(MOONEY_COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0.0);
        card.tensionTableId = reader.integer(TENSION_TABLE_FIELD, "tensionTableId", 0);
        card.equibiaxialTableId = reader.integer(TENSION_TABLE_FIELD + 1, "equibiaxialTableId", 0);
        card.simpleShearTableId = reader.integer(TENSION_TABLE_FIELD + 2, "simpleShearTableId", 0);
        card.pureShearTableId = reader.integer(TENSION_TABLE_FIELD + 3, "pureShearTableId", 0);
        card.volumetricTableId = reader.integer(VOLUMETRIC_TABLE_FIELD, "volumetricTableId", 0);
        card.distortionalOrder = reader.integer(DISTORTIONAL_ORDER_FIELD, "distortionalOrder",
                                                HyperelasticMatCard::DEFAULT_DISTORTIONAL_ORDER);
        card.volumetricOrder = reader.integer(VOLUMETRIC_ORDER_FIELD, "volumetricOrder", 1);
        requireBlankFields(reader, {3, 23, 24, 30, 31, 32, 39, 40, 48}, 47);
    }
    else if (isOgdenOrFoam(card.model))
    {
        card.termCount = reader.integer(3, "termCount", 1);
        for (std::size_t index = 0; index < 5; ++index)
        {
            card.ogdenModuli[index] = reader.real(OGDEN_MODULUS_FIELDS[index], OGDEN_MODULUS_NAMES[index], 0.0);
            card.alpha[index] = reader.real(ALPHA_FIELDS[index], ALPHA_NAMES[index], 0.0);
            card.beta[index] = reader.real(BETA_FIELDS[index], BETA_NAMES[index], 0.0);
            card.coefficientsD[index] = reader.real(OGDEN_COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0.0);
        }
        requireBlankFields(reader, {12, 13, 14, 15, 16, 23, 24, 31, 32, 38, 39, 40}, 37);
    }
    else if (isBoyceOrGent(card.model))
    {
        card.rubberModulus = reader.real(RUBBER_MODULUS_FIELD, "rubberModulus", 1.0);
        card.exponentOrModulus = reader.real(EXPONENT_OR_MODULUS_FIELD, "exponentOrModulus", 1.0);
        card.invariantLimit = reader.real(INVARIANT_LIMIT_FIELD, "invariantLimit", 0.0);
        for (std::size_t index = 0; index < 5; ++index)
            card.coefficientsD[index] = reader.real(BOYCE_COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0.0);
        requireBlankFields(reader, {3, 12, 13, 14, 15, 16, 22, 23, 24}, 21);
    }
    else
    {
        requireBlankFields(reader, {3}, 8);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HyperelasticMatFormat::check(const HyperelasticMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.bulkModulus && !(*card.bulkModulus > 0.0 || *card.bulkModulus == -1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 or -1.0",
                           "bulkModulus"};
    // What the model has no field for stays at its default (nothing is dropped silently).
    if (!isMooney(card.model))
    {
        if (!allZero(card.coefficientsC))
            return CardProblem{"card.bad-value", "the constants Cij belong to the MOONEY model", "coefficientC10"};
        if (card.tensionTableId != 0 || card.equibiaxialTableId != 0 || card.simpleShearTableId != 0 ||
            card.pureShearTableId != 0 || card.volumetricTableId != 0)
            return CardProblem{"card.bad-value", "the experiment tables belong to the MOONEY model", "tensionTableId"};
        if (card.distortionalOrder != HyperelasticMatCard::DEFAULT_DISTORTIONAL_ORDER)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " belongs to the MOONEY model",
                               "distortionalOrder"};
        if (card.volumetricOrder != 1)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " belongs to the MOONEY model",
                               "volumetricOrder"};
    }
    if (!isOgdenOrFoam(card.model))
    {
        if (card.termCount != 1)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " belongs to the OGDEN and FOAM models", "termCount"};
        if (!allZero(card.ogdenModuli) || !allZero(card.alpha))
            return CardProblem{"card.bad-value",
                               "the moduli and exponents of the Ogden terms (" + std::string(CardProblem::FIELD) +
                                   ", " + CardProblem::fieldNamed("alphaCoefficient1") +
                                   ", ...) belong to the OGDEN and FOAM models",
                               "ogdenModulus1"};
    }
    if (card.model != HyperelasticMatModel::Foam && !allZero(card.beta))
        return CardProblem{"card.bad-value",
                           "the exponents " + std::string(CardProblem::FIELD) + ", ... belong to the FOAM model",
                           "betaCoefficient1"};
    if (!isBoyceOrGent(card.model) && (card.rubberModulus != 1.0 || card.exponentOrModulus != 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("exponentOrModulus") +
                               " belong to the ABOYCE and GENT models",
                           "rubberModulus"};
    if (card.model != HyperelasticMatModel::Gent && card.invariantLimit != 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " belongs to the GENT model",
                           "invariantLimit"};
    if (!(isMooney(card.model) || card.model == HyperelasticMatModel::Ogden || isBoyceOrGent(card.model)) &&
        !allZero(card.coefficientsD))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " to " + CardProblem::fieldNamed("coefficientD5") +
                               " are not used by this model",
                           "coefficientD1"};
    // The ranges of the values.
    if (isMooney(card.model))
    {
        if (!inRange(card.distortionalOrder))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 5",
                               "distortionalOrder"};
        if (!inRange(card.volumetricOrder))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 5",
                               "volumetricOrder"};
        if (const auto problem = checkTableReference(card.tensionTableId, "tensionTableId"))
            return problem;
        if (const auto problem = checkTableReference(card.equibiaxialTableId, "equibiaxialTableId"))
            return problem;
        if (const auto problem = checkTableReference(card.simpleShearTableId, "simpleShearTableId"))
            return problem;
        if (const auto problem = checkTableReference(card.pureShearTableId, "pureShearTableId"))
            return problem;
        if (const auto problem = checkTableReference(card.volumetricTableId, "volumetricTableId"))
            return problem;
    }
    if (isOgdenOrFoam(card.model) && !inRange(card.termCount))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 5",
                           "termCount"};
    if (isBoyceOrGent(card.model) && !(card.rubberModulus > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "rubberModulus"};
    if (isBoyceOrGent(card.model) && !(card.exponentOrModulus > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "exponentOrModulus"};
    for (std::size_t index = 0; index < 5; ++index)
    {
        if (!(card.coefficientsD[index] >= 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               COEFFICIENT_D_NAMES[index]};
    }
    return std::nullopt;
}

void HyperelasticMatFormat::write(const HyperelasticMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, modelWord(card.model), "MooneyRivlin");
    if (isOgdenOrFoam(card.model))
        writer.integerUnlessDefault(3, card.termCount, 1);
    if (card.bulkModulus)
        writer.real(4, *card.bulkModulus);
    writer.realUnlessDefault(5, card.density, 0.0);
    writer.realUnlessDefault(6, card.thermalExpansion, 0.0);
    writer.realUnlessDefault(7, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(8, card.structuralDamping, 0.0);
    if (isMooney(card.model))
    {
        for (std::size_t index = 0; index < 20; ++index)
            writer.realUnlessDefault(COEFFICIENT_C_FIELDS[index], card.coefficientsC[index], 0.0);
        for (std::size_t index = 0; index < 5; ++index)
            writer.realUnlessDefault(MOONEY_COEFFICIENT_D_FIELDS[index], card.coefficientsD[index], 0.0);
        writer.integerUnlessDefault(TENSION_TABLE_FIELD, card.tensionTableId, 0);
        writer.integerUnlessDefault(TENSION_TABLE_FIELD + 1, card.equibiaxialTableId, 0);
        writer.integerUnlessDefault(TENSION_TABLE_FIELD + 2, card.simpleShearTableId, 0);
        writer.integerUnlessDefault(TENSION_TABLE_FIELD + 3, card.pureShearTableId, 0);
        writer.integerUnlessDefault(VOLUMETRIC_TABLE_FIELD, card.volumetricTableId, 0);
        writer.integerUnlessDefault(DISTORTIONAL_ORDER_FIELD, card.distortionalOrder,
                                    HyperelasticMatCard::DEFAULT_DISTORTIONAL_ORDER);
        writer.integerUnlessDefault(VOLUMETRIC_ORDER_FIELD, card.volumetricOrder, 1);
    }
    else if (isOgdenOrFoam(card.model))
    {
        for (std::size_t index = 0; index < 5; ++index)
        {
            writer.realUnlessDefault(OGDEN_MODULUS_FIELDS[index], card.ogdenModuli[index], 0.0);
            writer.realUnlessDefault(ALPHA_FIELDS[index], card.alpha[index], 0.0);
            writer.realUnlessDefault(BETA_FIELDS[index], card.beta[index], 0.0);
            writer.realUnlessDefault(OGDEN_COEFFICIENT_D_FIELDS[index], card.coefficientsD[index], 0.0);
        }
    }
    else if (isBoyceOrGent(card.model))
    {
        writer.realUnlessDefault(RUBBER_MODULUS_FIELD, card.rubberModulus, 1.0);
        writer.realUnlessDefault(EXPONENT_OR_MODULUS_FIELD, card.exponentOrModulus, 1.0);
        writer.realUnlessDefault(INVARIANT_LIMIT_FIELD, card.invariantLimit, 0.0);
        for (std::size_t index = 0; index < 5; ++index)
            writer.realUnlessDefault(BOYCE_COEFFICIENT_D_FIELDS[index], card.coefficientsD[index], 0.0);
    }
}

std::uint32_t HyperelasticMatTable::append(const HyperelasticMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_model.push(static_cast<std::uint8_t>(card.model));
    m_bulkModulus.push(card.bulkModulus.value_or(notGiven()));
    m_density.push(card.density);
    m_thermalExpansion.push(card.thermalExpansion);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_coefficientsC.push(card.coefficientsC);
    m_tensionTableId.push(card.tensionTableId);
    m_equibiaxialTableId.push(card.equibiaxialTableId);
    m_simpleShearTableId.push(card.simpleShearTableId);
    m_pureShearTableId.push(card.pureShearTableId);
    m_volumetricTableId.push(card.volumetricTableId);
    m_distortionalOrder.push(card.distortionalOrder);
    m_volumetricOrder.push(card.volumetricOrder);
    m_termCount.push(card.termCount);
    m_ogdenModuli.push(card.ogdenModuli);
    m_alpha.push(card.alpha);
    m_beta.push(card.beta);
    m_rubberModulus.push(card.rubberModulus);
    m_exponentOrModulus.push(card.exponentOrModulus);
    m_invariantLimit.push(card.invariantLimit);
    m_coefficientsD.push(card.coefficientsD);
    return row;
}

HyperelasticMatCard HyperelasticMatTable::card(std::uint32_t row) const
{
    HyperelasticMatCard card;
    card.id = m_id.get(row);
    card.model = static_cast<HyperelasticMatModel>(m_model.get(row));
    if (const double k = m_bulkModulus.get(row); !std::isnan(k))
        card.bulkModulus = k;
    card.density = m_density.get(row);
    card.thermalExpansion = m_thermalExpansion.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    card.coefficientsC = m_coefficientsC.get(row);
    card.tensionTableId = m_tensionTableId.get(row);
    card.equibiaxialTableId = m_equibiaxialTableId.get(row);
    card.simpleShearTableId = m_simpleShearTableId.get(row);
    card.pureShearTableId = m_pureShearTableId.get(row);
    card.volumetricTableId = m_volumetricTableId.get(row);
    card.distortionalOrder = m_distortionalOrder.get(row);
    card.volumetricOrder = m_volumetricOrder.get(row);
    card.termCount = m_termCount.get(row);
    card.ogdenModuli = m_ogdenModuli.get(row);
    card.alpha = m_alpha.get(row);
    card.beta = m_beta.get(row);
    card.rubberModulus = m_rubberModulus.get(row);
    card.exponentOrModulus = m_exponentOrModulus.get(row);
    card.invariantLimit = m_invariantLimit.get(row);
    card.coefficientsD = m_coefficientsD.get(row);
    return card;
}

void HyperelasticMatTable::store(std::uint32_t row, const HyperelasticMatCard& card)
{
    m_id.set(row, card.id);
    m_model.set(row, static_cast<std::uint8_t>(card.model));
    m_bulkModulus.set(row, card.bulkModulus.value_or(notGiven()));
    m_density.set(row, card.density);
    m_thermalExpansion.set(row, card.thermalExpansion);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_coefficientsC.set(row, card.coefficientsC);
    m_tensionTableId.set(row, card.tensionTableId);
    m_equibiaxialTableId.set(row, card.equibiaxialTableId);
    m_simpleShearTableId.set(row, card.simpleShearTableId);
    m_pureShearTableId.set(row, card.pureShearTableId);
    m_volumetricTableId.set(row, card.volumetricTableId);
    m_distortionalOrder.set(row, card.distortionalOrder);
    m_volumetricOrder.set(row, card.volumetricOrder);
    m_termCount.set(row, card.termCount);
    m_ogdenModuli.set(row, card.ogdenModuli);
    m_alpha.set(row, card.alpha);
    m_beta.set(row, card.beta);
    m_rubberModulus.set(row, card.rubberModulus);
    m_exponentOrModulus.set(row, card.exponentOrModulus);
    m_invariantLimit.set(row, card.invariantLimit);
    m_coefficientsD.set(row, card.coefficientsD);
}

void HyperelasticMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_model.keepRows(rows);
    m_bulkModulus.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_coefficientsC.keepRows(rows);
    m_tensionTableId.keepRows(rows);
    m_equibiaxialTableId.keepRows(rows);
    m_simpleShearTableId.keepRows(rows);
    m_pureShearTableId.keepRows(rows);
    m_volumetricTableId.keepRows(rows);
    m_distortionalOrder.keepRows(rows);
    m_volumetricOrder.keepRows(rows);
    m_termCount.keepRows(rows);
    m_ogdenModuli.keepRows(rows);
    m_alpha.keepRows(rows);
    m_beta.keepRows(rows);
    m_rubberModulus.keepRows(rows);
    m_exponentOrModulus.keepRows(rows);
    m_invariantLimit.keepRows(rows);
    m_coefficientsD.keepRows(rows);
}

void HyperelasticMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The experiment tables are TABLES1 cards.
    if (const base::EntityId tensionTable = m_tensionTableId.get(row); tensionTable > 0)
        m_tensionTableId.set(row, visitor.visit("tensionTableId", ReferenceTarget::StressTable, tensionTable));
    if (const base::EntityId equibiaxialTable = m_equibiaxialTableId.get(row); equibiaxialTable > 0)
        m_equibiaxialTableId.set(row,
                                 visitor.visit("equibiaxialTableId", ReferenceTarget::StressTable, equibiaxialTable));
    if (const base::EntityId simpleShearTable = m_simpleShearTableId.get(row); simpleShearTable > 0)
        m_simpleShearTableId.set(row,
                                 visitor.visit("simpleShearTableId", ReferenceTarget::StressTable, simpleShearTable));
    if (const base::EntityId pureShearTable = m_pureShearTableId.get(row); pureShearTable > 0)
        m_pureShearTableId.set(row, visitor.visit("pureShearTableId", ReferenceTarget::StressTable, pureShearTable));
    if (const base::EntityId volumetricTable = m_volumetricTableId.get(row); volumetricTable > 0)
        m_volumetricTableId.set(row, visitor.visit("volumetricTableId", ReferenceTarget::StressTable, volumetricTable));
}

} // namespace oor::model
