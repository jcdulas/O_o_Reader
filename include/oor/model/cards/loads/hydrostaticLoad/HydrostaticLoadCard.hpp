#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One HYDROS card: a hydrostatic pressure load on the faces of shell and solid elements, from element and grid sets
// (the fluid has no cavity of its own: its density RHO is on the card). The pressure at a face is PNOM plus RHO times G
// times the depth H below the free surface; the card becomes an equivalent set of PLOAD4 entries with the same load set
// SID.
//
// Card layout: HYDROS  SID  PNOM  RHO  G  SETE  SETG  PLD4  OVRD
//                     .    CID/GRD  HGHTB  HABOV  CNTL
// In this library's numbering (field 1 is SID): PNOM is field 2, RHO 3, G 4, SETE 5, SETG 6, PLD4 7, OVRD 8, and the
// second line is CID/GRD in field 9, HGHTB 10, HABOV 11 and CNTL 12. The defaults are static, so a field that holds its
// default is written blank: PNOM 0.0, G 1.0, SETE, SETG, PLD4, OVRD, CID/GRD and CNTL 0, HGHTB and HABOV 0.0. RHO is
// required and above zero (the format gives it no default). PNOM and G may be negative (an external pressure changes
// their signs); PLD4 and OVRD are 0 or 1, as the format lists them.
//
// SID is a load set ID shared with the other load cards (IdSpace::Group, category Load). SETE (elements of the
// interface) and SETG (their grid points) name a SET3 or a SET1 (ReferenceTarget::Set) and CNTL (the control grids that
// give the direction of the pressure on an open geometry) a SET3 of points; a zero is blank and names
// nothing. The format lets both SETE and SETG be blank, and so does the card. CID/GRD is one number whose sign is a
// switch (HydrostaticSupport.hpp): the card keeps it as written.
struct HydrostaticLoadCard
{
    base::EntityId id = 0;
    double nominalPressure = 0.0;
    double density = 0.0; // required, above zero
    double gravity = 1.0;
    base::EntityId elementSetId = 0;
    base::EntityId nodeSetId = 0;
    std::int32_t punchPressureLoads = 0;
    std::int32_t overrideOption = 0;
    base::EntityId frameOrNode = 0; // >= 0: a coordinate system (0 basic); < 0: minus the ID of a grid point
    double baseHeight = 0.0;
    double heightAbove = 0.0;
    base::EntityId controlNodeSetId = 0;

    [[nodiscard]] bool operator==(const HydrostaticLoadCard&) const noexcept = default;
};

} // namespace oor::model
