#include "oor/results/core/ResultKind.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <array>

namespace oor::results
{

namespace
{

constexpr auto KINDS = std::to_array<ResultKindInfo>({
    {ResultKind::Displacements, "displacements", "displacements of grid points and scalar points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::Velocities, "velocities", "velocities of grid points and scalar points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::Accelerations, "accelerations", "accelerations of grid points and scalar points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::Eigenvectors, "eigenvectors", "eigenvectors (mode shapes) of grid points and scalar points", true,
     false, false, RowKind::GridPoint, false},
    {ResultKind::SinglePointConstrForces, "singlePointConstrForces", "forces of single-point constraints", true, false,
     false, RowKind::GridPoint, false},
    {ResultKind::MultiPointConstrForces, "multiPointConstrForces", "forces of multipoint constraints", true, false,
     false, RowKind::GridPoint, false},
    {ResultKind::AppliedLoads, "appliedLoads", "applied loads on grid points and scalar points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::Temperatures, "temperatures", "temperatures of grid points and scalar points", true, true, false,
     RowKind::GridPoint, false},
    {ResultKind::ThermalLoads, "thermalLoads", "applied heat loads on grid points", true, true, false,
     RowKind::GridPoint, false},
    {ResultKind::TemperatureRates, "temperatureRates", "rates of change of the temperatures", true, true, false,
     RowKind::GridPoint, false},
    {ResultKind::Stress, "stress", "stresses of elements, one container per element type", false, false, true,
     RowKind::Element, true},
    {ResultKind::Strain, "strain", "strains of elements, one container per element type", false, false, true,
     RowKind::Element, true},
    {ResultKind::ElementForces, "forces", "forces of elements, one container per element type", false, false, true,
     RowKind::Element, true},
    {ResultKind::FailureIndices, "failureIndices",
     "failure indices of the plies of laminated shells, one container per element type", false, false, true,
     RowKind::Element, true},
    {ResultKind::StrainEnergy, "strainEnergy", "strain energy of elements, one container per element type", false,
     false, true, RowKind::Element, true},
    {ResultKind::Eigenvalues, "eigenvalues", "eigenvalues of the modes (real, complex or buckling), a row a mode",
     false, false, false, RowKind::Mode, false},
    {ResultKind::NodePointForces, "nodePointForces",
     "equilibrium forces of the grid points, a row for each grid point and element that acts on it", false, false,
     false, RowKind::GridPointElement, false},
    {ResultKind::NodePointStresses, "nodePointStresses",
     "stresses at the grid points (surface, volume direct, volume principal, discontinuities), one container per "
     "variant",
     false, false, false, RowKind::GridPointElement, true},
    {ResultKind::NodePointStrains, "nodePointStrains",
     "strains at the grid points (surface, volume direct, volume principal, discontinuities), one container per "
     "variant",
     false, false, false, RowKind::GridPointElement, true},
    {ResultKind::HeatFlux, "heatFlux",
     "temperature gradients and heat fluxes of elements, one container per element type", false, false, true,
     RowKind::Element, true},
    {ResultKind::HeatFlows, "heatFlows",
     "heat flows of the surface elements (applied, convected, radiated) and of the convection elements, one "
     "container per element type",
     false, false, true, RowKind::Element, true},
    {ResultKind::NonlinearLoads, "nonlinearLoads", "non-linear force vector (applied loads of a non-linear analysis)",
     true, false, false, RowKind::GridPoint, false},
    {ResultKind::ContactForces, "contactForces", "contact forces at grid points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::GlueForces, "glueForces", "glue forces at grid points", true, false, false, RowKind::GridPoint, false},
    {ResultKind::ConstraintHeat, "constraintHeat", "heat flow at the constrained points of a heat transfer analysis",
     true, true, false, RowKind::GridPoint, false},
    {ResultKind::SeparationInitial, "separationInitial", "initial contact separation distance at grid points", true,
     true, false, RowKind::GridPoint, false},
    {ResultKind::SeparationFinal, "separationFinal", "final (deformed) contact separation distance at grid points",
     true, true, false, RowKind::GridPoint, false},
    {ResultKind::ContactTractions, "contactTractions", "contact pressure and tractions at grid points", true, false,
     false, RowKind::GridPoint, false},
    {ResultKind::GlueSlide, "glueSlideDistance", "glue contact slide distance at grid points", true, false, false,
     RowKind::GridPoint, false},
    {ResultKind::BoltResults, "boltConnectionResults", "bolt forces and moments, a row a bolt element", true, false,
     false, RowKind::GridPoint, false},
    {ResultKind::MechanismShapes, "mechanismShapes",
     "the mechanism shapes of a singular model (the motions it allows without strain), a step a shape; made in "
     "memory by a solver, no result file holds them",
     true, false, false, RowKind::GridPoint, false},
    {ResultKind::KineticEnergy, "kineticEnergy", "kinetic energy of elements, one container per element type", false,
     false, true, RowKind::Element, true},
});

} // namespace

std::span<const ResultKindInfo> allResultKinds() noexcept
{
    return KINDS;
}

const ResultKindInfo& resultKindInfo(ResultKind kind) noexcept
{
    return KINDS[static_cast<std::size_t>(kind)];
}

std::string_view resultKindName(ResultKind kind) noexcept
{
    return resultKindInfo(kind).name;
}

std::optional<ResultKind> resultKindFromName(std::string_view name)
{
    for (const ResultKindInfo& info : KINDS)
        if (base::equalsIgnoreCase(info.name, name))
            return info.kind;
    return std::nullopt;
}

std::vector<std::string> resultKindComponents(ResultKind kind)
{
    switch (kind)
    {
    case ResultKind::Temperatures:
        return {"temperature"};
    case ResultKind::ThermalLoads:
        return {"heatLoad"};
    case ResultKind::TemperatureRates:
        return {"temperatureRate"};
    case ResultKind::ConstraintHeat:
        return {"heatFlow"};
    case ResultKind::SeparationInitial:
    case ResultKind::SeparationFinal:
        return {"separationDistance"};
    case ResultKind::ContactTractions:
        return {"pressure", "s1", "s2", "s3"};
    case ResultKind::BoltResults:
        return {"axialForce", "shearForce1", "shearForce2", "bendingMoment1", "bendingMoment2", "axialPreloadStrain"};
    case ResultKind::Stress:
    case ResultKind::Strain:
    case ResultKind::ElementForces:
    case ResultKind::FailureIndices:
    case ResultKind::StrainEnergy:
    case ResultKind::KineticEnergy:
    case ResultKind::Eigenvalues:
    case ResultKind::NodePointForces:
    case ResultKind::NodePointStresses:
    case ResultKind::NodePointStrains:
    case ResultKind::HeatFlux:
    case ResultKind::HeatFlows:
        return {};
    default:
        return {"t1", "t2", "t3", "r1", "r2", "r3"};
    }
}

} // namespace oor::results
