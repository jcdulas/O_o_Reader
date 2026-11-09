#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One PLTSURF card: a surface element that is only drawn, the mesh of the surface of a 3-D model (the structure and
// cavity models of an acoustic analysis, the trim component surfaces of a PEM job).
//
// Card layout:  PLTSURF  EID  GID1  GID2  GID3  GID4  GID5  GID6  GID7        fields 1 to 8
//                       GID8                                                  field 9
// The element has 3, 4, 6 or 8 grid points, in the order of the CTRIA3, the CQUAD4, the CTRIA6 and the CQUAD8:
// the corners first, then the mid-side points. The grid points are listed from the first with no blank between
// them, so the number of points is that of the first blank field, and the card refuses 1, 2, 5 or 7 points, a blank in
// the list and a repeated point. The format gives the EID no range. The element takes no part in the analysis and
// behaves as a PLOTEL: its ID is in a space of its own (it is not a structural element: a deck may reuse the ID of
// one), and a SET3 names it by ID, as the trim component of an ACPEMCP does.
struct PlotSurfaceCard
{
    static constexpr std::size_t MAX_GRIDS = 8;

    base::EntityId id = 0;
    std::array<base::EntityId, MAX_GRIDS> grids = {}; // GID1 to GID8, 0 after the last grid point of the element

    // The number of grid points: the length of the run of positive IDs from the first one.
    [[nodiscard]] std::size_t gridCount() const noexcept
    {
        std::size_t count = 0;
        while (count < MAX_GRIDS && grids[count] > 0)
            ++count;
        return count;
    }
};

} // namespace oor::model
