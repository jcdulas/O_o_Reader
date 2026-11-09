#include "oor/model/cards/materials/fabricMat/FabricMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIELD_WARP_MODULUS_LINEAR = 9;
constexpr int FIELD_WARP_ANGLE = 12;
constexpr int FIELD_WARP = 13; // XWARP, YWARP, ZWARP
constexpr int FIELD_WEFT_MODULUS_LINEAR = 17;
constexpr int FIELD_WEFT_ANGLE = 20;
constexpr int FIELD_WEFT = 21; // XWEFT, YWEFT, ZWEFT
constexpr int FIELD_SHEAR_FRICTION = 25;

bool isFinite(const std::optional<double>& value)
{
    return !value || std::isfinite(*value);
}

bool isFinite(const std::array<double, 3>& values)
{
    return std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]);
}

} // namespace

std::optional<FabricMatCard> FabricMatFormat::read(FieldReader& reader)
{
    FabricMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.density = reader.requiredReal(2, "density");
    card.coatingYoungsModulus = reader.optionalReal(3, "coatingYoungsModulus");
    card.coatingPoissonRatio = reader.optionalReal(4, "coatingPoissonRatio");
    card.coatingShearModulus = reader.optionalReal(5, "coatingShearModulus");
    card.coatingDamping = reader.real(6, "coatingDamping", 0.1);
    card.coatingCompressionScale = reader.real(7, "coatingCompressionScale", 1.0);
    card.coatingThicknessPercent = reader.real(8, "coatingThicknessPercent", 0.0);
    card.warpModulusLinear = reader.requiredReal(FIELD_WARP_MODULUS_LINEAR, "warpModulusLinear");
    card.warpModulusQuadratic = reader.real(FIELD_WARP_MODULUS_LINEAR + 1, "warpModulusQuadratic", 0.0);
    card.warpAngle = reader.optionalReal(FIELD_WARP_ANGLE, "warpAngle");
    card.warpDirection[0] = reader.real(FIELD_WARP, "warpDirectionX", 1.0);
    card.warpDirection[1] = reader.real(FIELD_WARP + 1, "warpDirectionY", 0.0);
    card.warpDirection[2] = reader.real(FIELD_WARP + 2, "warpDirectionZ", 0.0);
    card.weftModulusLinear = reader.requiredReal(FIELD_WEFT_MODULUS_LINEAR, "weftModulusLinear");
    card.weftModulusQuadratic = reader.real(FIELD_WEFT_MODULUS_LINEAR + 1, "weftModulusQuadratic", 0.0);
    card.weftAngle = reader.optionalReal(FIELD_WEFT_ANGLE, "weftAngle");
    card.weftDirection[0] = reader.real(FIELD_WEFT, "weftDirectionX", 0.0);
    card.weftDirection[1] = reader.real(FIELD_WEFT + 1, "weftDirectionY", 1.0);
    card.weftDirection[2] = reader.real(FIELD_WEFT + 2, "weftDirectionZ", 0.0);
    card.shearFriction = reader.real(FIELD_SHEAR_FRICTION, "shearFriction", 0.0);
    card.shearModulus12 = reader.optionalReal(FIELD_SHEAR_FRICTION + 1, "shearModulus12");
    card.fiberDamping = reader.real(FIELD_SHEAR_FRICTION + 2, "fiberDamping", 0.1);
    card.fiberCompressionScale = reader.real(FIELD_SHEAR_FRICTION + 3, "fiberCompressionScale", 1.0);
    card.lockAngle1 = reader.real(FIELD_SHEAR_FRICTION + 4, "lockAngle1", 10.0);
    card.lockAngle2 = reader.real(FIELD_SHEAR_FRICTION + 5, "lockAngle2", 15.0);
    requireBlankFields(reader, {11, 16, 19, 24}, FIELD_SHEAR_FRICTION + 5);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FabricMatFormat::check(const FabricMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.density > 0.0) || !std::isfinite(card.density))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "density"};
    if (!(card.warpModulusLinear > 0.0) || !std::isfinite(card.warpModulusLinear))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "warpModulusLinear"};
    if (!(card.weftModulusLinear > 0.0) || !std::isfinite(card.weftModulusLinear))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "weftModulusLinear"};
    if (!(card.warpModulusQuadratic >= 0.0) || !std::isfinite(card.warpModulusQuadratic) ||
        !(card.weftModulusQuadratic >= 0.0) || !std::isfinite(card.weftModulusQuadratic))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("weftModulusQuadratic") +
                               " must not be negative",
                           "warpModulusQuadratic"};
    for (const std::optional<double>& value :
         {card.coatingYoungsModulus, card.coatingPoissonRatio, card.coatingShearModulus})
    {
        if (value && !(*value > 0.0 && std::isfinite(*value)))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("coatingPoissonRatio") +
                                   " and " + CardProblem::fieldNamed("coatingShearModulus") +
                                   " must be greater than zero",
                               "coatingYoungsModulus"};
    }
    if (!(card.coatingDamping >= 0.0) || !std::isfinite(card.coatingDamping) || !(card.fiberDamping >= 0.0) ||
        !std::isfinite(card.fiberDamping))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("fiberDamping") +
                               " must not be negative",
                           "coatingDamping"};
    if (!(card.coatingCompressionScale >= 0.0 && card.coatingCompressionScale <= 1.0) ||
        !(card.fiberCompressionScale >= 0.0 && card.fiberCompressionScale <= 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("fiberCompressionScale") + " must be from 0.0 to 1.0",
                           "coatingCompressionScale"};
    if (!(card.coatingThicknessPercent >= 0.0 && card.coatingThicknessPercent <= 100.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be from 0.0 to 100.0",
                           "coatingThicknessPercent"};
    // with a coating, two of the three constants of the coating are given.
    const int given =
        (card.coatingYoungsModulus ? 1 : 0) + (card.coatingPoissonRatio ? 1 : 0) + (card.coatingShearModulus ? 1 : 0);
    if (card.coatingThicknessPercent > 0.0 && given < 2)
        return CardProblem{"card.bad-value",
                           "a coating (" + std::string(CardProblem::FIELD) + " above 0.0) needs two of " +
                               CardProblem::fieldNamed("coatingYoungsModulus") + ", " +
                               CardProblem::fieldNamed("coatingPoissonRatio") + " and " +
                               CardProblem::fieldNamed("coatingShearModulus"),
                           "coatingThicknessPercent"};
    if (!isFinite(card.warpAngle) || !isFinite(card.weftAngle) || !isFinite(card.shearModulus12) ||
        !isFinite(card.warpDirection) || !isFinite(card.weftDirection) || !std::isfinite(card.shearFriction))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("weftAngle") + ", " +
                               CardProblem::fieldNamed("shearModulus12") + ", the warp and weft vectors and " +
                               CardProblem::fieldNamed("shearFriction") + " must be finite numbers",
                           "warpAngle"};
    if (card.shearModulus12 && !(*card.shearModulus12 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "shearModulus12"};
    if (!(card.lockAngle1 >= 0.0) || !std::isfinite(card.lockAngle1) || !(card.lockAngle2 >= 0.0) ||
        !std::isfinite(card.lockAngle2))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("lockAngle2") + " must not be negative",
            "lockAngle1"};
    return std::nullopt;
}

void FabricMatFormat::write(const FabricMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.density);
    if (card.coatingYoungsModulus)
        writer.real(3, *card.coatingYoungsModulus);
    if (card.coatingPoissonRatio)
        writer.real(4, *card.coatingPoissonRatio);
    if (card.coatingShearModulus)
        writer.real(5, *card.coatingShearModulus);
    writer.realUnlessDefault(6, card.coatingDamping, 0.1);
    writer.realUnlessDefault(7, card.coatingCompressionScale, 1.0);
    writer.realUnlessDefault(8, card.coatingThicknessPercent, 0.0);
    writer.real(FIELD_WARP_MODULUS_LINEAR, card.warpModulusLinear);
    writer.realUnlessDefault(FIELD_WARP_MODULUS_LINEAR + 1, card.warpModulusQuadratic, 0.0);
    if (card.warpAngle)
        writer.real(FIELD_WARP_ANGLE, *card.warpAngle);
    writer.realUnlessDefault(FIELD_WARP, card.warpDirection[0], 1.0);
    writer.realUnlessDefault(FIELD_WARP + 1, card.warpDirection[1], 0.0);
    writer.realUnlessDefault(FIELD_WARP + 2, card.warpDirection[2], 0.0);
    writer.real(FIELD_WEFT_MODULUS_LINEAR, card.weftModulusLinear);
    writer.realUnlessDefault(FIELD_WEFT_MODULUS_LINEAR + 1, card.weftModulusQuadratic, 0.0);
    if (card.weftAngle)
        writer.real(FIELD_WEFT_ANGLE, *card.weftAngle);
    writer.realUnlessDefault(FIELD_WEFT, card.weftDirection[0], 0.0);
    writer.realUnlessDefault(FIELD_WEFT + 1, card.weftDirection[1], 1.0);
    writer.realUnlessDefault(FIELD_WEFT + 2, card.weftDirection[2], 0.0);
    writer.realUnlessDefault(FIELD_SHEAR_FRICTION, card.shearFriction, 0.0);
    if (card.shearModulus12)
        writer.real(FIELD_SHEAR_FRICTION + 1, *card.shearModulus12);
    writer.realUnlessDefault(FIELD_SHEAR_FRICTION + 2, card.fiberDamping, 0.1);
    writer.realUnlessDefault(FIELD_SHEAR_FRICTION + 3, card.fiberCompressionScale, 1.0);
    writer.realUnlessDefault(FIELD_SHEAR_FRICTION + 4, card.lockAngle1, 10.0);
    writer.realUnlessDefault(FIELD_SHEAR_FRICTION + 5, card.lockAngle2, 15.0);
}

std::uint32_t FabricMatTable::append(const FabricMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FabricMatTable::store(std::uint32_t row, const FabricMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FabricMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FabricMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
