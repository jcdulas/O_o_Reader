#include "oor/model/cards/materials/orthoMat3DNonlinear/OrthoMat3DNonlinearTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The fields of the card: eight to a line, the yield fields on the third and fourth lines, the axes on the fifth.
constexpr int FIELD_YIELD_CRITERION = 17;
constexpr int FIELD_MATERIAL_AXES = 33;

// The fields of R11 or m, R22 or C1, R33 or C2, R12 or C3, R23 or C6, R31.
constexpr int YIELD_FIELDS[6] = {21, 22, 23, 25, 26, 27};
constexpr const char* YIELD_NAMES[6] = {"yieldStressRatio11", "yieldStressRatio22", "yieldStressRatio33",
                                        "yieldStressRatio12", "yieldStressRatio23", "yieldStressRatio31"};
// The components of the two vectors of OPTION VECT: X1, Y1, Z1 and X2, Y2, Z2.
constexpr const char* VECTOR1_NAMES[3] = {"vector1X", "vector1Y", "vector1Z"};
constexpr const char* VECTOR2_NAMES[3] = {"vector2X", "vector2Y", "vector2Z"};

bool isFinitePositive(double value)
{
    return std::isfinite(value) && value > 0.0;
}

bool isNonZero(const std::array<double, 3>& vector)
{
    return vector[0] != 0.0 || vector[1] != 0.0 || vector[2] != 0.0;
}

bool areParallel(const std::array<double, 3>& a, const std::array<double, 3>& b)
{
    const double x = a[1] * b[2] - a[2] * b[1];
    const double y = a[2] * b[0] - a[0] * b[2];
    const double z = a[0] * b[1] - a[1] * b[0];
    return x == 0.0 && y == 0.0 && z == 0.0;
}

} // namespace

std::optional<OrthoMat3DNonlinearCard> OrthoMat3DNonlinearFormat::read(FieldReader& reader)
{
    OrthoMat3DNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1 = reader.requiredReal(2, "youngsModulus1");
    card.youngsModulus2 = reader.requiredReal(3, "youngsModulus2");
    card.youngsModulus3 = reader.requiredReal(4, "youngsModulus3");
    card.poissonRatio12 = reader.requiredReal(5, "poissonRatio12");
    card.poissonRatio23 = reader.real(6, "poissonRatio23", 0.0);
    card.poissonRatio31 =
        reader.real(7, "poissonRatio31", card.poissonRatio23); // format reference: the default is NU23
    card.density = reader.real(8, "density", 0.0);
    card.shearModulus12 = reader.real(9, "shearModulus12", 0.0);
    card.shearModulus23 = reader.real(10, "shearModulus23", 0.0);
    card.shearModulus31 = reader.real(11, "shearModulus31", 0.0);
    card.thermalExpansion1 = reader.real(12, "thermalExpansion1", 0.0);
    card.thermalExpansion2 = reader.real(13, "thermalExpansion2", 0.0);
    card.thermalExpansion3 = reader.real(14, "thermalExpansion3", 0.0);
    card.referenceTemperature = reader.real(15, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(16, "structuralDamping", 0.0);
    card.yieldCriterion = reader.integer(FIELD_YIELD_CRITERION, "yieldCriterion", 1);
    card.hardeningRule = reader.integer(FIELD_YIELD_CRITERION + 1, "hardeningRule", 1);
    card.yieldStress = reader.optionalReal(FIELD_YIELD_CRITERION + 2, "yieldStress");
    for (std::size_t index = 0; index < card.yieldFields.size(); ++index)
        card.yieldFields[index] = reader.optionalReal(YIELD_FIELDS[index], YIELD_NAMES[index]);
    card.materialAxes =
        reader.isBlank(FIELD_MATERIAL_AXES) ? "Elements" : std::string(reader.text(FIELD_MATERIAL_AXES));
    card.propertiesFile = std::string(reader.word(FIELD_MATERIAL_AXES + 1));
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        card.vector1[axis] = reader.real(FIELD_MATERIAL_AXES + 2 + static_cast<int>(axis), VECTOR1_NAMES[axis], 0.0);
        card.vector2[axis] = reader.real(FIELD_MATERIAL_AXES + 5 + static_cast<int>(axis), VECTOR2_NAMES[axis], 0.0);
    }
    requireUnusedBlank(reader, {20, 24, 28, 29, 30, 31, 32}, FIELD_MATERIAL_AXES + 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat3DNonlinearFormat::check(const OrthoMat3DNonlinearCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!isFinitePositive(card.youngsModulus1))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "youngsModulus1"};
    if (!isFinitePositive(card.youngsModulus2))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "youngsModulus2"};
    if (!isFinitePositive(card.youngsModulus3))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "youngsModulus3"};
    for (const double ratio : {card.poissonRatio12, card.poissonRatio23, card.poissonRatio31})
    {
        if (!std::isfinite(ratio))
            return CardProblem{"card.bad-value", "a Poisson's ratio must be a finite number", "poissonRatio12"};
    }
    // The shear moduli are meant to be given, but a card that leaves them blank is common: a blank one is 0.0, and a
    // modulus is not negative.
    for (const double modulus : {card.shearModulus12, card.shearModulus23, card.shearModulus31})
    {
        if (!(modulus >= 0.0) || !std::isfinite(modulus))
            return CardProblem{"card.bad-value", "a shear modulus must not be negative", "shearModulus12"};
    }
    for (const double value : {card.density, card.thermalExpansion1, card.thermalExpansion2, card.thermalExpansion3,
                               card.referenceTemperature, card.structuralDamping})
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value",
                               "the density, the thermal expansion coefficients, the reference temperature and the "
                               "damping must be finite numbers",
                               "density"};
    }
    if (card.yieldCriterion != -1 && (card.yieldCriterion < 1 || card.yieldCriterion > 4))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be -1, 1, 2, 3 or 4",
                           "yieldCriterion"};
    if (card.hardeningRule < 1 || card.hardeningRule > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3", "hardeningRule"};
    if (card.yieldStress && !isFinitePositive(*card.yieldStress))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "yieldStress"};
    for (std::size_t index = 0; index < card.yieldFields.size(); ++index)
    {
        if (card.yieldFields[index] && !isFinitePositive(*card.yieldFields[index]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               YIELD_NAMES[index]};
    }
    if (auto problem = checkKeyword(card.materialAxes, "materialAxes",
                                    {"Vector", "Elements", "ElemMatOrientation", "ElemPropOrientation"}))
        return problem;
    const std::string axes = CardProblem::fieldNamed("materialAxes");
    if (!card.propertiesFile.empty() && card.materialAxes != "ElemMatOrientation")
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is for " + axes + " ELMAT only",
                           "propertiesFile"};
    if (card.materialAxes == "ElemMatOrientation" && card.propertiesFile.empty())
        return CardProblem{"card.missing-field",
                           axes + " ELMAT needs the " + std::string(CardProblem::FIELD) + " of the material properties",
                           "propertiesFile"};
    const std::string vectors = CardProblem::fieldNamed("vector1X") + " to " + CardProblem::fieldNamed("vector2Z");
    for (const double value :
         {card.vector1[0], card.vector1[1], card.vector1[2], card.vector2[0], card.vector2[1], card.vector2[2]})
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "the vector components " + vectors + " must be finite numbers",
                               "vector1X"};
    }
    if (card.materialAxes == "Vector")
    {
        if (!isNonZero(card.vector1) || !isNonZero(card.vector2) || areParallel(card.vector1, card.vector2))
            return CardProblem{"card.bad-value",
                               axes + " VECT needs two vectors (" + vectors + ") that are not parallel", "vector1X"};
    }
    else if (isNonZero(card.vector1) || isNonZero(card.vector2))
    {
        return CardProblem{"card.bad-value", "the vectors " + vectors + " are for " + axes + " VECT only", "vector1X"};
    }
    return std::nullopt;
}

void OrthoMat3DNonlinearFormat::write(const OrthoMat3DNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.youngsModulus1);
    writer.real(3, card.youngsModulus2);
    writer.real(4, card.youngsModulus3);
    writer.real(5, card.poissonRatio12);
    writer.realUnlessDefault(6, card.poissonRatio23, 0.0);
    writer.real(7, card.poissonRatio31); // the default is NU23: written, so that the card is read back as it is
    writer.realUnlessDefault(8, card.density, 0.0);
    writer.realUnlessDefault(9, card.shearModulus12, 0.0);
    writer.realUnlessDefault(10, card.shearModulus23, 0.0);
    writer.realUnlessDefault(11, card.shearModulus31, 0.0);
    writer.realUnlessDefault(12, card.thermalExpansion1, 0.0);
    writer.realUnlessDefault(13, card.thermalExpansion2, 0.0);
    writer.realUnlessDefault(14, card.thermalExpansion3, 0.0);
    writer.realUnlessDefault(15, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(16, card.structuralDamping, 0.0);
    writer.integerUnlessDefault(FIELD_YIELD_CRITERION, card.yieldCriterion, 1);
    writer.integerUnlessDefault(FIELD_YIELD_CRITERION + 1, card.hardeningRule, 1);
    if (card.yieldStress)
        writer.real(FIELD_YIELD_CRITERION + 2, *card.yieldStress);
    for (std::size_t index = 0; index < card.yieldFields.size(); ++index)
    {
        if (card.yieldFields[index])
            writer.real(YIELD_FIELDS[index], *card.yieldFields[index]);
    }
    writer.textUnlessDefault(FIELD_MATERIAL_AXES, card.materialAxes, "Elements");
    if (!card.propertiesFile.empty())
        writer.text(FIELD_MATERIAL_AXES + 1, card.propertiesFile);
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        writer.realUnlessDefault(FIELD_MATERIAL_AXES + 2 + static_cast<int>(axis), card.vector1[axis], 0.0);
        writer.realUnlessDefault(FIELD_MATERIAL_AXES + 5 + static_cast<int>(axis), card.vector2[axis], 0.0);
    }
}

std::uint32_t OrthoMat3DNonlinearTable::append(const OrthoMat3DNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void OrthoMat3DNonlinearTable::store(std::uint32_t row, const OrthoMat3DNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void OrthoMat3DNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void OrthoMat3DNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
