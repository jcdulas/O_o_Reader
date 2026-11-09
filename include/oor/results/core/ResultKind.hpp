#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::results
{

// What a decoded result table holds. Each nodal kind is one container of the result object (`results.displacements`,
// `results.singlePointConstrForces`, ...); the three element kinds are families of containers, one per element type
// (`results.stress.ShellQuad4`), and so are the failure indices of the plies of laminates and the strain energy of the
// elements. The other kinds are the eigenvalues (a row a mode), the grid point forces (a row a grid point and
// element), the grid point stresses and strains (a container per variant: surface, volume direct, ...), the heat
// transfer results of the elements (the gradients and fluxes, the heat flows of the surface elements), the non-linear
// loads, and the contact, glue and bolt tables (forces, separation distances, tractions, slide distances, bolt
// forces), which are tables of points like the nodal ones. The kinds that are not blocks of rows (the grid point
// weight, the matrices, the optimisation tables, the aeroelastic tables) are `NamedResult`s. The mechanism shapes
// are made only by a program (no file holds them): the shapes of the motions a singular model allows without strain,
// a table like the eigenvectors with a step a shape. The kinetic energy of the elements (EKE) is a family like the
// strain energy.
enum class ResultKind : std::uint8_t
{
    Displacements,
    Velocities,
    Accelerations,
    Eigenvectors,
    SinglePointConstrForces,
    MultiPointConstrForces,
    AppliedLoads,
    Temperatures,
    ThermalLoads,
    TemperatureRates,
    Stress,
    Strain,
    ElementForces,
    FailureIndices,
    StrainEnergy,
    Eigenvalues,
    NodePointForces,
    NodePointStresses,
    NodePointStrains,
    HeatFlux,
    HeatFlows,
    NonlinearLoads,
    ContactForces,
    GlueForces,
    ConstraintHeat,
    SeparationInitial,
    SeparationFinal,
    ContactTractions,
    GlueSlide,
    BoltResults,
    MechanismShapes,
    KineticEnergy
};

// What a row of a table is.
enum class RowKind : std::uint8_t
{
    GridPoint,        // a grid point or scalar point (ResultData::ids)
    Element,          // a place in an element (ids = the element, nodeIds, layers, positions)
    GridPointElement, // a grid point of an element (ids = the grid point, elementIds = the element, or none)
    Mode              // a mode of an eigenvalue table (ids = the mode number)
};

// What is known about a kind, by itself.
struct ResultKindInfo
{
    ResultKind kind;
    std::string_view name;        // the name of the container: "displacements"
    std::string_view description; // what the table holds, in a few words
    bool nodal;                   // one row per grid point or scalar point
    bool scalar;                  // one component per row (temperatures), instead of six
    bool element;                 // rows are elements (or places in elements): stress, strain, element forces
    RowKind rows = RowKind::GridPoint;
    // One container per sub-name: the element types of an element kind, the variants of the grid point stresses.
    bool family = false;
};

[[nodiscard]] std::span<const ResultKindInfo> allResultKinds() noexcept;
[[nodiscard]] const ResultKindInfo& resultKindInfo(ResultKind kind) noexcept;
[[nodiscard]] std::string_view resultKindName(ResultKind kind) noexcept;
// The kind of a container name ("displacements"), in any case.
[[nodiscard]] std::optional<ResultKind> resultKindFromName(std::string_view name);
// The names of the components of a nodal kind, in the order the data holds them: t1 t2 t3 r1 r2 r3, or one name for a
// scalar kind ("temperature"). The components of an element table depend on its element type and layout, which only
// the data that holds them says; this gives none for them.
[[nodiscard]] std::vector<std::string> resultKindComponents(ResultKind kind);

} // namespace oor::results
