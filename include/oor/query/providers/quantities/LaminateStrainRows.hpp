#pragma once

#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// The value fields of the flats of the laminate strains: the mid-plane strains and the curvatures, engineering shears.
[[nodiscard]] const std::vector<std::string>& laminateStrainFields();

// The mid-plane strains and the curvatures of the shells from their force resultants, by classical
// lamination theory: [e0; k] = [A B; B D]^-1 [N; M], the ABD matrix of the property of each element (a PCOMP or PCOMPG
// laminate, or a PSHELL from its materials: shellStiffness) in its material axes. `forces` are flats of the resultants
// (nx ny nxy mx my mxy, the moments of the format: M = -integral of sigma z, so the lamination theory takes -M) in the
// element frame; the curvatures are those of the theory, the strain at the height z being e0 + z k. Each gives a flat
// of the same rows with the fields of laminateStrainFields, in the element frame, or in the material frame with
// `material`. NaN for an element whose property is not a shell one, whose ABD cannot be inverted, or that is not linked
// to the model; a complex flat (a frequency response) is left out.
[[nodiscard]] std::vector<ElementFlat> laminateStrainFlats(const ResultQueryContext& context,
                                                           const std::vector<ElementFlat>& forces, bool material);

// The core of the sandwich laminates, as the caller declares it (sandwichLayup's `corePly`): `byProperty` the number
// of the core ply in the stack of the laminate of a property ID (from 1, both halves of a SYM laminate), else `ply`
// that of every laminate; the thickest ply when neither gives one. A laminate whose stack has no such ply has no faces.
struct SandwichCore
{
    std::optional<std::size_t> ply;
    std::map<std::int32_t, std::size_t> byProperty;
};

// The value fields of the flats of a sandwich: the resultants of the face below the core (n1x n1y n1xy) and above it
// (n2x n2y n2xy), and the core shears (tcx tcy).
[[nodiscard]] const std::vector<std::string>& sandwichFields();

// The faces and core of the sandwich shells from their force resultants: the strains of the laminate
// (laminateStrainFlats), then the resultants of each face, A e0 + B k over its plies (sandwichLayup: the core is the
// ply `core` declares, else the thickest), the integral of the stresses of its plies; the core shears are the
// transverse shears over the core thickness. In the element frame, or the material frame with `material`. NaN for an
// element whose property is not a laminate or whose stack has no ply of the declared number.
[[nodiscard]] std::vector<ElementFlat> sandwichFlats(const ResultQueryContext& context,
                                                     const std::vector<ElementFlat>& forces, bool material,
                                                     const SandwichCore& core = {});

} // namespace oor::query
