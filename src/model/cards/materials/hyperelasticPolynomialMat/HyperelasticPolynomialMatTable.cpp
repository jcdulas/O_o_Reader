#include "oor/model/cards/materials/hyperelasticPolynomialMat/HyperelasticPolynomialMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// Where the constants of the higher orders sit on the continuation lines, in the order the card keeps them.
constexpr int COEFFICIENT_A_FIELDS[18] = {17, 18, 19, 25, 26, 27, 28, 33, 34, 35, 36, 37, 41, 42, 43, 44, 45, 46};
constexpr const char* COEFFICIENT_A_NAMES[18] = {
    "coefficientA20", "coefficientA11", "coefficientA02", "coefficientA30", "coefficientA21", "coefficientA12",
    "coefficientA03", "coefficientA40", "coefficientA31", "coefficientA22", "coefficientA13", "coefficientA04",
    "coefficientA50", "coefficientA41", "coefficientA32", "coefficientA23", "coefficientA14", "coefficientA05"};
constexpr int COEFFICIENT_D_FIELDS[4] = {20, 29, 38, 47};
constexpr const char* COEFFICIENT_D_NAMES[4] = {"coefficientD2", "coefficientD3", "coefficientD4", "coefficientD5"};
constexpr int DISTORTIONAL_ORDER_FIELD = 10;
constexpr int VOLUMETRIC_ORDER_FIELD = 11;
constexpr int TENSION_TABLE_FIELD = 49;
constexpr int VOLUMETRIC_TABLE_FIELD = 56;

bool inRange(int order)
{
    return order >= 1 && order <= 5;
}

} // namespace

std::optional<HyperelasticPolynomialMatCard> HyperelasticPolynomialMatFormat::read(FieldReader& reader)
{
    HyperelasticPolynomialMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA10 = reader.real(2, "coefficientA10", 0.0);
    card.coefficientA01 = reader.real(3, "coefficientA01", 0.0);
    card.coefficientD1 = reader.real(
        4, "coefficientD1",
        defaultCoefficientD1(card.coefficientA10, card.coefficientA01)); // format reference: depends on A10 and A01
    card.density = reader.real(5, "density", 0.0);
    card.thermalExpansion = reader.real(6, "thermalExpansion", 0.0);
    card.referenceTemperature = reader.real(7, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(8, "structuralDamping", 0.0);
    card.distortionalOrder = reader.integer(DISTORTIONAL_ORDER_FIELD, "distortionalOrder", 1);
    card.volumetricOrder = reader.integer(VOLUMETRIC_ORDER_FIELD, "volumetricOrder", 1);
    for (std::size_t index = 0; index < 18; ++index)
        card.coefficientsA[index] = reader.real(COEFFICIENT_A_FIELDS[index], COEFFICIENT_A_NAMES[index], 0.0);
    for (std::size_t index = 0; index < 4; ++index)
        card.coefficientsD[index] = reader.real(COEFFICIENT_D_FIELDS[index], COEFFICIENT_D_NAMES[index], 0.0);
    card.tensionTableId = reader.integer(TENSION_TABLE_FIELD, "tensionTableId", 0);
    card.equibiaxialTableId = reader.integer(TENSION_TABLE_FIELD + 1, "equibiaxialTableId", 0);
    card.simpleShearTableId = reader.integer(TENSION_TABLE_FIELD + 2, "simpleShearTableId", 0);
    card.pureShearTableId = reader.integer(TENSION_TABLE_FIELD + 3, "pureShearTableId", 0);
    card.volumetricTableId = reader.integer(VOLUMETRIC_TABLE_FIELD, "volumetricTableId", 0);
    requireBlankFields(reader, {9, 12, 13, 14, 15, 16, 21, 22, 23, 24, 30, 31, 32, 39, 40, 48, 53, 54, 55},
                       VOLUMETRIC_TABLE_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HyperelasticPolynomialMatFormat::check(const HyperelasticPolynomialMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!inRange(card.distortionalOrder))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 5",
                           "distortionalOrder"};
    if (!inRange(card.volumetricOrder))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer from 1 to 5",
                           "volumetricOrder"};
    if (!(card.coefficientD1 >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "coefficientD1"};
    for (std::size_t index = 0; index < 4; ++index)
    {
        if (!(card.coefficientsD[index] >= 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               COEFFICIENT_D_NAMES[index]};
    }
    if (const auto problem = checkTableReference(card.tensionTableId, "tensionTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.equibiaxialTableId, "equibiaxialTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.simpleShearTableId, "simpleShearTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.pureShearTableId, "pureShearTableId"))
        return problem;
    return checkTableReference(card.volumetricTableId, "volumetricTableId");
}

void HyperelasticPolynomialMatFormat::write(const HyperelasticPolynomialMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.coefficientA10, 0.0);
    writer.realUnlessDefault(3, card.coefficientA01, 0.0);
    writer.real(4, card.coefficientD1); // depends on A10 and A01: always written
    writer.realUnlessDefault(5, card.density, 0.0);
    writer.realUnlessDefault(6, card.thermalExpansion, 0.0);
    writer.realUnlessDefault(7, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(8, card.structuralDamping, 0.0);
    writer.integerUnlessDefault(DISTORTIONAL_ORDER_FIELD, card.distortionalOrder, 1);
    writer.integerUnlessDefault(VOLUMETRIC_ORDER_FIELD, card.volumetricOrder, 1);
    for (std::size_t index = 0; index < 18; ++index)
        writer.realUnlessDefault(COEFFICIENT_A_FIELDS[index], card.coefficientsA[index], 0.0);
    for (std::size_t index = 0; index < 4; ++index)
        writer.realUnlessDefault(COEFFICIENT_D_FIELDS[index], card.coefficientsD[index], 0.0);
    writer.integerUnlessDefault(TENSION_TABLE_FIELD, card.tensionTableId, 0);
    writer.integerUnlessDefault(TENSION_TABLE_FIELD + 1, card.equibiaxialTableId, 0);
    writer.integerUnlessDefault(TENSION_TABLE_FIELD + 2, card.simpleShearTableId, 0);
    writer.integerUnlessDefault(TENSION_TABLE_FIELD + 3, card.pureShearTableId, 0);
    writer.integerUnlessDefault(VOLUMETRIC_TABLE_FIELD, card.volumetricTableId, 0);
}

std::uint32_t HyperelasticPolynomialMatTable::append(const HyperelasticPolynomialMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_coefficientA10.push(card.coefficientA10);
    m_coefficientA01.push(card.coefficientA01);
    m_coefficientD1.push(card.coefficientD1);
    m_density.push(card.density);
    m_thermalExpansion.push(card.thermalExpansion);
    m_referenceTemperature.push(card.referenceTemperature);
    m_structuralDamping.push(card.structuralDamping);
    m_distortionalOrder.push(card.distortionalOrder);
    m_volumetricOrder.push(card.volumetricOrder);
    m_coefficientsA.push(card.coefficientsA);
    m_coefficientsD.push(card.coefficientsD);
    m_tensionTableId.push(card.tensionTableId);
    m_equibiaxialTableId.push(card.equibiaxialTableId);
    m_simpleShearTableId.push(card.simpleShearTableId);
    m_pureShearTableId.push(card.pureShearTableId);
    m_volumetricTableId.push(card.volumetricTableId);
    return row;
}

HyperelasticPolynomialMatCard HyperelasticPolynomialMatTable::card(std::uint32_t row) const
{
    HyperelasticPolynomialMatCard card;
    card.id = m_id.get(row);
    card.coefficientA10 = m_coefficientA10.get(row);
    card.coefficientA01 = m_coefficientA01.get(row);
    card.coefficientD1 = m_coefficientD1.get(row);
    card.density = m_density.get(row);
    card.thermalExpansion = m_thermalExpansion.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    card.distortionalOrder = m_distortionalOrder.get(row);
    card.volumetricOrder = m_volumetricOrder.get(row);
    card.coefficientsA = m_coefficientsA.get(row);
    card.coefficientsD = m_coefficientsD.get(row);
    card.tensionTableId = m_tensionTableId.get(row);
    card.equibiaxialTableId = m_equibiaxialTableId.get(row);
    card.simpleShearTableId = m_simpleShearTableId.get(row);
    card.pureShearTableId = m_pureShearTableId.get(row);
    card.volumetricTableId = m_volumetricTableId.get(row);
    return card;
}

void HyperelasticPolynomialMatTable::store(std::uint32_t row, const HyperelasticPolynomialMatCard& card)
{
    m_id.set(row, card.id);
    m_coefficientA10.set(row, card.coefficientA10);
    m_coefficientA01.set(row, card.coefficientA01);
    m_coefficientD1.set(row, card.coefficientD1);
    m_density.set(row, card.density);
    m_thermalExpansion.set(row, card.thermalExpansion);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_structuralDamping.set(row, card.structuralDamping);
    m_distortionalOrder.set(row, card.distortionalOrder);
    m_volumetricOrder.set(row, card.volumetricOrder);
    m_coefficientsA.set(row, card.coefficientsA);
    m_coefficientsD.set(row, card.coefficientsD);
    m_tensionTableId.set(row, card.tensionTableId);
    m_equibiaxialTableId.set(row, card.equibiaxialTableId);
    m_simpleShearTableId.set(row, card.simpleShearTableId);
    m_pureShearTableId.set(row, card.pureShearTableId);
    m_volumetricTableId.set(row, card.volumetricTableId);
}

void HyperelasticPolynomialMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_coefficientA10.keepRows(rows);
    m_coefficientA01.keepRows(rows);
    m_coefficientD1.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_distortionalOrder.keepRows(rows);
    m_volumetricOrder.keepRows(rows);
    m_coefficientsA.keepRows(rows);
    m_coefficientsD.keepRows(rows);
    m_tensionTableId.keepRows(rows);
    m_equibiaxialTableId.keepRows(rows);
    m_simpleShearTableId.keepRows(rows);
    m_pureShearTableId.keepRows(rows);
    m_volumetricTableId.keepRows(rows);
}

void HyperelasticPolynomialMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
