#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One term of an MDMPC equation: the coefficient Aj times the component Cj of the point Gj, which is in the module
// MODj.
struct ModuleMultiPointConstrTerm
{
    ModuleId point; // MODj and Gj: a grid or scalar point, and the module that defines it (0: main Bulk Data)
    std::int32_t component = 0; // Cj: one digit from 1 to 6 for a grid point, 0 (blank) or 1 for a scalar point
    double coefficient = 0.0;   // Aj

    [[nodiscard]] bool operator==(const ModuleMultiPointConstrTerm&) const noexcept = default;
};

// One MDMPC card: a multipoint constraint equation, the sum of Aj * u(Gj, Cj) = 0, whose points may lie in different
// modules (the multi-module counterpart of the MPC, whose rules apply). The first term is the dependent degree of
// freedom, so its coefficient is not zero. A module is a BEGIN MODULE section of the deck, which the library keeps as
// text; module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDMPC  SID  MOD1  G1  C1  A1                                       fields 1 to 5 (6 to 8 are blank)
//                    MOD2  G2  C2  A2  MOD3  G3  C3  A3                          fields 9 to 16
//                    MOD4  G4  C4  A4  ...                                       fields 17 on
// A term is four fields. The first sits next to SID and the line is then blank up to its end; the other terms follow
// each other two on a line from field 9 (the first field of a continuation line is the marker, not a field of the
// card). The card belongs in the main Bulk Data (a rule of the deck, not of the card).
//
// The SID is selected by the Case Control MPC command. The terms with a blank coefficient other than A1 are 0.0.
struct ModuleMultiPointConstrCard
{
    base::EntityId id = 0;
    std::vector<ModuleMultiPointConstrTerm> terms; // at least one
};

} // namespace oor::model
