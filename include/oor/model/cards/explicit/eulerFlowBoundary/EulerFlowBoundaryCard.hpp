#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/BoundaryBox.hpp"
#include "oor/model/cards/explicit/EulerFlowBoundaryProperty.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace oor::model
{

// One FLOW card (SOL 700): a flow boundary of an Euler mesh with the properties of the material at the boundary.
// Material properties it does not define are those of the element next to the boundary. No other entry of the format
// reference has this name.
//
// Card layout: FLOW  LID  BCID  MESH  DIR                                      fields 1 to 4
//             +     XMIN  XMAX  YMIN  YMAX  ZMIN  ZMAX                       fields 9 to 14
//             +     TYPE1  VALUE1  TYPE2  VALUE2  TYPE3  VALUE3  TYPE4  VALUE4   fields 17 to 24
//             +     TYPE5  VALUE5                                              fields 25 and 26
// LID is the number of a set of flow boundaries: several FLOW cards share it, and a TLOAD1 names it as its EXCITEID
// (which then leaves its TID blank). The faces are chosen in one of three ways, which exclude each other in this order:
// BCID, the number of a set of BCSEG cards; else DIR, one of the six faces of the mesh box; else
// the box XMIN to ZMAX on the boundary of the mesh. A choice that is outranked is kept as given. MESH is the Euler
// mesh, used only when several exist and BCID is blank (a blank MESH means all of them). The box has the defaults and
// the shape of BoundaryBox. The TYPEi and VALUEi pairs (at most five) define the properties: MATERIAL (a MATDEUL),
// XVEL, YVEL, ZVEL, PRESSURE, DENSITY and SIE (numbers), FLOW (IN, OUT, BOTH or SYM, BOTH when blank: how the material
// may cross the boundary) and HYDSTAT (a HYDSTAT). The pairs are kept in the order written, and a pair whose two fields
// are blank is not a property. The card does not check that a type is given once, which the format reference does not
// say either, nor its advice against giving the pressure and the velocity together.
struct EulerFlowBoundaryCard
{
    static constexpr std::size_t MAX_PROPERTIES = 5;

    base::EntityId setId = 0;         // the set of flow boundaries, shared by the cards of the set
    base::EntityId contactBodyId = 0; // a set of BCSEG cards, 0 when blank
    base::EntityId meshId = 0;        // an Euler MESH, 0 when blank
    std::optional<BoundaryFace> meshFace;
    BoundaryBox box;
    std::vector<EulerFlowBoundaryProperty> properties;
};

} // namespace oor::model
