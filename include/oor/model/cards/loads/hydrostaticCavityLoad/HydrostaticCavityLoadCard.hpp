#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One HYDROC card: a hydrostatic pressure load on the faces of shell and solid elements that are wetted by a fluid
// cavity. The cavity is a PSOLID (its MAT10 gives the density), the pressure at a wetted face is PNOM plus the density
// times G times the depth below the free surface, and PLD4 asks for the resulting PLOAD4 entries to be punched (the
// format reference says the card is not supported by SOL 101, which has no fluid-structure interface).
//
// Card layout: HYDROC  SID  PSOLID  PNOM  G  PCH  PLD4  OVRD  (blank)
//                     .    CID/GRD  HGHTB  HABOV
// In this library's numbering (field 1 is SID): PSOLID is field 2, PNOM 3, G 4, PCH 5, PLD4 6, OVRD 7, field 8 is
// blank, and the second line is CID/GRD in field 9, HGHTB 10 and HABOV 11. The defaults are static, so a field that
// holds its default is written blank: PNOM 0.0, G 1.0, PCH, PLD4 and OVRD 0, CID/GRD 0 (the basic system), HGHTB and
// HABOV 0.0. PCH is 0 to 3 (nothing, the grids, the elements, both: what is punched) and PLD4 and OVRD are 0 or 1, as
// the format lists them. PNOM and G may be negative (an external pressure: the format says to change the signs).
//
// SID is a load set ID shared with the other load cards (IdSpace::Group, category Load: Case Control LOAD selects it).
// PSOLID is the ID of a property (ReferenceTarget::Property): the format says a PSOLID and the library does not tell
// the kinds of property apart, so a card that names a PCOMP is not refused here. CID/GRD is one number whose sign is a
// switch (HydrostaticSupport.hpp): the card keeps it as written, and it names a coordinate system when it is above zero
// and a grid point when it is below.
struct HydrostaticCavityLoadCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    double nominalPressure = 0.0;
    double gravity = 1.0;
    std::int32_t punchOption = 0;
    std::int32_t punchPressureLoads = 0;
    std::int32_t overrideOption = 0;
    base::EntityId frameOrNode = 0; // >= 0: a coordinate system (0 basic); < 0: minus the ID of a grid point
    double baseHeight = 0.0;
    double heightAbove = 0.0;

    [[nodiscard]] bool operator==(const HydrostaticCavityLoadCard&) const noexcept = default;
};

} // namespace oor::model
