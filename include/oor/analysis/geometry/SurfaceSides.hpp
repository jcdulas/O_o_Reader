#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <vector>

namespace oor::analysis
{

// What a CHBDYE side number means for the conduction element it names.
enum class SurfaceSide
{
    Valid,      // the element has this side
    NoSuchSide, // the element has fewer sides (a tetrahedron has four, a prism five, a rod three)
    NotNumbered // this library has no side numbering for the element type (it gives the surface no geometry)
};

// The side `side` of the element at `element`, with the numbering of the format reference: the faces of the
// solids, the top, the edges and the bottom of a shell, the linear side and the two ends of a rod, tube, bar or beam.
[[nodiscard]] SurfaceSide surfaceSideOf(const model::Model& model, model::CardLocation element, int side);

// The kind of surface a heat transfer surface element is.
enum class SurfaceKind
{
    None, // not a surface element, or one whose element or side is missing
    Area, // a face: CHBDYG AREA3 to AREA8, a face of a solid, the top or bottom of a shell
    Line, // a segment or a surface of revolution: CHBDYP LINE, ELCYL, FTUBE, TUBE, CHBDYG REV, an edge of a shell, a
          // rod
    Point // a point: CHBDYP POINT, the end of a rod
};

// What a surface element covers: its kind and the grid points of its surface, the corners first (a CHBDYG: G1 to G4 and
// the mid-side points it has; a CHBDYP: G1 and G2; a CHBDYE: the corners of the side of the conduction element, in the
// order of the format reference side). `corners` is how many of `nodes` are corners (3 for AREA3 and AREA6, 4 for AREA4
// and AREA8, 2 for a line, 1 for a point).
struct SurfaceDescription
{
    SurfaceKind kind = SurfaceKind::None;
    std::vector<base::EntityId> nodes;
    std::size_t corners = 0;
};

// The description of the CHBDYE, CHBDYG or CHBDYP at `surface` (a SurfaceDescription of kind None for another card, or
// for a CHBDYE whose conduction element or side does not exist).
[[nodiscard]] SurfaceDescription describeSurface(const model::Model& model, model::CardLocation surface);

} // namespace oor::analysis
