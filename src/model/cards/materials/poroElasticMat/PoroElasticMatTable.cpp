#include "oor/model/cards/materials/poroElasticMat/PoroElasticMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

bool isFinitePositive(double value)
{
    return std::isfinite(value) && value > 0.0;
}

bool isFiniteNonNegative(double value)
{
    return std::isfinite(value) && value >= 0.0;
}

// MIKI and DELANY are semi-empirical models of a porosity of 1: no viscosity and no porosity are asked for.
bool isSemiEmpirical(const std::string& option)
{
    return option == "MikiModel" || option == "DelanyBazleyModel";
}

} // namespace

std::optional<PoroElasticMatCard> PoroElasticMatFormat::read(FieldReader& reader)
{
    PoroElasticMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.solidMatId = reader.integer(2, "solidMatId", 0);
    card.fluidMatId = reader.requiredInteger(3, "fluidMatId");
    card.biotFactor = reader.real(4, "biotFactor", 1.0);
    card.simplifiedModel = reader.isBlank(5) ? std::string() : std::string(reader.text(5));
    card.solidDensity = reader.real(6, "solidDensity", 0.0);
    card.viscosity = reader.real(9, "viscosity", 0.0);
    card.specificHeatRatio = reader.real(10, "specificHeatRatio", 1.402);
    card.prandtlNumber = reader.real(11, "prandtlNumber", 0.71);
    card.porosity = reader.real(12, "porosity", 0.0);
    card.tortuosity = reader.real(13, "tortuosity", 1.0);
    card.flowResistivity = reader.requiredReal(14, "flowResistivity");
    card.viscousLength = reader.real(15, "viscousLength", 0.0);
    card.thermalLength = reader.real(16, "thermalLength", 0.0);
    requireUnusedBlank(reader, {7, 8}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PoroElasticMatFormat::check(const PoroElasticMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.fluidMatId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a fluid material",
                           "fluidMatId"};
    if (card.solidMatId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of an isotropic material, or blank",
                           "solidMatId"};
    if (!card.simplifiedModel.empty())
    {
        if (auto problem = checkKeyword(card.simplifiedModel, "simplifiedModel",
                                        {"LumpedMass", "Rigid", "MikiModel", "DelanyBazleyModel"}))
            return problem;
    }
    // the skeleton is left out of the rigid and the semi-empirical models; the others need it (the
    // lumped one may have its density in SRHO instead).
    const bool needsSkeleton =
        card.simplifiedModel.empty() || (card.simplifiedModel == "LumpedMass" && card.solidDensity == 0.0);
    if (card.solidMatId != 0 && (card.simplifiedModel == "Rigid" || isSemiEmpirical(card.simplifiedModel)))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is left blank with " +
                               CardProblem::fieldNamed("simplifiedModel") + " " + card.simplifiedModel,
                           "solidMatId"};
    if (card.solidMatId == 0 && needsSkeleton)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is the skeleton of the porous material", "solidMatId"};
    if (!isFinitePositive(card.biotFactor))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "biotFactor"};
    if (!isFiniteNonNegative(card.solidDensity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "solidDensity"};
    if (card.solidDensity != 0.0 && card.simplifiedModel != "LumpedMass")
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is for " + CardProblem::fieldNamed("simplifiedModel") + " LUMPED",
            "solidDensity"};
    if (!isSemiEmpirical(card.simplifiedModel))
    {
        if (!isFinitePositive(card.viscosity))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "viscosity"};
        if (!isFinitePositive(card.porosity))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "porosity"};
    }
    else if (!isFiniteNonNegative(card.viscosity) || !isFiniteNonNegative(card.porosity))
    {
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("porosity") + " must not be negative",
            "viscosity"};
    }
    if (!isFinitePositive(card.specificHeatRatio))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "specificHeatRatio"};
    if (!isFinitePositive(card.prandtlNumber))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "prandtlNumber"};
    if (!isFinitePositive(card.tortuosity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "tortuosity"};
    if (!isFinitePositive(card.flowResistivity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "flowResistivity"};
    if (!isFiniteNonNegative(card.viscousLength))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "viscousLength"};
    if (!isFiniteNonNegative(card.thermalLength))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "thermalLength"};
    return std::nullopt;
}

void PoroElasticMatFormat::write(const PoroElasticMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.solidMatId, 0);
    writer.integer(3, card.fluidMatId);
    writer.realUnlessDefault(4, card.biotFactor, 1.0);
    if (!card.simplifiedModel.empty())
        writer.text(5, card.simplifiedModel);
    writer.realUnlessDefault(6, card.solidDensity, 0.0);
    writer.realUnlessDefault(9, card.viscosity, 0.0);
    writer.realUnlessDefault(10, card.specificHeatRatio, 1.402);
    writer.realUnlessDefault(11, card.prandtlNumber, 0.71);
    writer.realUnlessDefault(12, card.porosity, 0.0);
    writer.realUnlessDefault(13, card.tortuosity, 1.0);
    writer.real(14, card.flowResistivity);
    writer.realUnlessDefault(15, card.viscousLength, 0.0);
    writer.realUnlessDefault(16, card.thermalLength, 0.0);
}

std::uint32_t PoroElasticMatTable::append(const PoroElasticMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_solidMatId.push(card.solidMatId);
    m_fluidMatId.push(card.fluidMatId);
    m_biotFactor.push(card.biotFactor);
    m_simplifiedModel.push(card.simplifiedModel);
    m_solidDensity.push(card.solidDensity);
    m_viscosity.push(card.viscosity);
    m_specificHeatRatio.push(card.specificHeatRatio);
    m_prandtlNumber.push(card.prandtlNumber);
    m_porosity.push(card.porosity);
    m_tortuosity.push(card.tortuosity);
    m_flowResistivity.push(card.flowResistivity);
    m_viscousLength.push(card.viscousLength);
    m_thermalLength.push(card.thermalLength);
    return row;
}

PoroElasticMatCard PoroElasticMatTable::card(std::uint32_t row) const
{
    PoroElasticMatCard card;
    card.id = m_id.get(row);
    card.solidMatId = m_solidMatId.get(row);
    card.fluidMatId = m_fluidMatId.get(row);
    card.biotFactor = m_biotFactor.get(row);
    card.simplifiedModel = m_simplifiedModel.get(row);
    card.solidDensity = m_solidDensity.get(row);
    card.viscosity = m_viscosity.get(row);
    card.specificHeatRatio = m_specificHeatRatio.get(row);
    card.prandtlNumber = m_prandtlNumber.get(row);
    card.porosity = m_porosity.get(row);
    card.tortuosity = m_tortuosity.get(row);
    card.flowResistivity = m_flowResistivity.get(row);
    card.viscousLength = m_viscousLength.get(row);
    card.thermalLength = m_thermalLength.get(row);
    return card;
}

void PoroElasticMatTable::store(std::uint32_t row, const PoroElasticMatCard& card)
{
    m_id.set(row, card.id);
    m_solidMatId.set(row, card.solidMatId);
    m_fluidMatId.set(row, card.fluidMatId);
    m_biotFactor.set(row, card.biotFactor);
    m_simplifiedModel.set(row, card.simplifiedModel);
    m_solidDensity.set(row, card.solidDensity);
    m_viscosity.set(row, card.viscosity);
    m_specificHeatRatio.set(row, card.specificHeatRatio);
    m_prandtlNumber.set(row, card.prandtlNumber);
    m_porosity.set(row, card.porosity);
    m_tortuosity.set(row, card.tortuosity);
    m_flowResistivity.set(row, card.flowResistivity);
    m_viscousLength.set(row, card.viscousLength);
    m_thermalLength.set(row, card.thermalLength);
}

void PoroElasticMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_solidMatId.keepRows(rows);
    m_fluidMatId.keepRows(rows);
    m_biotFactor.keepRows(rows);
    m_simplifiedModel.keepRows(rows);
    m_solidDensity.keepRows(rows);
    m_viscosity.keepRows(rows);
    m_specificHeatRatio.keepRows(rows);
    m_prandtlNumber.keepRows(rows);
    m_porosity.keepRows(rows);
    m_tortuosity.keepRows(rows);
    m_flowResistivity.keepRows(rows);
    m_viscousLength.keepRows(rows);
    m_thermalLength.keepRows(rows);
}

void PoroElasticMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId solidMatId = m_solidMatId.get(row); solidMatId != 0)
        m_solidMatId.set(row, visitor.visit("solidMatId", ReferenceTarget::Material, solidMatId));
    m_fluidMatId.set(row, visitor.visit("fluidMatId", ReferenceTarget::Material, m_fluidMatId.get(row)));
}

} // namespace oor::model
