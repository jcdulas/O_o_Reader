#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One PSPRMAT card (SOL 700): the property of a translational spring element CSPR, linear or nonlinear.
//
// Card layout (the second line is empty but must be there):
//   PSPRMAT  PID                                                         line 1: field 1
//                                                                        line 2: empty (fields 9 to 16)
//            K  FAILMTF  FAILMCF  LOAD  UNLOAD                           line 3: fields 17 to 21
// The values of the line 3 are in fields 17 to 21, not 9 to 13: the second line has every cell empty, so a deck that
// leaves it out puts K in the wrong field and is refused (a K in field 9 is an error, not a guess). PID is a property
// number, unique among the properties (the card is a primary property): the CSPR names it.
//
// The card is linear or nonlinear, never both:
//   linear:    K is required (a real above 0.0, the stiffness: force per displacement). FAILMTF and FAILMCF, the
//              tensile and compressive failure forces (above 0.0), are optional and have no default (a blank one
//              means that the spring does not fail), so they are std::optional. LOAD and UNLOAD are not allowed.
//   nonlinear: LOAD is required, the TABLED1 of the force against the displacement while loading. UNLOAD is optional,
//              the table of the unloading path (the format says TABLExx: the card treats it as a TABLED1 like LOAD,
//              since every force-deflection curve of SOL 700 is one; a TABLEM1 or another kind named there is
//              reported as an unresolved reference); blank, the unloading path is the loading path. K, FAILMTF and
//              FAILMCF are not allowed.
// The format asks the two curves to be consistent (both with negative and positive values, or both with positive values
// only): that depends on the tables, which the card does not look at.
struct SpringOffsetPropCard
{
    base::EntityId id = 0;
    std::optional<double> stiffness;               // set: the linear form
    std::optional<double> tensileFailureForce;     // linear form only
    std::optional<double> compressiveFailureForce; // linear form only
    base::EntityId loadingTableId = 0;             // the TABLED1 of the nonlinear form (0 for the linear form)
    base::EntityId unloadingTableId = 0;           // the table of the unloading path (0: the same as LOAD)

    [[nodiscard]] bool isLinear() const noexcept { return stiffness.has_value(); }
};

} // namespace oor::model
